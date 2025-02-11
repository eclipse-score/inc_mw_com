// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#include "platform/aas/mw/com/impl/bindings/lola/subscription_state_base.h"
#include "platform/aas/mw/com/impl/bindings/lola/subscription_state_machine.h"

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

SubscriptionStateBase::SubscriptionStateBase(SubscriptionStateMachine& subscription_event_state_machine) noexcept
    : state_machine_{subscription_event_state_machine}
{
}

}  // namespace lola
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
