// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#include "platform/aas/mw/com/impl/bindings/lola/subscription_not_subscribed_states.h"
#include "platform/aas/mw/com/impl/bindings/lola/event_subscription_control.h"
#include "platform/aas/mw/com/impl/bindings/lola/subscription_helpers.h"
#include "platform/aas/mw/com/impl/bindings/lola/subscription_state_machine.h"
#include "platform/aas/mw/com/impl/bindings/lola/subscription_state_machine_states.h"
#include "platform/aas/mw/com/impl/bindings/lola/transaction_log_registration_guard.h"
#include "platform/aas/mw/com/impl/com_error.h"

#include "platform/aas/lib/result/result.h"
#include "platform/aas/mw/log/logging.h"

#include <sstream>
#include <utility>

namespace bmw::mw::com::impl::lola
{

ResultBlank NotSubscribedState::SubscribeEvent(const std::size_t max_sample_count) noexcept
{
    auto transaction_log_registration_guard_result = TransactionLogRegistrationGuard::Create(
        state_machine_.event_control_.data_control, state_machine_.transaction_log_id_);
    if (!(transaction_log_registration_guard_result.has_value()))
    {
        std::stringstream ss{};
        ss << "Subscribe was rejected by skeleton. Could not Register TransactionLog due to "
           << transaction_log_registration_guard_result.error();
        ::bmw::mw::log::LogError("lola") << CreateLoggingString(
            ss.str(), state_machine_.GetElementFqId(), state_machine_.GetCurrentStateNoLock());
        return MakeUnexpected(ComErrc::kMaxSubscribersExceeded);
    }
    state_machine_.transaction_log_registration_guard_ = std::move(transaction_log_registration_guard_result).value();

    auto transaction_log_index = state_machine_.transaction_log_registration_guard_->GetTransactionLogIndex();
    auto& transaction_log =
        state_machine_.event_control_.data_control.GetTransactionLogSet().GetTransactionLog(transaction_log_index);
    transaction_log.SubscribeTransactionBegin(max_sample_count);

    const auto max_sample_count_uint16 = static_cast<std::uint16_t>(max_sample_count);
    const auto subscription_result =
        state_machine_.event_control_.subscription_control.Subscribe(max_sample_count_uint16);
    if (subscription_result != SubscribeResult::kSuccess)
    {
        AMP_ASSERT_MESSAGE(
            subscription_result != SubscribeResult::kMaxSubscribersOverflow,
            "TransactionLogRegistrationGuard::Create will return an error if we have a subscriber overflow.");
        transaction_log.SubscribeTransactionAbort();
        std::stringstream ss{};
        ss << "Subscribe was rejected by skeleton. Cannot complete SubscribeEvent() call due to "
           << ToString(subscription_result).data();
        ::bmw::mw::log::LogError("lola") << CreateLoggingString(
            ss.str(), state_machine_.GetElementFqId(), state_machine_.GetCurrentStateNoLock());
        state_machine_.transaction_log_registration_guard_.reset();
        return MakeUnexpected(ComErrc::kMaxSampleCountNotRealizable);
    }
    transaction_log.SubscribeTransactionCommit();

    SlotCollector slot_collector{
        state_machine_.event_control_.data_control, static_cast<std::size_t>(max_sample_count), transaction_log_index};
    if (state_machine_.event_receiver_handler_.has_value())
    {
        state_machine_.event_receive_handler_manager_.Register(
            std::move(state_machine_.event_receiver_handler_.value()));
        state_machine_.event_receiver_handler_.reset();
    }
    state_machine_.subscription_data_.slot_collector_ = std::move(slot_collector);
    state_machine_.subscription_data_.max_sample_count_ = max_sample_count;

    if (state_machine_.provider_service_instance_is_available_)
    {
        state_machine_.TransitionToState(SubscriptionStateMachineState::SUBSCRIBED_STATE);
    }
    else
    {
        state_machine_.TransitionToState(SubscriptionStateMachineState::SUBSCRIPTION_PENDING_STATE);
    }
    return {};
}

void NotSubscribedState::UnsubscribeEvent() noexcept {}

void NotSubscribedState::StopOfferEvent() noexcept
{
    state_machine_.provider_service_instance_is_available_ = false;
}

void NotSubscribedState::ReOfferEvent(const pid_t new_event_source_pid) noexcept
{
    state_machine_.event_receive_handler_manager_.UpdatePid(new_event_source_pid);
    state_machine_.provider_service_instance_is_available_ = true;
}

void NotSubscribedState::SetReceiveHandler(BindingEventReceiveHandler handler) noexcept
{
    state_machine_.event_receiver_handler_ = std::move(handler);
}

void NotSubscribedState::UnsetReceiveHandler() noexcept
{
    state_machine_.event_receiver_handler_ = amp::nullopt;
}

amp::optional<std::uint16_t> NotSubscribedState::GetMaxSampleCount() const noexcept
{
    AMP_ASSERT_MESSAGE(!state_machine_.subscription_data_.max_sample_count_.has_value(),
                       "Max sample count should not be set until Subscribe is called.");
    return {};
}

amp::optional<SlotCollector>& NotSubscribedState::GetSlotCollector() noexcept
{
    AMP_ASSERT_MESSAGE(!state_machine_.subscription_data_.slot_collector_.has_value(),
                       "Slot collector should not be created until Subscribe is called.");
    return state_machine_.subscription_data_.slot_collector_;
}

const amp::optional<SlotCollector>& NotSubscribedState::GetSlotCollector() const noexcept
{
    AMP_ASSERT_MESSAGE(!state_machine_.subscription_data_.slot_collector_.has_value(),
                       "Slot collector should not be created until Subscribe is called.");
    return state_machine_.subscription_data_.slot_collector_;
}

amp::optional<TransactionLogSet::TransactionLogIndex> NotSubscribedState::GetTransactionLogIndex() const noexcept
{
    AMP_ASSERT_MESSAGE(!state_machine_.transaction_log_registration_guard_.has_value(),
                       "TransactionLogRegistrationGuard should not be set until Subscribe is called.");
    return {};
}

void NotSubscribedState::OnEntry() noexcept
{
    const auto transaction_log_index = state_machine_.transaction_log_registration_guard_->GetTransactionLogIndex();
    auto& transaction_log =
        state_machine_.event_control_.data_control.GetTransactionLogSet().GetTransactionLog(transaction_log_index);

    transaction_log.UnsubscribeTransactionBegin();
    state_machine_.event_control_.subscription_control.Unsubscribe(
        state_machine_.subscription_data_.max_sample_count_.value());
    transaction_log.UnsubscribeTransactionCommit();

    state_machine_.event_receive_handler_manager_.Unregister();
    state_machine_.subscription_data_.Clear();
    state_machine_.transaction_log_registration_guard_.reset();
}

}  // namespace bmw::mw::com::impl::lola
