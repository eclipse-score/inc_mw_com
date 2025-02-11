// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#ifndef PLATFORM_AAS_MW_COM_IMPL_TRACING_I_TRACING_RUNTIME_BINDING_H
#define PLATFORM_AAS_MW_COM_IMPL_TRACING_I_TRACING_RUNTIME_BINDING_H

#include "platform/aas/mw/com/impl/tracing/configuration/service_element_instance_identifier_view.h"

#include "platform/aas/analysis/tracing/library/generic_trace_api/generic_trace_api.h"
#include "platform/aas/lib/memory/shared/i_shared_memory_resource.h"
#include "platform/aas/mw/com/impl/tracing/type_erased_sample_ptr.h"

#include <amp_callback.hpp>
#include <amp_optional.hpp>

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

class ITracingRuntimeBinding
{
  public:
    using TraceContextId = analysis::tracing::TraceContextId;
    using TracedShmDataCallback = amp::callback<void()>;

    virtual ~ITracingRuntimeBinding() = default;

    /// \brief Registers LoLa service element that will call impl::Runtime::Trace with a ShmDataChunkList with the
    ///        TracingRuntime, which also needs a context_id and will lead to a TraceDoneCallback.
    /// \return The index of the callback in type_erased_sample_ptrs_. This should be passed when unregistering
    ///         the callback with UnregisterServiceElement. It should also be used to create the TraceContextId which
    ///         will be passed to a impl::TracingRuntime::Trace() call which will then be used to identify the
    ///         service element in this class.
    ///
    /// This must be called by every LoLa service element that will call impl::Runtime::Trace with a ShmDataChunkList.
    /// \todo In the future, if the TraceContextId is not fixed for each service element but generated dynamically, then
    /// it will not be generated once during registration. This signature would have to be refactored.
    virtual TraceContextId RegisterServiceElement() noexcept = 0;

    /// \brief Each binding specific tracing runtime represents a distinct client from the perspective of
    ///        GenericTraceAPI. So it registers itself with the GenericTraceAPI, which gets triggered via this method.
    /// \return true in case registering with GenericTraceAPI was successful, false else.
    virtual bool RegisterWithGenericTraceApi() noexcept = 0;

    /// \brief Return trace client id this binding specific tracing runtime got assigned in
    ///        RegisterWithGenericTraceApi()
    /// \return trace client id
    virtual analysis::tracing::TraceClientId GetTraceClientId() const noexcept = 0;

    /// \brief Set data loss flag for the specific binding.
    /// \param new_value
    virtual void SetDataLossFlag(const bool new_value) noexcept = 0;

    /// \brief Read data loss for the specific binding.
    /// \return
    virtual bool GetDataLossFlag() const noexcept = 0;

    /// \brief Register the shm-object, which has been successfully registered at GenericTraceAPI under
    ///        shm_object_handle with binding specific tracing runtime, which relates to/owns this shm-object.
    /// \param service_element_instance_identifier_view
    /// \param shm_object_handle
    /// \param shm_memory_start_address
    virtual void RegisterShmObject(
        const impl::tracing::ServiceElementInstanceIdentifierView& service_element_instance_identifier_view,
        const analysis::tracing::ShmObjectHandle shm_object_handle,
        void* const shm_memory_start_address) noexcept = 0;
    virtual void UnregisterShmObject(const impl::tracing::ServiceElementInstanceIdentifierView&
                                         service_element_instance_identifier_view) noexcept = 0;

    virtual amp::optional<analysis::tracing::ShmObjectHandle> GetShmObjectHandle(
        const impl::tracing::ServiceElementInstanceIdentifierView& service_element_instance_identifier_view)
        const noexcept = 0;
    virtual amp::optional<void*> GetShmRegionStartAddress(
        const impl::tracing::ServiceElementInstanceIdentifierView& service_element_instance_identifier_view)
        const noexcept = 0;

    virtual void CacheFileDescriptorForReregisteringShmObject(
        const impl::tracing::ServiceElementInstanceIdentifierView& service_element_instance_identifier_view,
        const memory::shared::ISharedMemoryResource::FileDescriptor shm_file_descriptor,
        void* const shm_memory_start_address) noexcept = 0;
    virtual amp::optional<std::pair<memory::shared::ISharedMemoryResource::FileDescriptor, void*>>
    GetCachedFileDescriptorForReregisteringShmObject(const impl::tracing::ServiceElementInstanceIdentifierView&
                                                         service_element_instance_identifier_view) const noexcept = 0;
    virtual void ClearCachedFileDescriptorForReregisteringShmObject(
        const impl::tracing::ServiceElementInstanceIdentifierView&
            service_element_instance_identifier_view) noexcept = 0;

    virtual analysis::tracing::ServiceInstanceElement ConvertToTracingServiceInstanceElement(
        const impl::tracing::ServiceElementInstanceIdentifierView service_element_instance_identifier_view) const = 0;

    virtual bool IsServiceElementTracingActive(const TraceContextId service_element_idx) noexcept = 0;
    virtual void SetTypeErasedSamplePtr(TypeErasedSamplePtr type_erased_sample_ptr,
                                        const TraceContextId service_element_idx) noexcept = 0;
    virtual void ClearTypeErasedSamplePtr(const TraceContextId service_element_idx) noexcept = 0;
};

}  // namespace tracing
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_TRACING_I_TRACING_RUNTIME_BINDING_H
