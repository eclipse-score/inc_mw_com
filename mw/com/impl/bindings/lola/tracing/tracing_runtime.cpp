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


#include "platform/aas/mw/com/impl/bindings/lola/tracing/tracing_runtime.h"
#include "platform/aas/analysis/tracing/common/types.h"
#include "platform/aas/language/safecpp/scoped_function/scope.h"
#include "platform/aas/mw/log/logging.h"

#include <amp_assert.hpp>

#include <exception>
#include <limits>
#include <mutex>

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

namespace
{

/// \brief Converts a detailed/element specific ServiceElementInstanceIdentifierView used by the binding independent
///        layer into a representation used by the lola binding.
/// \details In the context of shm-object identification, the binding independent layer expects/supports that a shm-
///          capable binding maintains shm-objects per service-element! I.e. a shm-object is identified by a full
///          fledged ServiceElementInstanceIdentifierView. But LoLa only maintains shm-objects on the granularity level
///          of service-instances (aggregating many service elements). So in the LoLa case
///          ServiceElementInstanceIdentifierView::service_element_identifier_view::service_element_name and
///          ServiceElementInstanceIdentifierView::service_element_identifier_view::service_element_type are just
///          an aggregated dummy value!
///          But whenever the upper/binding independent layer makes a lookup for a shm-object on the detailed
///          ServiceElementInstanceIdentifierView (with real/concrete service-element names and types), we have to
///          transform it into the simplified/aggregated ServiceElementInstanceIdentifierView, which LoLa uses ...
/// \param service_element_instance_identifier_view detailed/element specific ServiceElementInstanceIdentifierView used
///         by the binding independent layer
///
/// \return converted LoLa binding specific ServiceElementInstanceIdentifierView to identify a shm-object.
impl::tracing::ServiceElementInstanceIdentifierView ConvertServiceElementInstanceIdentifierViewForLoLaShmIdentification(
    const impl::tracing::ServiceElementInstanceIdentifierView& service_element_instance_identifier_view)
{
    impl::tracing::ServiceElementInstanceIdentifierView simplifiedIdentifier{service_element_instance_identifier_view};
    simplifiedIdentifier.service_element_identifier_view.service_element_name =
        TracingRuntime::kDummyElementNameForShmRegisterCallback;
    simplifiedIdentifier.service_element_identifier_view.service_element_type =
        TracingRuntime::kDummyElementTypeForShmRegisterCallback;
    return simplifiedIdentifier;
}

}  // namespace

TracingRuntime::TracingRuntime(const std::size_t number_of_service_elements_with_trace_done_callback,
                               const Configuration& configuration) noexcept
    : impl::tracing::ITracingRuntimeBinding{},
      configuration_{configuration},
      trace_client_id_{},
      data_loss_flag_{false},
      type_erased_sample_ptrs_{number_of_service_elements_with_trace_done_callback},
      current_service_element_idx_{0U},
      shm_object_handle_map_{},
      failed_shm_object_registration_cache_{},
      receive_handler_scope_{}
{
}

bool TracingRuntime::RegisterWithGenericTraceApi() noexcept
{
    const auto app_instance_identifier = configuration_.GetTracingConfiguration().GetApplicationInstanceID();
    std::string app_instance_identifier_string{app_instance_identifier.data(), app_instance_identifier.size()};
    const auto register_client_result = analysis::tracing::GenericTraceAPI::RegisterClient(
        analysis::tracing::BindingType::kLoLa, app_instance_identifier_string);
    if (!register_client_result.has_value())
    {
        bmw::mw::log::LogError("lola")
            << "Lola TracingRuntime: RegisterClient with the GenericTraceAPI failed with error:"
            << register_client_result.error();
        return false;
    }
    trace_client_id_ = *register_client_result;

    analysis::tracing::TraceDoneCallBackType trace_done_callback{
        receive_handler_scope_, [this](analysis::tracing::TraceContextId trace_context_id) noexcept {
            if (!IsServiceElementTracingActive(trace_context_id))
            {
                bmw::mw::log::LogWarn("lola")
                    << "Lola TracingRuntime: TraceDoneCB with TraceContextId" << trace_context_id
                    << "was not pending but has been called anyway. Ignoring callback.";
                return;
            }
            ClearTypeErasedSamplePtr(trace_context_id);
        }};
    const auto register_trace_done_callback_result =
        analysis::tracing::GenericTraceAPI::RegisterTraceDoneCB(*trace_client_id_, std::move(trace_done_callback));
    if (!register_trace_done_callback_result.has_value())
    {
        bmw::mw::log::LogError("lola")
            << "Lola TracingRuntime: RegisterTraceDoneCB with the GenericTraceAPI failed with error: "
            << register_trace_done_callback_result.error();
        return false;
    }
    return true;
}

