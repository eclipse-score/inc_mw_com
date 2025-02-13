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


#include "platform/aas/mw/com/impl/bindings/lola/event_data_control.h"

#include <iostream>

namespace bmw
{
namespace mw
{
namespace com
{
namespace impl
{
namespace lola
{
namespace detail_event_data_control
{

namespace
{

constexpr auto MAX_ALLOCATE_RETRIES{100U};
constexpr auto MAX_REFERENCE_RETRIES{100U};

}  // namespace

template <template <class> class AtomicIndirectorType>
EventDataControlImpl<AtomicIndirectorType>::EventDataControlImpl(
    const SlotIndexType max_slots,
    const bmw::memory::shared::MemoryResourceProxy* const proxy,
    const LolaEventInstanceDeployment::SubscriberCountType max_number_combined_subscribers) noexcept
    : state_slots_{max_slots, proxy}, transaction_log_set_{max_number_combined_subscribers, max_slots, proxy}
{
}

template <template <class> class AtomicIndirectorType>
auto EventDataControlImpl<AtomicIndirectorType>::AllocateNextSlot() noexcept -> amp::optional<SlotIndexType>
{
    amp::optional<SlotIndexType> selected_index{};
    auto retry_counter{0U};

    for (; retry_counter <= MAX_ALLOCATE_RETRIES; ++retry_counter)
    {
        selected_index = FindOldestUnusedSlot();

        if (!selected_index.has_value())
        {
            continue;
        }

        EventSlotStatus status{state_slots_[selected_index.value()].load(std::memory_order_acquire)};

        // we need to check that this is still the same, since it is possible that is has change after we found it
        // earlier
        if ((status.GetReferenceCount() != static_cast<EventSlotStatus::SubscriberCount>(0)) || status.IsInWriting())
        {
            continue;
        }

        EventSlotStatus status_new{};  // This will set the refcount to 0 by default
        status_new.MarkInWriting();

        auto status_value_type{static_cast<EventSlotStatus::value_type&>(status)};
        auto status_new_value_type{static_cast<EventSlotStatus::value_type&>(status_new)};
        if (state_slots_[selected_index.value()].compare_exchange_weak(
                status_value_type, status_new_value_type, std::memory_order_acq_rel))
        {
            break;
        }
    }

    num_alloc_retries.fetch_add(retry_counter);

    if (retry_counter >= MAX_ALLOCATE_RETRIES)
    {
        ++num_alloc_misses;
        // If this happens, it shows that we have a wrong configuration in the system, see doc-string
        return {};
    }

    return selected_index;
}

template <template <class> class AtomicIndirectorType>
auto EventDataControlImpl<AtomicIndirectorType>::FindOldestUnusedSlot() const noexcept -> amp::optional<SlotIndexType>
{
    EventSlotStatus::EventTimeStamp oldest_time_stamp{EventSlotStatus::TIMESTAMP_MAX};
    auto current_index{0};
    amp::optional<SlotIndexType> selected_index{};
    for (const auto& slot : state_slots_)
    {
        // 
        const EventSlotStatus status{slot.load(std::memory_order_acquire)};

        if (status.IsInvalid())
        {
            selected_index = current_index;
            break;
        }
        else if ((status.GetReferenceCount() == static_cast<EventSlotStatus::SubscriberCount>(0)) &&
                 (status.IsInWriting() == false))
        {
            if (status.GetTimeStamp() < oldest_time_stamp)
            {
                oldest_time_stamp = status.GetTimeStamp();
                selected_index = current_index;
            }
        }
        else
        {
            ; /* Not an unused slot, skip it. */
        }
        ++current_index;
    }
    return selected_index;
}

template <template <class> class AtomicIndirectorType>
auto EventDataControlImpl<AtomicIndirectorType>::EventReady(const SlotIndexType slot_index,
                                                            const EventSlotStatus::EventTimeStamp time_stamp) noexcept
    -> void
{
    const EventSlotStatus initial{time_stamp, 0};
    state_slots_[slot_index].store(static_cast<EventSlotStatus::value_type>(
        initial));  // no race-condition can happen, since sender is only in one thread
}

template <template <class> class AtomicIndirectorType>
auto EventDataControlImpl<AtomicIndirectorType>::Discard(const SlotIndexType slot_index) noexcept -> void
{
    auto slot = static_cast<EventSlotStatus>(state_slots_[slot_index].load(std::memory_order_acquire));
    if (slot.IsInWriting())
    {
        slot.MarkInvalid();
        state_slots_[slot_index].store(static_cast<EventSlotStatus::value_type>(slot), std::memory_order_release);
    }
}

template <template <class> class AtomicIndirectorType>
auto EventDataControlImpl<AtomicIndirectorType>::ReferenceSpecificEvent(
    const SlotIndexType slot_index,
    const TransactionLogSet::TransactionLogIndex transaction_log_index) noexcept -> bool
{
    auto& transaction_log = transaction_log_set_.GetTransactionLog(transaction_log_index);
    for (auto counter = 0U; counter < MAX_REFERENCE_RETRIES; counter++)
    {
        auto slot_current_status =
            static_cast<EventSlotStatus>(state_slots_[slot_index].load(std::memory_order_relaxed));

        if (slot_current_status.IsInWriting() || slot_current_status.IsInvalid())
        {
            return false;
        }

        auto slot_current_status_value{static_cast<EventSlotStatus::value_type>(slot_current_status)};
        const EventSlotStatus::value_type slot_new_status_value{slot_current_status_value + 1U};
        const EventSlotStatus slot_new_status{slot_new_status_value};
        if (slot_new_status.GetReferenceCount() == 0)
        {
            // ref-counter overflow
            return false;
        }

        transaction_log.ReferenceTransactionBegin(slot_index);
        if (AtomicIndirectorType<EventSlotStatus::value_type>::compare_exchange_weak(
                state_slots_[slot_index], slot_current_status_value, slot_new_status_value, std::memory_order_acq_rel))
        {
            transaction_log.ReferenceTransactionCommit(slot_index);
            return true;
        }
        transaction_log.ReferenceTransactionAbort(slot_index);
    }
    return false;
}

template <template <class> class AtomicIndirectorType>
auto EventDataControlImpl<AtomicIndirectorType>::ReferenceNextEvent(
    const EventSlotStatus::EventTimeStamp last_search_time,
    const TransactionLogSet::TransactionLogIndex transaction_log_index,
    const EventSlotStatus::EventTimeStamp upper_limit) noexcept -> amp::optional<SlotIndexType>
{
    // function can only finish with result, if use count was able to be increased
    amp::optional<SlotIndexType> possible_index{};

    auto& transaction_log = transaction_log_set_.GetTransactionLog(transaction_log_index);

    // possible optimization: We can remember a history of possible candidates, then we don't need to fully reiterate
    auto counter = 0U;
    for (; counter < MAX_REFERENCE_RETRIES; counter++)
    {
        
        /* False positive: possible_index.has_value_ is initialized during construction. */
        possible_index.reset();
        

        // initialize candidate_slot_status with last_search_time. candidate_slot_status.timestamp always reflects
        // "highest new timestamp". The sentinel, if we did find a possible candidate is always possible_index.
        EventSlotStatus candidate_slot_status{last_search_time, 0};

        auto current_index{0};
        for (const auto& slot : state_slots_)
        {
            // 
            const EventSlotStatus slot_status{slot.load(std::memory_order_relaxed)};
            if (slot_status.IsTimeStampBetween(candidate_slot_status.GetTimeStamp(), upper_limit))
            {
                possible_index = current_index;
                candidate_slot_status = slot_status;
            }

            ++current_index;
        }

        if (!possible_index.has_value())
        {
            return {};  // no sample within searched timestamp range exists.
        }

        EventSlotStatus::value_type status_new_val{candidate_slot_status};
        ++status_new_val;

        auto candidate_slot_status_value_type{static_cast<EventSlotStatus::value_type&>(candidate_slot_status)};

        auto possible_index_value = possible_index.value();
        auto& slot_value = state_slots_[possible_index_value];

        transaction_log.ReferenceTransactionBegin(possible_index_value);
        if (AtomicIndirectorType<EventSlotStatus::value_type>::compare_exchange_weak(
                slot_value, candidate_slot_status_value_type, status_new_val, std::memory_order_acq_rel))
        {
            transaction_log.ReferenceTransactionCommit(possible_index_value);
            break;
        }
        transaction_log.ReferenceTransactionAbort(possible_index_value);
    }

    num_ref_retries += counter;

    if (counter < MAX_REFERENCE_RETRIES)
    {
        return possible_index;
    }

    ++num_ref_misses;

    // if this happens it means we have a wrong configuration in the system, see doc-string
    return {};
}

template <template <class> class AtomicIndirectorType>
std::size_t EventDataControlImpl<AtomicIndirectorType>::GetNumNewEvents(
    const EventSlotStatus::EventTimeStamp reference_time) const noexcept
{
    std::size_t result{0};
    for (const auto& slot : state_slots_)
    {
        // 
        const EventSlotStatus slot_status{slot.load(std::memory_order_relaxed)};
        if (slot_status.IsTimeStampBetween(reference_time, EventSlotStatus::TIMESTAMP_MAX))
        {
            result++;
        }
    }
    return result;
}

template <template <class> class AtomicIndirectorType>
auto EventDataControlImpl<AtomicIndirectorType>::DereferenceEvent(
    const SlotIndexType event_slot_index,
    const TransactionLogSet::TransactionLogIndex transaction_log_index) noexcept -> void
{
    auto& transaction_log = transaction_log_set_.GetTransactionLog(transaction_log_index);
    transaction_log.DereferenceTransactionBegin(event_slot_index);
    DereferenceEventWithoutTransactionLogging(event_slot_index);
    transaction_log.DereferenceTransactionCommit(event_slot_index);
}

template <template <class> class AtomicIndirectorType>
void EventDataControlImpl<AtomicIndirectorType>::DereferenceEventWithoutTransactionLogging(
    const SlotIndexType event_slot_index) noexcept
{
    state_slots_[event_slot_index].fetch_sub(1, std::memory_order_acq_rel);
}

template <template <class> class AtomicIndirectorType>
auto EventDataControlImpl<AtomicIndirectorType>::operator[](const SlotIndexType slot_index) const noexcept
    -> EventSlotStatus
{
    return static_cast<EventSlotStatus>(state_slots_[slot_index].load(std::memory_order_acquire));
}

template <template <class> class AtomicIndirectorType>
auto EventDataControlImpl<AtomicIndirectorType>::RemoveAllocationsForWriting() noexcept -> void
{
    for (auto& slot : state_slots_)
    {
        // 
        EventSlotStatus status{slot.load(std::memory_order_acquire)};

        if (status.IsInWriting())
        {
            EventSlotStatus status_new{};

            auto status_value_type{static_cast<EventSlotStatus::value_type&>(status)};
            auto status_new_value_type{static_cast<EventSlotStatus::value_type&>(status_new)};
            if (slot.compare_exchange_weak(status_value_type, status_new_value_type, std::memory_order_acq_rel) ==
                false)
            {
                // atomic could not be changed, contract violation (other skeleton must be dead, nobody other should
                // change the slot)
                std::terminate();
            }
        }
    }
}

template <template <class> class AtomicIndirectorType>
auto EventDataControlImpl<AtomicIndirectorType>::DumpPerformanceCounters() -> void
{
    std::cout << "EventDataControlImpl performance breakdown\n"
              << "======================================\n"
              << "\nnum_alloc_misses:  " << num_alloc_misses << "\nnum_ref_misses     " << num_ref_misses
              << "\nnum_alloc_retries: " << num_alloc_retries << "\nnum_ref_retries:   " << num_ref_retries
              << std::endl;
}

template <template <class> class AtomicIndirectorType>
auto EventDataControlImpl<AtomicIndirectorType>::ResetPerformanceCounters() -> void
{
    num_alloc_misses.store(0U);
    num_ref_misses.store(0U);
    num_alloc_retries.store(0U);
    num_ref_retries.store(0U);
}

template class EventDataControlImpl<memory::shared::AtomicIndirectorReal>;
template class EventDataControlImpl<memory::shared::AtomicIndirectorMock>;

}  // namespace detail_event_data_control
}  // namespace lola
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
