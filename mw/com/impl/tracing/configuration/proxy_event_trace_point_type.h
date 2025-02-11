// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#ifndef PLATFORM_AAS_MW_COM_IMPL_TRACING_CONFIGURATION_PROXY_EVENT_TRACE_POINT_TYPE_H
#define PLATFORM_AAS_MW_COM_IMPL_TRACING_CONFIGURATION_PROXY_EVENT_TRACE_POINT_TYPE_H

#include <cstdint>

namespace bmw
{
namespace mw
{
namespace com
{
namespace impl
{
namespace tracing
{

enum class ProxyEventTracePointType : std::uint8_t
{
    INVALID = 0,
    SUBSCRIBE,
    UNSUBSCRIBE,
    SUBSCRIBE_STATE_CHANGE,
    SET_SUBSCRIPTION_STATE_CHANGE_HANDLER,
    UNSET_SUBSCRIPTION_STATE_CHANGE_HANDLER,
    SUBSCRIPTION_STATE_CHANGE_HANDLER_CALLBACK,
    SET_RECEIVE_HANDLER,
    UNSET_RECEIVE_HANDLER,
    RECEIVE_HANDLER_CALLBACK,
    GET_NEW_SAMPLES,
    GET_NEW_SAMPLES_CALLBACK
};

}  // namespace tracing
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_TRACING_CONFIGURATION_PROXY_EVENT_TRACE_POINT_TYPE_H
