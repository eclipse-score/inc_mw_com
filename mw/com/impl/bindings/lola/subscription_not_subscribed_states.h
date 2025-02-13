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


#ifndef PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_SUBSCRIPTION_NOT_SUBSCRIBED_STATES_H
#define PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_SUBSCRIPTION_NOT_SUBSCRIBED_STATES_H

#include "platform/aas/mw/com/impl/binding_event_receive_handler.h"
#include "platform/aas/mw/com/impl/bindings/lola/messaging/i_message_passing_service.h"
#include "platform/aas/mw/com/impl/bindings/lola/subscription_state_base.h"

#include <amp_optional.hpp>

#include <cstddef>
#include <exception>
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

class NotSubscribedState final : public SubscriptionStateBase
{
  public:
    // Inherit parent class constructor
    using SubscriptionStateBase::SubscriptionStateBase;

    NotSubscribedState(const NotSubscribedState&) = delete;
    NotSubscribedState(NotSubscribedState&&) = delete;
    NotSubscribedState& operator=(const NotSubscribedState&) & = delete;
    NotSubscribedState& operator=(NotSubscribedState&&) & = delete;

    ~NotSubscribedState() noexcept override final = default;

    ResultBlank SubscribeEvent(const std::size_t max_sample_count) noexcept override final;
    void UnsubscribeEvent() noexcept override final;
    void StopOfferEvent() noexcept override final;
    void ReOfferEvent(const pid_t new_event_source_pid) noexcept override final;

    void SetReceiveHandler(BindingEventReceiveHandler handler) noexcept override final;
    void UnsetReceiveHandler() noexcept override final;
    amp::optional<std::uint16_t> GetMaxSampleCount() const noexcept override final;
    amp::optional<SlotCollector>& GetSlotCollector() noexcept override final;
    const amp::optional<SlotCollector>& GetSlotCollector() const noexcept override final;
    amp::optional<TransactionLogSet::TransactionLogIndex> GetTransactionLogIndex() const noexcept override final;

    void OnEntry() noexcept override final;
};

}  // namespace lola
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_SUBSCRIPTION_NOT_SUBSCRIBED_STATES_H
