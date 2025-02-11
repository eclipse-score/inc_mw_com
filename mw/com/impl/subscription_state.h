// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#ifndef PLATFORM_AAS_MW_COM_SUBSCRIPTIONSTATE_H
#define PLATFORM_AAS_MW_COM_SUBSCRIPTIONSTATE_H

#include <cstdint>

namespace bmw
{
namespace mw
{
namespace com
{
namespace impl
{

/// Subscription state of a proxy event.
/// \requirement 
enum class SubscriptionState : std::uint8_t
{
    kSubscribed,           ///< Proxy is subscribed to a particular event and will receive data.
    kNotSubscribed,        ///< Proxy is not subscribed, no data is received.
    kSubscriptionPending,  ///< Subscription is requested but not yet acknowledged by the producer.
};

}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_SUBSCRIPTIONSTATE_H