impl::tracing::ITracingRuntimeBinding::TraceContextId TracingRuntime::RegisterServiceElement() noexcept
{
    if (current_service_element_idx_ >= type_erased_sample_ptrs_.size())
    {
        bmw::mw::log::LogFatal("lola")
            << "Could not register service element as the maximum number of service elements that can be registered ("
            << type_erased_sample_ptrs_.size() << ") has already been reached. Terminating.";
        std::terminate();
    }
    if (current_service_element_idx_ >= std::numeric_limits<TraceContextId>::max())
    {
        bmw::mw::log::LogFatal("lola")
            << "Could not register service element as the service element must be indexable by a "
               "TraceContextId. Terminating.";
        std::terminate();
    }

    const auto callback_idx = current_service_element_idx_;
    current_service_element_idx_++;
    return static_cast<TraceContextId>(callback_idx);
}

void TracingRuntime::RegisterShmObject(
    const impl::tracing::ServiceElementInstanceIdentifierView& service_element_instance_identifier_view,
    const analysis::tracing::ShmObjectHandle shm_object_handle,
    void* const shm_memory_start_address) noexcept
{
    AMP_ASSERT_PRD_MESSAGE(
        service_element_instance_identifier_view.service_element_identifier_view.service_element_type ==
            kDummyElementTypeForShmRegisterCallback,
        "Unexpected service_element_type in LoLa TracingRuntime::RegisterShmObject");
    AMP_ASSERT_PRD_MESSAGE(
        service_element_instance_identifier_view.service_element_identifier_view.service_element_name ==
            kDummyElementNameForShmRegisterCallback,
        "Unexpected service_element_name in LoLa TracingRuntime::RegisterShmObject");
    const auto map_value = std::make_pair(shm_object_handle, shm_memory_start_address);
    const auto insert_result =
        shm_object_handle_map_.insert(std::make_pair(service_element_instance_identifier_view, map_value));
    if (!insert_result.second)
    {
        bmw::mw::log::LogFatal("lola") << "Could not insert shm object handle" << shm_object_handle
                                       << "into map. Terminating.";
        std::terminate();
    }
}

void TracingRuntime::UnregisterShmObject(
    const impl::tracing::ServiceElementInstanceIdentifierView& service_element_instance_identifier_view) noexcept
{
    const auto erase_result = shm_object_handle_map_.erase(service_element_instance_identifier_view);
    if (erase_result == 0U)
    {
        bmw::mw::log::LogWarn("lola") << "UnregisterShmObject called on non-existing shared memory object. Ignoring.";
    }
}

void TracingRuntime::CacheFileDescriptorForReregisteringShmObject(
    const impl::tracing::ServiceElementInstanceIdentifierView& service_element_instance_identifier_view,
    const memory::shared::ISharedMemoryResource::FileDescriptor shm_file_descriptor,
    void* const shm_memory_start_address) noexcept
{
    const auto map_value = std::make_pair(shm_file_descriptor, shm_memory_start_address);
    const auto insert_result = failed_shm_object_registration_cache_.insert(
        std::make_pair(service_element_instance_identifier_view, map_value));
    if (!insert_result.second)
    {
        bmw::mw::log::LogFatal("lola") << "Could not insert file descriptor" << shm_file_descriptor
                                       << "for shm object which failed registration into map. Terminating.";
        std::terminate();
    }
}

amp::optional<std::pair<memory::shared::ISharedMemoryResource::FileDescriptor, void*>>
TracingRuntime::GetCachedFileDescriptorForReregisteringShmObject(
    const impl::tracing::ServiceElementInstanceIdentifierView& service_element_instance_identifier_view) const noexcept
{
    const auto find_result = failed_shm_object_registration_cache_.find(service_element_instance_identifier_view);
    if (find_result == failed_shm_object_registration_cache_.end())
    {
        return {};
    }
    return find_result->second;
}

void TracingRuntime::ClearCachedFileDescriptorForReregisteringShmObject(
    const impl::tracing::ServiceElementInstanceIdentifierView& service_element_instance_identifier_view) noexcept
{
    const auto erase_result = failed_shm_object_registration_cache_.erase(service_element_instance_identifier_view);
    if (erase_result == 0U)
    {
        bmw::mw::log::LogWarn("lola")
            << "ClearCachedFileDescriptorForReregisteringShmObject called on non-existing cached "
               "file descriptor. Ignoring.";
    }
}

