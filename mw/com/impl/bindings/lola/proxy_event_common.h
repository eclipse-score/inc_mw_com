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

#ifndef PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_PROXY_EVENT_COMMON_H
#define PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_PROXY_EVENT_COMMON_H

#include "platform/aas/mw/com/impl/binding_event_receive_handler.h"
#include "platform/aas/mw/com/impl/bindings/lola/element_fq_id.h"
#include "platform/aas/mw/com/impl/bindings/lola/event_control.h"
#include "platform/aas/mw/com/impl/bindings/lola/event_meta_info.h"
#include "platform/aas/mw/com/impl/bindings/lola/proxy.h"
#include "platform/aas/mw/com/impl/bindings/lola/slot_collector.h"
#include "platform/aas/mw/com/impl/bindings/lola/subscription_state_machine.h"
#include "platform/aas/mw/com/impl/bindings/lola/transaction_log_id.h"
#include "platform/aas/mw/com/impl/subscription_state.h"

#include "platform/aas/lib/result/result.h"

#include <amp_assert.hpp>
#include <amp_optional.hpp>
#include <amp_string_view.hpp>

#include <mutex>

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

/// \brief Type agnostic part of proxy event binding implementation for the Lola IPC binding
///
/// This class instantiates the SubscriptionStateMachine and forwards user calls to it. During subscription, the
/// state machine instantiates a SlotCollector whose ownership is then passed to this class. When the user calls
/// GetNewSamplesSlotIndices(), the call is forwarded to the SlotCollector.
class ProxyEventCommon final
{
    friend class ProxyEventCommonAttorney;

  public:
    ProxyEventCommon(Proxy& parent, const ElementFqId element_fq_id, const amp::string_view event_name);
    ~ProxyEventCommon();

    ProxyEventCommon(const ProxyEventCommon&) = delete;
    ProxyEventCommon(ProxyEventCommon&&) noexcept = delete;
    ProxyEventCommon& operator=(const ProxyEventCommon&) = delete;
    ProxyEventCommon& operator=(ProxyEventCommon&&) noexcept = delete;

    ResultBlank Subscribe(const std::size_t max_sample_count);
    void Unsubscribe();

    SubscriptionState GetSubscriptionState() const noexcept;

    /// \brief Returns the number of new samples a call to GetNewSamples() (given that parameter max_count
    ///        doesn't restrict it) would currently provide.
    ///
    /// The call is dispatched to SlotCollector. It is the responsibility of the calling code to ensure that
    /// GetNumNewSamplesAvailable() is only called when the event is in the subscribed state.
    Result<std::size_t> GetNumNewSamplesAvailable() const noexcept;

    /// \brief Get the indicies of the slots containing samples that are pending for reception.
    ///
    /// The call is dispatched to SlotCollector. It is the responsibility of the calling code to ensure that
    /// GetNewSamplesSlotIndices() is only called when the event is in the subscribed state.
    SlotCollector::SlotIndices GetNewSamplesSlotIndices(const std::size_t max_count) noexcept;

    ResultBlank SetReceiveHandler(BindingEventReceiveHandler handler);
    ResultBlank UnsetReceiveHandler();

    pid_t GetEventSourcePid() const noexcept;
    ElementFqId GetElementFQId() const noexcept { return event_fq_id_; };
    const void* GetRawEventDataStorage() const noexcept { return parent_.GetRawDataStorage(event_fq_id_); };
    EventControl& GetEventControl() const noexcept { return event_control_; };
    EventMetaInfo GetEventMetaInfo() const noexcept { return parent_.GetEventMetaInfo(event_fq_id_); };
    amp::optional<std::uint16_t> GetMaxSampleCount() const noexcept;
    amp::optional<TransactionLogSet::TransactionLogIndex> GetTransactionLogIndex() const noexcept;
    void NotifyServiceInstanceChangedAvailability(const bool is_available, const pid_t new_event_source_pid) noexcept;

  private:
    /// \brief Manually insert a slot collector. Only used for tests.
    void InjectSlotCollector(SlotCollector&& slot_collector) { test_slot_collector_ = std::move(slot_collector); };
    amp::optional<SlotCollector> test_slot_collector_;

    Proxy& parent_;
    ElementFqId event_fq_id_;
    const amp::string_view event_name_;
    TransactionLogId transaction_log_id_;
    EventControl& event_control_;
    SubscriptionStateMachine subscription_event_state_machine_;
};

}  // namespace lola
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_PROXY_EVENT_COMMON_H
