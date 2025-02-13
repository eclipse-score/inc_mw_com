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


#ifndef PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_SUBSCRIPTION_STATE_MACHINE_STATES_H
#define PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_SUBSCRIPTION_STATE_MACHINE_STATES_H

#include <string>

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

/**
 * Enum of states which is used to index into the state vector.
 **/
enum class SubscriptionStateMachineState : std::uint8_t
{
    NOT_SUBSCRIBED_STATE = 0,
    SUBSCRIPTION_PENDING_STATE,
    SUBSCRIBED_STATE,
    STATE_COUNT
};

std::string MessageForSubscriptionState(const SubscriptionStateMachineState& state) noexcept;

}  // namespace lola
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_SUBSCRIPTION_STATE_MACHINE_STATES_H
