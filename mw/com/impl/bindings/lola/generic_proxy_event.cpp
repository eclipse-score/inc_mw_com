// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



///
/// @file
/// 
///

#include "platform/aas/mw/com/impl/bindings/lola/generic_proxy_event.h"

#include "platform/aas/mw/log/logging.h"

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

namespace
{

std::size_t CalculateAlignedSize(const std::size_t type_size, const std::size_t alignment)
{
    AMP_ASSERT_PRD_MESSAGE(alignment != 0, "Alignment must be non-zero");
    const auto remainder = type_size % alignment;
    if (remainder == 0)
    {
        return type_size;
    }
    return type_size + alignment - remainder;
}

}  // namespace

GenericProxyEvent::GenericProxyEvent(Proxy& parent, const ElementFqId element_fq_id, const amp::string_view event_name)
    : GenericProxyEventBinding{}, proxy_event_common_{parent, element_fq_id, event_name}
{
}

ResultBlank GenericProxyEvent::Subscribe(const std::size_t max_sample_count) noexcept
{
    return proxy_event_common_.Subscribe(max_sample_count);
}

void GenericProxyEvent::Unsubscribe() noexcept
{
    proxy_event_common_.Unsubscribe();
}

SubscriptionState GenericProxyEvent::GetSubscriptionState() const noexcept
{
    return proxy_event_common_.GetSubscriptionState();
}

inline Result<std::size_t> GenericProxyEvent::GetNumNewSamplesAvailable() const noexcept
{
    /// \todo When we have full service discovery, we can still dispatch to GetNumNewSamplesAvailable even if the
    /// provider side has gone down as long as we haven't called Unsubscribe(). See
    /// .
    const auto subscription_state = proxy_event_common_.GetSubscriptionState();
    if (subscription_state == SubscriptionState::kSubscribed)
    {
        return GetNumNewSamplesAvailableImpl();
    }
    else
    {
        return MakeUnexpected(ComErrc::kNotSubscribed,
                              "Attempt to call GetNumNewSamplesAvailable without successful subscription.");
    }
}

inline Result<std::size_t> GenericProxyEvent::GetNewSamples(Callback&& receiver, TrackerGuardFactory& tracker) noexcept
{
    /// \todo When we have full service discovery, we can still dispatch to GetNewSamples even if the provider side has
    /// gone down as long as we haven't called Unsubscribe(). See .
    const auto subscription_state = proxy_event_common_.GetSubscriptionState();
    if (subscription_state == SubscriptionState::kSubscribed)
    {
        return GetNewSamplesImpl(std::move(receiver), tracker);
    }
    else
    {
        return MakeUnexpected(ComErrc::kNotSubscribed,
                              "Attempt to call GetNewSamples without successful subscription.");
    }
}

std::size_t GenericProxyEvent::GetSampleSize() const noexcept
{
    const EventMetaInfo& event_meta_info = proxy_event_common_.GetEventMetaInfo();
    return event_meta_info.data_type_info_.size_of_;
}

bool GenericProxyEvent::HasSerializedFormat() const noexcept
{
    // our shared-memory based binding does no serialization at all!
    return false;
}

ResultBlank GenericProxyEvent::SetReceiveHandler(BindingEventReceiveHandler handler) noexcept
{
    return proxy_event_common_.SetReceiveHandler(std::move(handler));
}

ResultBlank GenericProxyEvent::UnsetReceiveHandler() noexcept
{
    return proxy_event_common_.UnsetReceiveHandler();
}

pid_t GenericProxyEvent::GetEventSourcePid() const noexcept
{
    return proxy_event_common_.GetEventSourcePid();
}

ElementFqId GenericProxyEvent::GetElementFQId() const noexcept
{
    return proxy_event_common_.GetElementFQId();
}

amp::optional<std::uint16_t> GenericProxyEvent::GetMaxSampleCount() const noexcept
{
    return proxy_event_common_.GetMaxSampleCount();
}

Result<std::size_t> GenericProxyEvent::GetNumNewSamplesAvailableImpl() const noexcept
{
    return proxy_event_common_.GetNumNewSamplesAvailable();
}

Result<std::size_t> GenericProxyEvent::GetNewSamplesImpl(Callback&& receiver, TrackerGuardFactory& tracker) noexcept
{
    const auto max_sample_count = tracker.GetNumAvailableGuards();

    const auto slot_indices = proxy_event_common_.GetNewSamplesSlotIndices(max_sample_count);

    auto& event_control = proxy_event_common_.GetEventControl();
    const EventMetaInfo& event_meta_info = proxy_event_common_.GetEventMetaInfo();

    const auto sample_size = event_meta_info.data_type_info_.size_of_;
    const auto sample_alignment = event_meta_info.data_type_info_.align_of_;
    const auto aligned_size = CalculateAlignedSize(sample_size, sample_alignment);

    auto transaction_log_index = proxy_event_common_.GetTransactionLogIndex();
    AMP_PRECONDITION_PRD_MESSAGE(transaction_log_index.has_value(),
                                 "GetNewSamplesImpl should only be called after a TransactionLog has been registered.");

    const void* const event_slots_raw_array = event_meta_info.event_slots_raw_array_.get();
    // AMP assert that the event_slots_raw_array address is according to sample_alignment
    for (auto slot = slot_indices.begin; slot != slot_indices.end; ++slot)
    {
        const auto slot_index = *slot;

        /* NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic) The pointer event_slots_raw_array points
           to the memory managed by a std::vector which has been type erased. The standard says that elements of a
           vector may be accessed by using offsets to regular pointers to elements. Therefore, the pointer
           arithmetic is being done on memory which can be treated as an array. */
        const auto event_slots_array = static_cast<const std::uint8_t*>(event_slots_raw_array);
        AMP_PRECONDITION_PRD_MESSAGE(nullptr != event_slots_array, "Null event slot array");
        const auto object_start_address = &event_slots_array[aligned_size * slot_index];
        /* NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic) deviation ends here */

        const EventSlotStatus event_slot_status{event_control.data_control[*slot]};
        const EventSlotStatus::EventTimeStamp sample_timestamp{event_slot_status.GetTimeStamp()};

        SamplePtr<void> sample{object_start_address, event_control.data_control, *slot, transaction_log_index.value()};

        auto guard = std::move(*tracker.TakeGuard());
        auto sample_binding_independent = this->MakeSamplePtr(std::move(sample), std::move(guard));

        receiver(std::move(sample_binding_independent), sample_timestamp);
    }

    const auto num_collected_slots = static_cast<std::size_t>(std::distance(slot_indices.begin, slot_indices.end));
    return num_collected_slots;
}

void GenericProxyEvent::NotifyServiceInstanceChangedAvailability(bool is_available, pid_t new_event_source_pid) noexcept
{
    proxy_event_common_.NotifyServiceInstanceChangedAvailability(is_available, new_event_source_pid);
}

}  // namespace lola
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
