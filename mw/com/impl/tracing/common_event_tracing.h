// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#ifndef PLATFORM_AAS_MW_COM_IMPL_TRACING_COMMON_EVENT_TRACING_H
#define PLATFORM_AAS_MW_COM_IMPL_TRACING_COMMON_EVENT_TRACING_H

#include "platform/aas/mw/com/impl/instance_identifier.h"
#include "platform/aas/mw/com/impl/tracing/configuration/service_element_instance_identifier_view.h"
#include "platform/aas/mw/com/impl/tracing/configuration/service_element_type.h"
#include "platform/aas/mw/com/impl/tracing/tracing_runtime.h"

#include <amp_optional.hpp>
#include <amp_string_view.hpp>

#include <cstdint>
#include <utility>

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

template <typename T>
std::pair<const void*, std::size_t> ConvertToFatPointer(const T& input_value)
{
    auto* const data_ptr = static_cast<const void*>(&input_value);
    const auto data_size = sizeof(input_value);
    return {data_ptr, data_size};
}

ResultBlank TraceData(const ServiceElementInstanceIdentifierView service_element_instance_identifier_view,
                      const TracingRuntime::TracePointType trace_point,
                      const BindingType binding_type,
                      const std::pair<const void*, std::size_t>& local_data_chunk = {nullptr, 0U},
                      const amp::optional<TracingRuntime::TracePointDataId> trace_point_data_id = {}) noexcept;

ResultBlank TraceShmData(const BindingType binding_type,
                         ITracingRuntimeBinding::TraceContextId trace_context_id,
                         const ServiceElementInstanceIdentifierView service_element_instance_identifier_view,
                         const TracingRuntime::TracePointType trace_point,
                         TracingRuntime::TracePointDataId trace_point_data_id,
                         amp::optional<TypeErasedSamplePtr> sample_ptr,
                         const std::pair<const void*, std::size_t>& data_chunk) noexcept;

ServiceElementInstanceIdentifierView GetServiceElementInstanceIdentifierView(
    const InstanceIdentifier& instance_identifier,
    const amp::string_view service_element_name,
    const ServiceElementType service_element_type) noexcept;

}  // namespace tracing
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_TRACING_COMMON_EVENT_TRACING_H