analysis::tracing::ServiceInstanceElement TracingRuntime::ConvertToTracingServiceInstanceElement(
    const impl::tracing::ServiceElementInstanceIdentifierView service_element_instance_identifier_view) const
{
    const auto& service_instance_deployments = configuration_.GetServiceInstances();
    const auto& service_type_deployments = configuration_.GetServiceTypes();

    // @todo: Replace the configuration unordered_maps with maps and use CompareId?
    const auto instance_specifier =
        InstanceSpecifier::Create(service_element_instance_identifier_view.instance_specifier).value();
    const auto& service_instance_deployment = service_instance_deployments.at(instance_specifier);
    auto* lola_service_instance_deployment =
        amp::get_if<LolaServiceInstanceDeployment>(&service_instance_deployment.bindingInfo_);
    AMP_ASSERT_PRD(lola_service_instance_deployment != nullptr);

    const auto& service_identifier = service_instance_deployment.service_;

    const auto service_type_deployment = service_type_deployments.at(service_identifier);
    const auto* const lola_service_type_deployment =
        amp::get_if<LolaServiceTypeDeployment>(&service_type_deployment.binding_info_);
    AMP_ASSERT_PRD(lola_service_type_deployment != nullptr);

    using ServiceInstanceElement = analysis::tracing::ServiceInstanceElement;
    ServiceInstanceElement output_service_instance_element{};
    const auto service_element_type =
        service_element_instance_identifier_view.service_element_identifier_view.service_element_type;
    const auto service_element_name =
        service_element_instance_identifier_view.service_element_identifier_view.service_element_name;
    if (service_element_type == impl::tracing::ServiceElementType::EVENT)
    {
        const auto lola_event_id = lola_service_type_deployment->events_.at(
            std::string{service_element_name.data(), service_element_name.size()});
        output_service_instance_element.element_id_ = static_cast<ServiceInstanceElement::EventIdType>(lola_event_id);
    }
    else if (service_element_type == impl::tracing::ServiceElementType::FIELD)
    {
        const auto lola_field_id = lola_service_type_deployment->fields_.at(
            std::string{service_element_name.data(), service_element_name.size()});
        output_service_instance_element.element_id_ = static_cast<ServiceInstanceElement::FieldIdType>(lola_field_id);
    }
    else
    {
        bmw::mw::log::LogFatal("lola") << "Service element type: " << service_element_type
                                       << " is invalid. Terminating.";
        std::terminate();
    }

    output_service_instance_element.service_id_ =
        static_cast<ServiceInstanceElement::ServiceIdType>(lola_service_type_deployment->service_id_);

    if (!lola_service_instance_deployment->instance_id_.has_value())
    {
        bmw::mw::log::LogFatal("lola")
            << "Tracing should not be done on service element without configured instance ID. Terminating.";
        std::terminate();
    }
    output_service_instance_element.instance_id_ =
        static_cast<ServiceInstanceElement::InstanceIdType>(lola_service_instance_deployment->instance_id_.value().id_);

    const auto version = ServiceIdentifierTypeView{service_identifier}.GetVersion();
    output_service_instance_element.major_version_ = ServiceVersionTypeView{version}.getMajor();
    output_service_instance_element.minor_version_ = ServiceVersionTypeView{version}.getMinor();
    return output_service_instance_element;
}

amp::optional<analysis::tracing::ShmObjectHandle> TracingRuntime::GetShmObjectHandle(
    const impl::tracing::ServiceElementInstanceIdentifierView& service_element_instance_identifier_view) const noexcept
{
    impl::tracing::ServiceElementInstanceIdentifierView lolaBindingSpecificIdentifier =
        ConvertServiceElementInstanceIdentifierViewForLoLaShmIdentification(service_element_instance_identifier_view);

    const auto find_result = shm_object_handle_map_.find(lolaBindingSpecificIdentifier);
    if (find_result == shm_object_handle_map_.end())
    {
        return {};
    }
    return find_result->second.first;
}

amp::optional<void*> TracingRuntime::GetShmRegionStartAddress(
    const impl::tracing::ServiceElementInstanceIdentifierView& service_element_instance_identifier_view) const noexcept
{
    impl::tracing::ServiceElementInstanceIdentifierView simplifiedIdentifier =
        ConvertServiceElementInstanceIdentifierViewForLoLaShmIdentification(service_element_instance_identifier_view);

    const auto find_result = shm_object_handle_map_.find(simplifiedIdentifier);
    if (find_result == shm_object_handle_map_.end())
    {
        return {};
    }
    return find_result->second.second;
}

bool TracingRuntime::IsServiceElementTracingActive(const TraceContextId service_element_idx) noexcept
{
    auto& element = type_erased_sample_ptrs_.at(static_cast<std::size_t>(service_element_idx));
    std::lock_guard<std::mutex> lock(element.mutex);
    return element.sample_ptr.has_value();
}

void TracingRuntime::SetTypeErasedSamplePtr(impl::tracing::TypeErasedSamplePtr type_erased_sample_ptr,
                                            const TraceContextId service_element_idx) noexcept
{
    if (service_element_idx >= current_service_element_idx_)
    {
        bmw::mw::log::LogFatal("lola") << "Cannot set type erased sample pointer as provided service element index"
                                       << service_element_idx << "was never registered. Terminating.";
        std::terminate();
    }
    auto& element = type_erased_sample_ptrs_.at(static_cast<std::size_t>(service_element_idx));
    std::lock_guard<std::mutex> lock(element.mutex);
    AMP_ASSERT_PRD(!element.sample_ptr.has_value());
    element.sample_ptr = std::move(type_erased_sample_ptr);
}

void TracingRuntime::ClearTypeErasedSamplePtr(const TraceContextId service_element_idx) noexcept
{
    auto& element = type_erased_sample_ptrs_.at(static_cast<std::size_t>(service_element_idx));
    std::lock_guard<std::mutex> lock(element.mutex);
    element.sample_ptr = {};
}

}  // namespace tracing
}  // namespace lola
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
