// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#ifndef PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_PROXY_EVENT_H
#define PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_PROXY_EVENT_H

#include "platform/aas/mw/com/impl/bindings/lola/event_data_storage.h"
#include "platform/aas/mw/com/impl/bindings/lola/proxy_event_common.h"
#include "platform/aas/mw/com/impl/proxy_event_binding.h"
#include "platform/aas/mw/com/impl/sample_reference_tracker.h"
#include "platform/aas/mw/com/impl/tracing/i_tracing_runtime.h"

#include "platform/aas/lib/result/result.h"
#include "platform/aas/mw/log/logging.h"

#include <amp_assert.hpp>
#include <amp_optional.hpp>
#include <amp_string_view.hpp>
#include <amp_variant.hpp>

#include <exception>
#include <iostream>
#include <limits>
#include <memory>
#include <mutex>
#include <sstream>
#include <utility>

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

/// \brief Proxy event binding implementation for the Lola IPC binding.
///
/// All subscription operations are implemented in the separate class SubscriptionStateMachine and the associated
/// states. All type agnostic proxy event operations are dispatched to the class ProxyEventCommon.
///
/// \tparam SampleType Data type that is transmitted
template <typename SampleType>
class ProxyEvent final : public ProxyEventBinding<SampleType>
{
    template <typename T>
    friend class ProxyEventAttorney;

  public:
    using typename ProxyEventBinding<SampleType>::Callback;

    ProxyEvent() = delete;
    /// Create a new instance that is bound to the specified ShmBindingInformation and ElementId.
    ///
    /// \param parent Parent proxy of the proxy event.
    /// \param element_fq_id The ID of the event inside the proxy type.
    /// \param event_name The name of the event inside the proxy type.
    ProxyEvent(Proxy& parent, const ElementFqId element_fq_id, const amp::string_view event_name)
        : ProxyEventBinding<SampleType>{}, proxy_event_common_{parent, element_fq_id, event_name}
    {
    }

    ProxyEvent(const ProxyEvent&) = delete;
    ProxyEvent(ProxyEvent&&) noexcept = delete;
    ProxyEvent& operator=(const ProxyEvent&) = delete;
    ProxyEvent& operator=(ProxyEvent&&) noexcept = delete;

    ~ProxyEvent() noexcept = default;

    ResultBlank Subscribe(const std::size_t max_sample_count) noexcept override
    {
        return proxy_event_common_.Subscribe(max_sample_count);
    }
    void Unsubscribe() noexcept override { proxy_event_common_.Unsubscribe(); }

    SubscriptionState GetSubscriptionState() const noexcept override
    {
        return proxy_event_common_.GetSubscriptionState();
    }
    Result<std::size_t> GetNumNewSamplesAvailable() const noexcept override;
    Result<std::size_t> GetNewSamples(Callback&& receiver, TrackerGuardFactory& tracker) noexcept override;

    ResultBlank SetReceiveHandler(BindingEventReceiveHandler handler) noexcept override
    {
        return proxy_event_common_.SetReceiveHandler(std::move(handler));
    }
    ResultBlank UnsetReceiveHandler() noexcept override { return proxy_event_common_.UnsetReceiveHandler(); }
    amp::optional<std::uint16_t> GetMaxSampleCount() const noexcept override
    {
        return proxy_event_common_.GetMaxSampleCount();
    }
    BindingType GetBindingType() const noexcept override { return BindingType::kLoLa; }
    void NotifyServiceInstanceChangedAvailability(bool is_available, pid_t new_event_source_pid) noexcept override
    {
        proxy_event_common_.NotifyServiceInstanceChangedAvailability(is_available, new_event_source_pid);
    }

    pid_t GetEventSourcePid() const noexcept { return proxy_event_common_.GetEventSourcePid(); }
    ElementFqId GetElementFQId() const noexcept { return proxy_event_common_.GetElementFQId(); };

  private:
    Result<std::size_t> GetNewSamplesImpl(Callback&& receiver, TrackerGuardFactory& tracker) noexcept;
    Result<std::size_t> GetNumNewSamplesAvailableImpl() const noexcept;

    ProxyEventCommon proxy_event_common_;
};

template <typename SampleType>
inline Result<std::size_t> ProxyEvent<SampleType>::GetNumNewSamplesAvailable() const noexcept
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

template <typename SampleType>
inline Result<std::size_t> ProxyEvent<SampleType>::GetNumNewSamplesAvailableImpl() const noexcept
{
    return proxy_event_common_.GetNumNewSamplesAvailable();
}

template <typename SampleType>
inline Result<std::size_t> ProxyEvent<SampleType>::GetNewSamples(Callback&& receiver,
                                                                 TrackerGuardFactory& tracker) noexcept
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

template <typename SampleType>
// 
inline Result<std::size_t> ProxyEvent<SampleType>::GetNewSamplesImpl(Callback&& receiver,
                                                                     TrackerGuardFactory& tracker) noexcept
{
    const auto max_sample_count = tracker.GetNumAvailableGuards();

    const auto slot_indices = proxy_event_common_.GetNewSamplesSlotIndices(max_sample_count);

    const void* const event_data_storage = proxy_event_common_.GetRawEventDataStorage();
    if (event_data_storage == nullptr)
    {
        bmw::mw::log::LogFatal("lola") << __func__ << __LINE__
                                       << "Unable to find data channel for given event instance. Terminating.";
        std::terminate();
    }
    auto& event_control = proxy_event_common_.GetEventControl();
    auto transaction_log_index = proxy_event_common_.GetTransactionLogIndex();
    AMP_PRECONDITION_PRD_MESSAGE(transaction_log_index.has_value(),
                                 "GetNewSamplesImpl should only be called after a TransactionLog has been registered.");

    const auto* const samples = static_cast<const EventDataStorage<SampleType>*>(event_data_storage);

    for (auto slot = slot_indices.begin; slot != slot_indices.end; ++slot)
    {
        const SampleType& sample_data{samples->at(*slot)};
        const EventSlotStatus event_slot_status{event_control.data_control[*slot]};
        const EventSlotStatus::EventTimeStamp sample_timestamp{event_slot_status.GetTimeStamp()};

        SamplePtr<SampleType> sample{&sample_data, event_control.data_control, *slot, transaction_log_index.value()};

        auto guard = std::move(*tracker.TakeGuard());
        auto sample_binding_independent = this->MakeSamplePtr(std::move(sample), std::move(guard));

        static_assert(
            sizeof(EventSlotStatus::EventTimeStamp) == sizeof(impl::tracing::ITracingRuntime::TracePointDataId),
            "Event timestamp is used for the trace point data id, therefore, the types should be the same.");
        receiver(std::move(sample_binding_independent),
                 static_cast<impl::tracing::ITracingRuntime::TracePointDataId>(sample_timestamp));
    }

    const auto num_collected_slots = static_cast<std::size_t>(std::distance(slot_indices.begin, slot_indices.end));
    return num_collected_slots;
}

}  // namespace lola
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_PROXY_EVENT_H
