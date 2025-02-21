/********************************************************************************
* Copyright (c) 2025 Contributors to the Eclipse Foundation
*
* See the NOTICE file(s) distributed with this work for additional
* information regarding copyright ownership.
*
* This program and the accompanying materials are made available under the
* terms of the Apache License Version 2.0 which is available at
* https://www.apache.org/licenses/LICENSE-2.0
*
* SPDX-License-Identifier: Apache-2.0
********************************************************************************/


#include "platform/aas/mw/com/impl/bindings/lola/event_data_control_composite.h"
#include <utility>

namespace bmw::mw::com::impl::lola::detail_event_data_control_composite
{

namespace
{
constexpr std::size_t MAX_MULTI_ALLOCATE_COUNT{100U};
}

template <template <class> class AtomicIndirectorType>
EventDataControlCompositeImpl<AtomicIndirectorType>::EventDataControlCompositeImpl(
    EventDataControl* const asil_qm_control)
    : asil_qm_control_{asil_qm_control}, asil_b_control_{nullptr}, ignore_qm_control_{false}
{
    CheckForValidDataControls();
}

template <template <class> class AtomicIndirectorType>
EventDataControlCompositeImpl<AtomicIndirectorType>::EventDataControlCompositeImpl(
    EventDataControl* const asil_qm_control,
    EventDataControl* const asil_b_control)
    : asil_qm_control_{asil_qm_control}, asil_b_control_{asil_b_control}, ignore_qm_control_{false}
{
    CheckForValidDataControls();
}

template <template <class> class AtomicIndirectorType>
auto EventDataControlCompositeImpl<AtomicIndirectorType>::GetNextFreeMultiSlot() const noexcept
    -> amp::optional<std::tuple<EventDataControl::SlotIndexType, EventSlotStatus::EventTimeStamp>>
{
    EventSlotStatus::EventTimeStamp oldest_time_stamp{EventSlotStatus::TIMESTAMP_MAX};
    amp::optional<EventDataControl::SlotIndexType> possible_index{};

    for (EventDataControl::SlotIndexType slot_index = 0;
         slot_index < static_cast<EventDataControl::SlotIndexType>(asil_b_control_->state_slots_.size());
         ++slot_index)
    {
        const EventSlotStatus slot_qm{asil_qm_control_->state_slots_[slot_index].load(std::memory_order_acquire)};
        const EventSlotStatus slot_b{asil_b_control_->state_slots_[slot_index].load(std::memory_order_acquire)};
        if (slot_b.IsInvalid() || ((slot_qm.IsUsed() == false) && (slot_b.IsUsed() == false)))
        {
            const auto time_stamp = slot_b.GetTimeStamp();
            if (time_stamp < oldest_time_stamp)
            {
                possible_index = slot_index;
                oldest_time_stamp = time_stamp;
            }
        }
    }

    if (possible_index.has_value())
    {
        return std::make_tuple(possible_index.value(), oldest_time_stamp);
    }
    else
    {
        return amp::nullopt;
    }
}

template <template <class> class AtomicIndirectorType>
auto EventDataControlCompositeImpl<AtomicIndirectorType>::TryLockSlot(
    const EventDataControl::SlotIndexType slot,
    const EventSlotStatus::EventTimeStamp expected_time_stamp) noexcept -> bool
{
    auto& slot_value_qm = asil_qm_control_->state_slots_[slot];
    auto& slot_value_asil_b = asil_b_control_->state_slots_[slot];

    EventSlotStatus asil_qm_old{slot_value_qm.load(std::memory_order_acquire)};
    EventSlotStatus asil_b_old{slot_value_asil_b.load(std::memory_order_acquire)};

    if ((asil_qm_old.IsUsed() || (asil_qm_old.GetTimeStamp() > expected_time_stamp)) ||
        (asil_b_old.IsUsed() || (asil_b_old.GetTimeStamp() > expected_time_stamp)))
    {
        return false;
    }

    EventSlotStatus in_writing{};
    in_writing.MarkInWriting();

    auto asil_qm_old_value_type{static_cast<EventSlotStatus::value_type&>(asil_qm_old)};
    auto in_writing_value_type{static_cast<EventSlotStatus::value_type&>(in_writing)};
    if (AtomicIndirectorType<EventSlotStatus::value_type>::compare_exchange_strong(
            slot_value_qm, asil_qm_old_value_type, in_writing_value_type, std::memory_order_acq_rel) == false)
    {
        return false;
    }

    auto asil_b_old_value_type{static_cast<EventSlotStatus::value_type&>(asil_b_old)};
    if (AtomicIndirectorType<EventSlotStatus::value_type>::compare_exchange_strong(
            slot_value_asil_b, asil_b_old_value_type, in_writing_value_type, std::memory_order_acq_rel) == false)
    {
        // Roll back write lock on asil qm since lock on b failed
        slot_value_qm.store(static_cast<EventSlotStatus::value_type>(asil_qm_old), std::memory_order_release);
        return false;
    }

    return true;
}

template <template <class> class AtomicIndirectorType>
auto EventDataControlCompositeImpl<AtomicIndirectorType>::AllocateNextMultiSlot() noexcept
    -> amp::optional<EventDataControl::SlotIndexType>
{
    for (std::size_t counter{0}; counter < MAX_MULTI_ALLOCATE_COUNT; ++counter)
    {
        const auto possible_slot = GetNextFreeMultiSlot();
        if (possible_slot.has_value())
        {
            const auto& possible_slot_value = possible_slot.value();
            EventDataControl::SlotIndexType slot_index{std::get<0>(possible_slot_value)};
            const EventSlotStatus::EventTimeStamp time_stamp{std::get<1>(possible_slot_value)};
            if (TryLockSlot(slot_index, time_stamp))
            {
                return slot_index;
            }
        }
    }

    return amp::nullopt;
}

template <template <class> class AtomicIndirectorType>
auto EventDataControlCompositeImpl<AtomicIndirectorType>::AllocateNextSlot() noexcept
    -> std::pair<amp::optional<EventDataControl::SlotIndexType>, bool>
{
    if (asil_b_control_ != nullptr)
    {
        if (!ignore_qm_control_)
        {
            auto slot = AllocateNextMultiSlot();
            if (slot.has_value() == false)
            {
                // we failed to allocate a "multi-slot". This is per our definition a misbehaviour of the QM consumers.
                // from this point onwards, we ignore/dismiss the whole qm control section.
                ignore_qm_control_ = true;
                // fall back to allocation solely within asil_b control.
                slot = asil_b_control_->AllocateNextSlot();
            }
            return {slot, ignore_qm_control_};
        }
        else
        {
            return {asil_b_control_->AllocateNextSlot(), true};
        }
    }
    else
    {
        return {asil_qm_control_->AllocateNextSlot(), false};
    }
}

template <template <class> class AtomicIndirectorType>
auto EventDataControlCompositeImpl<AtomicIndirectorType>::EventReady(
    const EventDataControl::SlotIndexType slot,
    const EventSlotStatus::EventTimeStamp time_stamp) noexcept -> void
{
    if (asil_b_control_ != nullptr)
    {
        asil_b_control_->EventReady(slot, time_stamp);
    }

    if (!ignore_qm_control_)
    {
        asil_qm_control_->EventReady(slot, time_stamp);
    }
}

template <template <class> class AtomicIndirectorType>
auto EventDataControlCompositeImpl<AtomicIndirectorType>::Discard(const EventDataControl::SlotIndexType slot) noexcept
    -> void
{
    if (asil_b_control_ != nullptr)
    {
        asil_b_control_->Discard(slot);
    }

    if (!ignore_qm_control_)
    {
        asil_qm_control_->Discard(slot);
    }
}

template <template <class> class AtomicIndirectorType>
bool EventDataControlCompositeImpl<AtomicIndirectorType>::IsQmControlDisconnected() const noexcept
{
    return ignore_qm_control_;
}

template <template <class> class AtomicIndirectorType>
EventDataControl& EventDataControlCompositeImpl<AtomicIndirectorType>::GetQmEventDataControl() const noexcept
{
    return *asil_qm_control_;
}

template <template <class> class AtomicIndirectorType>
amp::optional<EventDataControl*>
EventDataControlCompositeImpl<AtomicIndirectorType>::GetAsilBEventDataControl() noexcept
{
    if (asil_b_control_ != nullptr)
    {
        return asil_b_control_;
    }
    return {};
}

template <template <class> class AtomicIndirectorType>
EventSlotStatus::EventTimeStamp EventDataControlCompositeImpl<AtomicIndirectorType>::GetEventSlotTimestamp(
    const EventDataControl::SlotIndexType slot) const noexcept
{
    if (asil_b_control_ != nullptr)
    {
        const EventSlotStatus event_slot_status{(*asil_b_control_)[slot]};
        const EventSlotStatus::EventTimeStamp sample_timestamp{event_slot_status.GetTimeStamp()};
        return sample_timestamp;
    }
    else
    {
        const EventSlotStatus event_slot_status{(*asil_qm_control_)[slot]};
        const EventSlotStatus::EventTimeStamp sample_timestamp{event_slot_status.GetTimeStamp()};
        return sample_timestamp;
    }
}

template <template <class> class AtomicIndirectorType>
void EventDataControlCompositeImpl<AtomicIndirectorType>::CheckForValidDataControls() const noexcept
{
    if (asil_qm_control_ == nullptr)
    {
        std::terminate();
    }
}

template <template <class> class AtomicIndirectorType>
EventSlotStatus::EventTimeStamp EventDataControlCompositeImpl<AtomicIndirectorType>::GetLatestTimestamp() const noexcept
{
    EventSlotStatus::EventTimeStamp latest_time_stamp{1};
    EventDataControl* control = (asil_b_control_ != nullptr) ? asil_b_control_ : asil_qm_control_;
    for (EventDataControl::SlotIndexType slot_index = 0;
         slot_index < static_cast<EventDataControl::SlotIndexType>(control->state_slots_.size());
         ++slot_index)
    {
        const EventSlotStatus slot{control->state_slots_[slot_index].load(std::memory_order_acquire)};
        if (!slot.IsInvalid() && !slot.IsInWriting())
        {
            const auto slot_time_stamp = slot.GetTimeStamp();
            if (latest_time_stamp < slot_time_stamp)
            {
                latest_time_stamp = slot_time_stamp;
            }
        }
    }
    return latest_time_stamp;
}

template class EventDataControlCompositeImpl<memory::shared::AtomicIndirectorReal>;
template class EventDataControlCompositeImpl<memory::shared::AtomicIndirectorMock>;

}  // namespace bmw::mw::com::impl::lola::detail_event_data_control_composite
