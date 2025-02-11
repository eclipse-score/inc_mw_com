// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#include "platform/aas/mw/com/impl/bindings/lola/subscription_subscribed_states.h"
#include "platform/aas/mw/com/impl/bindings/lola/subscription_helpers.h"
#include "platform/aas/mw/com/impl/bindings/lola/subscription_state_machine.h"
#include "platform/aas/mw/com/impl/bindings/lola/subscription_state_machine_states.h"
#include "platform/aas/mw/com/impl/com_error.h"

#include "platform/aas/mw/log/logging.h"

#include <amp_assert.hpp>

#include <exception>
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

ResultBlank SubscribedState::SubscribeEvent(const std::size_t max_sample_count) noexcept
{
    const auto max_sample_count_uint16 = static_cast<std::uint16_t>(max_sample_count);
    if (state_machine_.subscription_data_.max_sample_count_.value() == max_sample_count_uint16)
    {
        ::bmw::mw::log::LogWarn("lola") << CreateLoggingString(
            "Calling SubscribeEvent() while already subscribed has no effect.",
            state_machine_.GetElementFqId(),
            state_machine_.GetCurrentStateNoLock());
        return {};
    }
    else
    {
        ::bmw::mw::log::LogError("lola") << CreateLoggingString(
            "Calling SubscribeEvent() while already subscribed with a different max_sample_count is illegal.",
            state_machine_.GetElementFqId(),
            state_machine_.GetCurrentStateNoLock());
        return MakeUnexpected(ComErrc::kMaxSampleCountNotRealizable);
    }
}

void SubscribedState::UnsubscribeEvent() noexcept
{
    // Unsubscribe functionality will be done in NotSubscribedState::OnEntry() which will be called synchronously by
    // TransitionToState. We do this to avoid code duplication between SubscriptionPendingState::UnsubscribeEvent() and
    // SubscribedState::UnsubscribeEvent()
    state_machine_.TransitionToState(SubscriptionStateMachineState::NOT_SUBSCRIBED_STATE);
}

void SubscribedState::StopOfferEvent() noexcept
{
    state_machine_.provider_service_instance_is_available_ = false;
    state_machine_.TransitionToState(SubscriptionStateMachineState::SUBSCRIPTION_PENDING_STATE);
}

void SubscribedState::ReOfferEvent(const pid_t) noexcept
{
    ::bmw::mw::log::LogWarn("lola") << CreateLoggingString("Service cannot be re-offered while already subscribed.",
                                                           state_machine_.GetElementFqId(),
                                                           state_machine_.GetCurrentStateNoLock());
}

void SubscribedState::SetReceiveHandler(BindingEventReceiveHandler handler) noexcept
{
    state_machine_.event_receive_handler_manager_.Register(std::move(handler));
}

void SubscribedState::UnsetReceiveHandler() noexcept
{
    state_machine_.event_receive_handler_manager_.Unregister();
}

amp::optional<std::uint16_t> SubscribedState::GetMaxSampleCount() const noexcept
{
    AMP_ASSERT_MESSAGE(
        state_machine_.subscription_data_.max_sample_count_.value(),
        "The subscription dispatch manager and the contained max sample count should be initialised on subscription.");
    return state_machine_.subscription_data_.max_sample_count_.value();
}

amp::optional<SlotCollector>& SubscribedState::GetSlotCollector() noexcept
{
    AMP_ASSERT_MESSAGE(state_machine_.subscription_data_.max_sample_count_.has_value(),
                       "The subscription data and the contained slot collector should be initialised on subscription.");
    return state_machine_.subscription_data_.slot_collector_;
}

const amp::optional<SlotCollector>& SubscribedState::GetSlotCollector() const noexcept
{
    AMP_ASSERT_MESSAGE(state_machine_.subscription_data_.max_sample_count_.has_value(),
                       "The subscription data and the contained slot collector should be initialised on subscription.");
    return state_machine_.subscription_data_.slot_collector_;
}

amp::optional<TransactionLogSet::TransactionLogIndex> SubscribedState::GetTransactionLogIndex() const noexcept
{
    AMP_ASSERT_MESSAGE(state_machine_.transaction_log_registration_guard_.has_value(),
                       "TransactionLogRegistrationGuard should be initialised on subscription.");
    return state_machine_.transaction_log_registration_guard_.value().GetTransactionLogIndex();
}

}  // namespace lola
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
