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


#ifndef PLATFORM_AAS_MW_COM_IMPL_TRACING_I_TRACING_RUNTIME_H
#define PLATFORM_AAS_MW_COM_IMPL_TRACING_I_TRACING_RUNTIME_H

#include "platform/aas/lib/memory/shared/i_shared_memory_resource.h"
#include "platform/aas/lib/result/result.h"
#include "platform/aas/mw/com/impl/binding_type.h"
#include "platform/aas/mw/com/impl/tracing/configuration/proxy_event_trace_point_type.h"
#include "platform/aas/mw/com/impl/tracing/configuration/proxy_field_trace_point_type.h"
#include "platform/aas/mw/com/impl/tracing/configuration/service_element_instance_identifier_view.h"
#include "platform/aas/mw/com/impl/tracing/configuration/skeleton_event_trace_point_type.h"
#include "platform/aas/mw/com/impl/tracing/configuration/skeleton_field_trace_point_type.h"
#include "platform/aas/mw/com/impl/tracing/i_tracing_runtime_binding.h"
#include "platform/aas/mw/com/impl/tracing/type_erased_sample_ptr.h"

#include <amp_variant.hpp>

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

class ITracingRuntime
{
  public:
    using TracePointType = amp::variant<ProxyEventTracePointType,
                                        ProxyFieldTracePointType,
                                        SkeletonEventTracePointType,
                                        SkeletonFieldTracePointType>;
    using TracePointDataId = analysis::tracing::AraComProperties::TracePointDataId;

    virtual ~ITracingRuntime() = default;

    virtual void DisableTracing() noexcept = 0;

    virtual impl::tracing::ITracingRuntimeBinding::TraceContextId RegisterServiceElement(
        const BindingType binding_type) noexcept = 0;

    virtual void SetDataLossFlag(const BindingType binding_type) noexcept = 0;
    virtual void RegisterShmObject(const BindingType binding_type,
                                   const ServiceElementInstanceIdentifierView service_element_instance_identifier_view,
                                   const memory::shared::ISharedMemoryResource::FileDescriptor shm_object_fd,
                                   void* const shm_memory_start_address) noexcept = 0;
    virtual void UnregisterShmObject(
        BindingType binding_type,
        ServiceElementInstanceIdentifierView service_element_instance_identifier_view) noexcept = 0;

    virtual ResultBlank Trace(const BindingType binding_type,
                              const impl::tracing::ITracingRuntimeBinding::TraceContextId trace_context_id,
                              const ServiceElementInstanceIdentifierView service_element_instance_identifier,
                              const TracePointType trace_point_type,
                              const TracePointDataId trace_point_data_id,
                              TypeErasedSamplePtr sample_ptr,
                              const void* const shm_data_ptr,
                              const std::size_t shm_data_size) noexcept = 0;

    virtual ResultBlank Trace(const BindingType binding_type,
                              const ServiceElementInstanceIdentifierView service_element_instance_identifier,
                              const TracePointType trace_point_type,
                              const amp::optional<TracePointDataId> trace_point_data_id,
                              const void* const local_data_ptr,
                              const std::size_t local_data_size) noexcept = 0;
};

}  // namespace tracing
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_TRACING_I_TRACING_RUNTIME_H
