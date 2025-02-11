// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#ifndef PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_SUBSCRIPTION_SUBSCRIPTION_STATE_BASE_H
#define PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_SUBSCRIPTION_SUBSCRIPTION_STATE_BASE_H

#include "platform/aas/mw/com/impl/binding_event_receive_handler.h"
#include "platform/aas/mw/com/impl/bindings/lola/messaging/i_message_passing_service.h"
#include "platform/aas/mw/com/impl/bindings/lola/slot_collector.h"
#include "platform/aas/mw/com/impl/bindings/lola/subscription_state_machine_states.h"
#include "platform/aas/mw/com/impl/bindings/lola/transaction_log_set.h"
#include "platform/aas/mw/com/impl/com_error.h"
#include "platform/aas/mw/com/impl/proxy_event_binding.h"

#include "platform/aas/lib/result/result.h"

#include <amp_optional.hpp>

#include <cstddef>
#include <functional>
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
class SubscriptionStateMachine;

class SubscriptionStateBase
{
  public:
    explicit SubscriptionStateBase(SubscriptionStateMachine& subscription_event_state_machine) noexcept;
    virtual ~SubscriptionStateBase() noexcept = default;

    SubscriptionStateBase(const SubscriptionStateBase&) = delete;
    SubscriptionStateBase& operator=(const SubscriptionStateBase&) & = delete;
    SubscriptionStateBase(SubscriptionStateBase&&) noexcept = delete;
    SubscriptionStateBase& operator=(SubscriptionStateBase&&) & noexcept = delete;

    virtual ResultBlank SubscribeEvent(const std::size_t max_sample_count) noexcept = 0;
    virtual void UnsubscribeEvent() noexcept = 0;
    virtual void StopOfferEvent() noexcept = 0;
    virtual void ReOfferEvent(const pid_t new_event_source_pid) noexcept = 0;

    virtual void SetReceiveHandler(BindingEventReceiveHandler handler) noexcept = 0;
    virtual void UnsetReceiveHandler() noexcept = 0;
    virtual amp::optional<std::uint16_t> GetMaxSampleCount() const noexcept = 0;
    virtual amp::optional<SlotCollector>& GetSlotCollector() noexcept = 0;
    virtual const amp::optional<SlotCollector>& GetSlotCollector() const noexcept = 0;
    virtual amp::optional<TransactionLogSet::TransactionLogIndex> GetTransactionLogIndex() const noexcept = 0;

    virtual void OnEntry() noexcept {};
    virtual void OnExit() noexcept {};

  protected:
    SubscriptionStateMachine& state_machine_;
};

}  // namespace lola
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_SUBSCRIPTION_SUBSCRIPTION_STATE_BASE_H
