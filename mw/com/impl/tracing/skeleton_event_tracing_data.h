// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#ifndef PLATFORM_AAS_MW_COM_IMPL_TRACING_SKELETON_EVENT_TRACING_DATA_H
#define PLATFORM_AAS_MW_COM_IMPL_TRACING_SKELETON_EVENT_TRACING_DATA_H

#include "platform/aas/mw/com/impl/tracing/configuration/service_element_instance_identifier_view.h"
#include "platform/aas/mw/com/impl/tracing/i_tracing_runtime_binding.h"

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

struct SkeletonEventTracingData
{
    ServiceElementInstanceIdentifierView service_element_instance_identifier_view{};
    ITracingRuntimeBinding::TraceContextId trace_context_id{};

    bool enable_send{false};
    bool enable_send_with_allocate{false};
};

void DisableAllTracePoints(SkeletonEventTracingData& skeleton_event_tracing_data) noexcept;

}  // namespace tracing
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_TRACING_SKELETON_EVENT_TRACING_DATA_H
