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


#include "platform/aas/mw/com/impl/tracing/common_event_tracing.h"

#include "platform/aas/mw/com/impl/runtime.h"

#include <amp_assert.hpp>

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

namespace
{

amp::string_view GetServiceType(const InstanceIdentifier& instance_identifier) noexcept
{
    InstanceIdentifierView instance_identifier_view{instance_identifier};
    const auto& service_instance_deployment = instance_identifier_view.GetServiceInstanceDeployment();
    return service_instance_deployment.service_.ToString();
}

amp::string_view GetInstanceSpecifier(const InstanceIdentifier& instance_identifier) noexcept
{
    InstanceIdentifierView instance_identifier_view{instance_identifier};
    const auto& service_instance_deployment = instance_identifier_view.GetServiceInstanceDeployment();
    return service_instance_deployment.instance_specifier_.ToString();
}

}  // namespace

ResultBlank TraceData(const ServiceElementInstanceIdentifierView service_element_instance_identifier_view,
                      const TracingRuntime::TracePointType trace_point,
                      const BindingType binding_type,
                      const std::pair<const void*, std::size_t>& local_data_chunk,
                      const amp::optional<TracingRuntime::TracePointDataId> trace_point_data_id) noexcept
{
    auto* const tracing_runtime = impl::Runtime::getInstance().GetTracingRuntime();
    AMP_ASSERT_PRD(tracing_runtime != nullptr);
    return tracing_runtime->Trace(binding_type,
                                  service_element_instance_identifier_view,
                                  trace_point,
                                  trace_point_data_id,
                                  local_data_chunk.first,
                                  local_data_chunk.second);
}

ResultBlank TraceShmData(const BindingType binding_type,
                         ITracingRuntimeBinding::TraceContextId trace_context_id,
                         const ServiceElementInstanceIdentifierView service_element_instance_identifier_view,
                         const TracingRuntime::TracePointType trace_point,
                         TracingRuntime::TracePointDataId trace_point_data_id,
                         amp::optional<TypeErasedSamplePtr> sample_ptr,
                         const std::pair<const void*, std::size_t>& data_chunk) noexcept
{
    auto* const tracing_runtime = impl::Runtime::getInstance().GetTracingRuntime();
    if (tracing_runtime == nullptr)
    {
        return {};
    }

    if (!sample_ptr.has_value())
    {
        tracing_runtime->SetDataLossFlag(binding_type);
        return {};
    }

    return tracing_runtime->Trace(binding_type,
                                  trace_context_id,
                                  service_element_instance_identifier_view,
                                  trace_point,
                                  trace_point_data_id,
                                  std::move(sample_ptr.value()),
                                  data_chunk.first,
                                  data_chunk.second);
}

ServiceElementInstanceIdentifierView GetServiceElementInstanceIdentifierView(
    const InstanceIdentifier& instance_identifier,
    const amp::string_view service_element_name,
    const ServiceElementType service_element_type) noexcept
{
    const auto instance_specifier_view = GetInstanceSpecifier(instance_identifier);
    const auto service_type = GetServiceType(instance_identifier);
    const ServiceElementIdentifierView service_element_identifier_view{
        service_type, service_element_name, service_element_type};
    const ServiceElementInstanceIdentifierView service_element_instance_identifier_view{service_element_identifier_view,
                                                                                        instance_specifier_view};
    return service_element_instance_identifier_view;
}

}  // namespace tracing
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
