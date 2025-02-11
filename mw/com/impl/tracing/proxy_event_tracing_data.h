// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#ifndef PLATFORM_AAS_MW_COM_IMPL_TRACING_PROXY_EVENT_TRACING_DATA_H
#define PLATFORM_AAS_MW_COM_IMPL_TRACING_PROXY_EVENT_TRACING_DATA_H

#include "platform/aas/mw/com/impl/tracing/configuration/service_element_instance_identifier_view.h"

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

struct ProxyEventTracingData
{
    ServiceElementInstanceIdentifierView service_element_instance_identifier_view{};

    bool enable_subscribe{false};
    bool enable_unsubscribe{false};
    bool enable_subscription_state_changed{false};
    bool enable_set_subcription_state_change_handler{false};
    bool enable_unset_subscription_state_change_handler{false};
    bool enable_call_subscription_state_change_handler{false};
    bool enable_set_receive_handler{false};
    bool enable_unset_receive_handler{false};
    bool enable_call_receive_handler{false};
    bool enable_get_new_samples{false};
    bool enable_new_samples_callback{false};
};

void DisableAllTracePoints(ProxyEventTracingData& proxy_event_tracing_data) noexcept;

}  // namespace tracing
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_TRACING_PROXY_EVENT_TRACING_DATA_H
