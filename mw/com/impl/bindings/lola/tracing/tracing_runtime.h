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


#ifndef PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_TRACING_TRACING_RUNTIME_H
#define PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_TRACING_TRACING_RUNTIME_H

#include "platform/aas/language/safecpp/scoped_function/scope.h"
#include "platform/aas/lib/containers/dynamic_array.h"
#include "platform/aas/lib/memory/shared/i_shared_memory_resource.h"
#include "platform/aas/mw/com/impl/configuration/configuration.h"
#include "platform/aas/mw/com/impl/tracing/configuration/service_element_instance_identifier_view.h"
#include "platform/aas/mw/com/impl/tracing/configuration/service_element_type.h"
#include "platform/aas/mw/com/impl/tracing/i_tracing_runtime_binding.h"

#include <amp_optional.hpp>
#include <amp_string_view.hpp>

#include <mutex>
#include <unordered_map>
#include <utility>

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
namespace tracing
{

class TracingRuntime : public impl::tracing::ITracingRuntimeBinding
{
    friend class TracingRuntimeAttorney;
    friend class ServiceElementRegistrar;

  public:
    constexpr static amp::string_view kDummyElementNameForShmRegisterCallback{"DUMMY_ELEMENT_NAME"};
    constexpr static impl::tracing::ServiceElementType kDummyElementTypeForShmRegisterCallback{
        impl::tracing::ServiceElementType::EVENT};

    /// \brief Constructor
    /// \param number_of_service_elements_with_trace_done_callback The maximum number of service elements, which will
    ///        register themselves via RegisterServiceElement. This is used to set the capacity of
    ///        type_erased_sample_ptrs_.
    TracingRuntime(const std::size_t number_of_service_elements_with_trace_done_callback,
                   const Configuration& configuration) noexcept;
    ~TracingRuntime() override = default;

    TracingRuntime(const TracingRuntime&) = delete;
    TracingRuntime(TracingRuntime&&) noexcept = delete;
    TracingRuntime& operator=(const TracingRuntime&) = delete;
    TracingRuntime& operator=(TracingRuntime&&) noexcept = delete;

    TraceContextId RegisterServiceElement() noexcept override;

    bool RegisterWithGenericTraceApi() noexcept override;

    analysis::tracing::TraceClientId GetTraceClientId() const noexcept override { return trace_client_id_.value(); }

    void SetDataLossFlag(const bool new_value) noexcept override { data_loss_flag_ = new_value; }

    bool GetDataLossFlag() const noexcept override { return data_loss_flag_; }

    void RegisterShmObject(
        const impl::tracing::ServiceElementInstanceIdentifierView& service_element_instance_identifier_view,
        const analysis::tracing::ShmObjectHandle shm_object_handle,
        void* const shm_memory_start_address) noexcept override;
    void UnregisterShmObject(const impl::tracing::ServiceElementInstanceIdentifierView&
                                 service_element_instance_identifier_view) noexcept override;
    amp::optional<analysis::tracing::ShmObjectHandle> GetShmObjectHandle(
        const impl::tracing::ServiceElementInstanceIdentifierView& service_element_instance_identifier_view)
        const noexcept override;
    amp::optional<void*> GetShmRegionStartAddress(const impl::tracing::ServiceElementInstanceIdentifierView&
                                                      service_element_instance_identifier_view) const noexcept override;

    void CacheFileDescriptorForReregisteringShmObject(
        const impl::tracing::ServiceElementInstanceIdentifierView& service_element_instance_identifier_view,
        const memory::shared::ISharedMemoryResource::FileDescriptor shm_file_descriptor,
        void* const shm_memory_start_address) noexcept override;
    amp::optional<std::pair<memory::shared::ISharedMemoryResource::FileDescriptor, void*>>
    GetCachedFileDescriptorForReregisteringShmObject(
        const impl::tracing::ServiceElementInstanceIdentifierView& service_element_instance_identifier_view)
        const noexcept override;
    void ClearCachedFileDescriptorForReregisteringShmObject(
        const impl::tracing::ServiceElementInstanceIdentifierView& service_element_instance_identifier_view) noexcept
        override;
    analysis::tracing::ServiceInstanceElement ConvertToTracingServiceInstanceElement(
        const impl::tracing::ServiceElementInstanceIdentifierView service_element_instance_identifier_view)
        const override;

    bool IsServiceElementTracingActive(const TraceContextId service_element_idx) noexcept override;
    void SetTypeErasedSamplePtr(impl::tracing::TypeErasedSamplePtr type_erased_sample_ptr,
                                const TraceContextId service_element_idx) noexcept override;
    void ClearTypeErasedSamplePtr(const TraceContextId service_element_idx) noexcept override;

  private:
    /// \brief Helper struct which contains an optional sample_ptr and a mutex which is used to protect access to the
    ///        sample_ptr. A struct is used instead of a std::pair to make the code more explicit when accessing the
    ///        elements.
    struct TypeErasedSamplePtrWithMutex
    {
        amp::optional<impl::tracing::TypeErasedSamplePtr> sample_ptr;
        // 
        std::mutex mutex;
    };

    const Configuration& configuration_;
    amp::optional<analysis::tracing::TraceClientId> trace_client_id_;
    bool data_loss_flag_;

    /// \brief Array of type erased sample pointers containing one element per service element that registers itself via
    ///        RegisterServiceElement.
    ///
    /// Since the array is of fixed size, we can insert new elements and read other elements at the same time
    /// without synchronisation. However, operations on individual elements must be protected by a mutex.
    bmw::containers::DynamicArray<TypeErasedSamplePtrWithMutex> type_erased_sample_ptrs_;

    /// \brief Index in type_erased_sample_ptrs_ of the last service element that was registered via
    ///        RegisterServiceElement.
    std::size_t current_service_element_idx_;

    std::unordered_map<impl::tracing::ServiceElementInstanceIdentifierView,
                       std::pair<analysis::tracing::ShmObjectHandle, void*>>
        shm_object_handle_map_;
    std::unordered_map<impl::tracing::ServiceElementInstanceIdentifierView,
                       std::pair<memory::shared::ISharedMemoryResource::FileDescriptor, void*>>
        failed_shm_object_registration_cache_;

    /// \brief Ensure that the associated scoped function called only as long as the scope is not expired.
    /// \details The scope is used for the callback registered with RegisterTraceDoneCB.
    safecpp::Scope<> receive_handler_scope_;
};

}  // namespace tracing
}  // namespace lola
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_TRACING_TRACING_RUNTIME_H
