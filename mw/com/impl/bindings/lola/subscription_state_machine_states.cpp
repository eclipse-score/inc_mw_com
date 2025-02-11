// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#include "platform/aas/mw/com/impl/bindings/lola/subscription_state_machine_states.h"

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

std::string MessageForSubscriptionState(const SubscriptionStateMachineState& state) noexcept
{
    using State = SubscriptionStateMachineState;
    switch (state)
    {
        case State::NOT_SUBSCRIBED_STATE:
            return "Not Subscribed State.";
        case State::SUBSCRIPTION_PENDING_STATE:
            return "Subscription Pending State.";
        case State::SUBSCRIBED_STATE:
            return "Subscribed State.";
        case State::STATE_COUNT:
            return "Invalid state: State Count is not a valid state.";
        default:
            return "Unknown subscription state.";
    }
}

}  // namespace lola
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
