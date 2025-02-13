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


#include "platform/aas/mw/com/impl/tracing/tracing_runtime.h"

#include "platform/aas/analysis/tracing/common/types.h"
#include "platform/aas/analysis/tracing/library/generic_trace_api/ara_com_meta_info.h"
#include "platform/aas/analysis/tracing/library/generic_trace_api/error_code/error_code.h"
#include "platform/aas/analysis/tracing/library/generic_trace_api/generic_trace_api.h"
#include "platform/aas/lib/memory/shared/pointer_arithmetic_util.h"
#include "platform/aas/mw/com/impl/tracing/trace_error.h"

#include <amp_assert.hpp>
#include <amp_overload.hpp>

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

namespace
{

bool IsTerminalFatalError(const bmw::result::Error error)
{
    using TracingErrorCodeType = std::underlying_type<analysis::tracing::ErrorCode>::type;

    return (*error == static_cast<TracingErrorCodeType>(analysis::tracing::ErrorCode::kTerminalFatal));
}

bool IsNonRecoverableError(const bmw::result::Error error)
{
    using ErrorCode = analysis::tracing::ErrorCode;
    using TracingErrorCodeType = std::underlying_type<ErrorCode>::type;

    if ((*error == static_cast<TracingErrorCodeType>(ErrorCode::kNoErrorRecoverable)) ||
        (*error == static_cast<TracingErrorCodeType>(ErrorCode::kNotEnoughMemoryRecoverable)) ||
        (*error == static_cast<TracingErrorCodeType>(ErrorCode::kModuleNotInitializedRecoverable)) ||
        (*error == static_cast<TracingErrorCodeType>(ErrorCode::kModuleInitializedRecoverable)) ||
        (*error == static_cast<TracingErrorCodeType>(ErrorCode::kRingBufferFullRecoverable)) ||
        (*error == static_cast<TracingErrorCodeType>(ErrorCode::kRingBufferEmptyRecoverable)) ||
        (*error == static_cast<TracingErrorCodeType>(ErrorCode::kRingBufferNotInitializedRecoverable)) ||
        (*error == static_cast<TracingErrorCodeType>(ErrorCode::kRingBufferInitializedRecoverable)) ||
        (*error == static_cast<TracingErrorCodeType>(ErrorCode::kRingBufferMaxRetriesRecoverable)) ||
        (*error == static_cast<TracingErrorCodeType>(ErrorCode::kRingBufferInvalidStateRecoverable)) ||
        (*error == static_cast<TracingErrorCodeType>(ErrorCode::kRingBufferTooLargeRecoverable)) ||
        (*error == static_cast<TracingErrorCodeType>(ErrorCode::kRingBufferInvalidMemoryResourceRecoverable)) ||
        (*error == static_cast<TracingErrorCodeType>(ErrorCode::kCallbackAlreadyRegisteredRecoverable)) ||
        (*error == static_cast<TracingErrorCodeType>(ErrorCode::kMessageSendFailedRecoverable)) ||
        (*error == static_cast<TracingErrorCodeType>(ErrorCode::kWrongMessageIdRecoverable)) ||
        (*error == static_cast<TracingErrorCodeType>(ErrorCode::kWrongClientIdRecoverable)) ||
        (*error == static_cast<TracingErrorCodeType>(ErrorCode::kDispatchDestroyFailedRecoverable)) ||
        (*error == static_cast<TracingErrorCodeType>(ErrorCode::kWrongHandleRecoverable)) ||
        (*error == static_cast<TracingErrorCodeType>(ErrorCode::kLastRecoverable)) ||
        (*error == static_cast<TracingErrorCodeType>(ErrorCode::kClientNotFoundRecoverable)) ||
        (*error == static_cast<TracingErrorCodeType>(ErrorCode::kGenericErrorRecoverable)))
    {
        return false;
    }
    else if ((*error == static_cast<TracingErrorCodeType>(ErrorCode::kDaemonNotConnectedFatal)) ||
             (*error == static_cast<TracingErrorCodeType>(ErrorCode::kInvalidArgumentFatal)) ||
             (*error == static_cast<TracingErrorCodeType>(ErrorCode::kDaemonConnectionFailedFatal)) ||
             (*error == static_cast<TracingErrorCodeType>(ErrorCode::kServerConnectionNameOpenFailedFatal)) ||
             (*error == static_cast<TracingErrorCodeType>(ErrorCode::kNoDeallocatorCallbackRegisteredFatal)) ||
             (*error == static_cast<TracingErrorCodeType>(ErrorCode::kSharedMemoryObjectRegistrationFailedFatal)) ||
             (*error == static_cast<TracingErrorCodeType>(ErrorCode::kSharedMemoryObjectUnregisterFailedFatal)) ||
             (*error == static_cast<TracingErrorCodeType>(ErrorCode::kSharedMemoryObjectHandleCreationFailedFatal)) ||
             (*error == static_cast<TracingErrorCodeType>(ErrorCode::kSharedMemoryObjectHandleDeletionFailedFatal)) ||
             (*error == static_cast<TracingErrorCodeType>(ErrorCode::kBadFileDescriptorFatal)) ||
             (*error == static_cast<TracingErrorCodeType>(ErrorCode::kChannelCreationFailedFatal)) ||
             (*error == static_cast<TracingErrorCodeType>(ErrorCode::kNameAttachFailedFatal)) ||
             (*error == static_cast<TracingErrorCodeType>(ErrorCode::kNameDetachFailedFatal)) ||
             (*error == static_cast<TracingErrorCodeType>(ErrorCode::kInvalidAppInstanceIdFatal)) ||
             (*error == static_cast<TracingErrorCodeType>(ErrorCode::kInvalidBindingTypeFatal)) ||
             (*error == static_cast<TracingErrorCodeType>(ErrorCode::kTerminalFatal)))
    {
        return true;
    }
    else
    {
        bmw::mw::log::LogError("lola") << "TracingRuntime: Unexpected analysis::tracing::ErrorCode: " << *error;
        std::terminate();
    }
}

const std::unordered_map<ProxyEventTracePointType, analysis::tracing::TracePointType>
    kProxyEventTracePointToTracingTracePointMap{
        {ProxyEventTracePointType::SUBSCRIBE, analysis::tracing::TracePointType::PROXY_EVENT_SUB},
        {ProxyEventTracePointType::UNSUBSCRIBE, analysis::tracing::TracePointType::PROXY_EVENT_UNSUB},
        {ProxyEventTracePointType::SUBSCRIBE_STATE_CHANGE,
         analysis::tracing::TracePointType::PROXY_EVENT_SUBSTATE_CHANGE},
        {ProxyEventTracePointType::SET_SUBSCRIPTION_STATE_CHANGE_HANDLER,
         analysis::tracing::TracePointType::PROXY_EVENT_SET_CHGHDL},
        {ProxyEventTracePointType::UNSET_SUBSCRIPTION_STATE_CHANGE_HANDLER,
         analysis::tracing::TracePointType::PROXY_EVENT_UNSET_CHGHDL},
        {ProxyEventTracePointType::SUBSCRIPTION_STATE_CHANGE_HANDLER_CALLBACK,
         analysis::tracing::TracePointType::PROXY_EVENT_CHGHDL},
        {ProxyEventTracePointType::SET_RECEIVE_HANDLER, analysis::tracing::TracePointType::PROXY_EVENT_SET_RECHDL},
        {ProxyEventTracePointType::UNSET_RECEIVE_HANDLER, analysis::tracing::TracePointType::PROXY_EVENT_UNSET_RECHDL},
        {ProxyEventTracePointType::RECEIVE_HANDLER_CALLBACK, analysis::tracing::TracePointType::PROXY_EVENT_RECHDL},
        {ProxyEventTracePointType::GET_NEW_SAMPLES, analysis::tracing::TracePointType::PROXY_EVENT_GET_SAMPLES},
        {ProxyEventTracePointType::GET_NEW_SAMPLES_CALLBACK, analysis::tracing::TracePointType::PROXY_EVENT_SAMPLE_CB},
    };

const std::unordered_map<ProxyFieldTracePointType, analysis::tracing::TracePointType>
    kProxyFieldTracePointToTracingTracePointMap{
        {ProxyFieldTracePointType::SUBSCRIBE, analysis::tracing::TracePointType::PROXY_FIELD_SUB},
        {ProxyFieldTracePointType::UNSUBSCRIBE, analysis::tracing::TracePointType::PROXY_FIELD_UNSUB},
        {ProxyFieldTracePointType::SUBSCRIBE_STATE_CHANGE,
         analysis::tracing::TracePointType::PROXY_FIELD_SUBSTATE_CHANGE},
        {ProxyFieldTracePointType::SET_SUBSCRIPTION_STATE_CHANGE_HANDLER,
         analysis::tracing::TracePointType::PROXY_FIELD_SET_CHGHDL},
        {ProxyFieldTracePointType::UNSET_SUBSCRIPTION_STATE_CHANGE_HANDLER,
         analysis::tracing::TracePointType::PROXY_FIELD_UNSET_CHGHDL},
        {ProxyFieldTracePointType::SUBSCRIPTION_STATE_CHANGE_HANDLER_CALLBACK,
         analysis::tracing::TracePointType::PROXY_FIELD_CHGHDL},
        {ProxyFieldTracePointType::SET_RECEIVE_HANDLER, analysis::tracing::TracePointType::PROXY_FIELD_SET_RECHDL},
        {ProxyFieldTracePointType::UNSET_RECEIVE_HANDLER, analysis::tracing::TracePointType::PROXY_FIELD_UNSET_RECHDL},
        {ProxyFieldTracePointType::RECEIVE_HANDLER_CALLBACK, analysis::tracing::TracePointType::PROXY_FIELD_RECHDL},
        {ProxyFieldTracePointType::GET_NEW_SAMPLES, analysis::tracing::TracePointType::PROXY_FIELD_GET_SAMPLES},
        {ProxyFieldTracePointType::GET_NEW_SAMPLES_CALLBACK, analysis::tracing::TracePointType::PROXY_FIELD_SAMPLE_CB},
        {ProxyFieldTracePointType::GET, analysis::tracing::TracePointType::PROXY_FIELD_GET},
        {ProxyFieldTracePointType::GET_RESULT, analysis::tracing::TracePointType::PROXY_FIELD_GET_RESULT},
        {ProxyFieldTracePointType::SET, analysis::tracing::TracePointType::PROXY_FIELD_SET},
        {ProxyFieldTracePointType::SET_RESULT, analysis::tracing::TracePointType::PROXY_FIELD_SET_RESULT},
    };

const std::unordered_map<SkeletonEventTracePointType, analysis::tracing::TracePointType>
    kSkeletonEventTracePointToTracingTracePointMap{
        {SkeletonEventTracePointType::SEND, analysis::tracing::TracePointType::SKEL_EVENT_SND},
        {SkeletonEventTracePointType::SEND_WITH_ALLOCATE, analysis::tracing::TracePointType::SKEL_EVENT_SND_A},
    };

const std::unordered_map<SkeletonFieldTracePointType, analysis::tracing::TracePointType>
    kSkeletonFieldTracePointToTracingTracePointMap{
        {SkeletonFieldTracePointType::UPDATE, analysis::tracing::TracePointType::SKEL_FIELD_UPD},
        {SkeletonFieldTracePointType::UPDATE_WITH_ALLOCATE, analysis::tracing::TracePointType::SKEL_FIELD_UPD_A},
        {SkeletonFieldTracePointType::GET_CALL, analysis::tracing::TracePointType::SKEL_FIELD_GET_CALL},
        {SkeletonFieldTracePointType::GET_CALL_RESULT, analysis::tracing::TracePointType::SKEL_FIELD_GET_CALL_RESULT},
        {SkeletonFieldTracePointType::SET_CALL, analysis::tracing::TracePointType::SKEL_FIELD_SET_CALL},
        {SkeletonFieldTracePointType::SET_CALL_RESULT, analysis::tracing::TracePointType::SKEL_FIELD_SET_CALL_RESULT},
    };

analysis::tracing::TracePointType InternalToExternalTracePointType(
    const TracingRuntime::TracePointType& internal_trace_point_type) noexcept
{
    auto visitor = amp::overload(
        [](const ProxyEventTracePointType& proxy_event_trace_point) -> analysis::tracing::TracePointType {
            if (proxy_event_trace_point == ProxyEventTracePointType::INVALID)
            {
                bmw::mw::log::LogFatal("lola") << "TracingRuntime: Unexpected ProxyEventTracePointType!";
                std::terminate();
            }
            return kProxyEventTracePointToTracingTracePointMap.at(proxy_event_trace_point);
        },
        [](const ProxyFieldTracePointType& proxy_field_trace_point) -> analysis::tracing::TracePointType {
            if (proxy_field_trace_point == ProxyFieldTracePointType::INVALID)
            {
                bmw::mw::log::LogFatal("lola") << "TracingRuntime: Unexpected ProxyFieldTracePointType!";
                std::terminate();
            }
            return kProxyFieldTracePointToTracingTracePointMap.at(proxy_field_trace_point);
        },
        [](const SkeletonEventTracePointType& skeleton_event_trace_point) -> analysis::tracing::TracePointType {
            if (skeleton_event_trace_point == SkeletonEventTracePointType::INVALID)
            {
                bmw::mw::log::LogFatal("lola") << "TracingRuntime: Unexpected SkeletonEventTracePointType!";
                std::terminate();
            }
            return kSkeletonEventTracePointToTracingTracePointMap.at(skeleton_event_trace_point);
        },
        [](const SkeletonFieldTracePointType& skeleton_field_trace_point) -> analysis::tracing::TracePointType {
            if (skeleton_field_trace_point == SkeletonFieldTracePointType::INVALID)
            {
                bmw::mw::log::LogFatal("lola") << "TracingRuntime: Unexpected SkeletonFieldTracePointType!";
                std::terminate();
            }
            return kSkeletonFieldTracePointToTracingTracePointMap.at(skeleton_field_trace_point);
        });
    return amp::visit(visitor, internal_trace_point_type);
}

analysis::tracing::AraComMetaInfo CreateMetaInfo(
    const ServiceElementInstanceIdentifierView& service_element_instance_identifier,
    const TracingRuntime::TracePointType& trace_point_type,
    const amp::optional<TracingRuntime::TracePointDataId> trace_point_data_id,
    const ITracingRuntimeBinding& runtime_binding)
{
    const analysis::tracing::TracePointType ext_trace_point_type = InternalToExternalTracePointType(trace_point_type);
    analysis::tracing::AraComMetaInfo result{analysis::tracing::AraComProperties(
        ext_trace_point_type,
        runtime_binding.ConvertToTracingServiceInstanceElement(service_element_instance_identifier),
        trace_point_data_id)};
    if (runtime_binding.GetDataLossFlag())
    {
        result.SetDataLossBit();
    }
    return result;
}

}  // namespace

namespace detail_tracing_runtime
{

TracingRuntimeAtomicState::TracingRuntimeAtomicState() noexcept
    : consecutive_failure_counter{0}, is_tracing_enabled{true}
{
}

TracingRuntimeAtomicState::TracingRuntimeAtomicState(TracingRuntimeAtomicState&& other) noexcept
    : consecutive_failure_counter{other.consecutive_failure_counter.load()},
      is_tracing_enabled{other.is_tracing_enabled.load()}
{
}

TracingRuntimeAtomicState& TracingRuntimeAtomicState::operator=(TracingRuntimeAtomicState&& other) noexcept
{
    consecutive_failure_counter = other.consecutive_failure_counter.load();
    is_tracing_enabled = other.is_tracing_enabled.load();
    return *this;
}

}  // namespace detail_tracing_runtime

void TracingRuntime::DisableTracing() noexcept
{
    bmw::mw::log::LogWarn("lola") << "TracingRuntime: Disabling Tracing due to call to DisableTracing.";
    atomic_state_.is_tracing_enabled = false;
}

impl::tracing::ITracingRuntimeBinding::TraceContextId TracingRuntime::RegisterServiceElement(
    const BindingType binding_type) noexcept
{
    auto& runtime_binding = GetTracingRuntimeBinding(binding_type);
    return runtime_binding.RegisterServiceElement();
}

ResultBlank TracingRuntime::ProcessTraceCallResult(
    const ServiceElementInstanceIdentifierView& service_element_instance_identifier,
    const analysis::tracing::TraceResult& trace_call_result,
    ITracingRuntimeBinding& tracing_runtime_binding) noexcept
{
    if (trace_call_result.has_value())
    {
        tracing_runtime_binding.SetDataLossFlag(false);
        atomic_state_.consecutive_failure_counter = 0;
        return {};
    }

    if (IsTerminalFatalError(trace_call_result.error()))
    {
        bmw::mw::log::LogWarn("lola") << "TracingRuntime: Disabling Tracing because of kTerminalFatal Error: "
                                      << trace_call_result.error();
        atomic_state_.is_tracing_enabled = false;
        return MakeUnexpected(TraceErrorCode::TraceErrorDisableAllTracePoints);
    }
    atomic_state_.consecutive_failure_counter++;
    tracing_runtime_binding.SetDataLossFlag(true);
    if (atomic_state_.consecutive_failure_counter >= MAX_CONSECUTIVE_ACCEPTABLE_TRACE_FAILURES)
    {
        bmw::mw::log::LogWarn("lola") << "TracingRuntime: Disabling Tracing because of max number of consecutive "
                                         "errors during call of Trace has been reached.";
        atomic_state_.is_tracing_enabled = false;
        return MakeUnexpected(TraceErrorCode::TraceErrorDisableAllTracePoints);
    }
    if (IsNonRecoverableError(trace_call_result.error()))
    {
        bmw::mw::log::LogWarn("lola") << "TracingRuntime: Disabling Tracing for " << service_element_instance_identifier
                                      << " because of non-recoverable error during call of Trace(). Error: "
                                      << trace_call_result.error();
        return MakeUnexpected(TraceErrorCode::TraceErrorDisableTracePointInstance);
    }
    return {};
}

ITracingRuntimeBinding& TracingRuntime::GetTracingRuntimeBinding(const BindingType binding_type) const noexcept
{
    const auto& search = tracing_runtime_bindings_.find(binding_type);
    AMP_ASSERT_PRD_MESSAGE(search != tracing_runtime_bindings_.cend(), "No tracing runtime for given BindingType!");
    return *search->second;
}

TracingRuntime::TracingRuntime(
    std::unordered_map<BindingType, ITracingRuntimeBinding*>&& tracing_runtime_bindings) noexcept
    : ITracingRuntime{}, tracing_runtime_bindings_{std::move(tracing_runtime_bindings)}
{
    for (auto tracing_runtime_binding : tracing_runtime_bindings_)
    {
        bool success = tracing_runtime_binding.second->RegisterWithGenericTraceApi();
        if (!success)
        {
            bmw::mw::log::LogError("lola")
                << "TracingRuntime: Registration as Client with the GenericTraceAPI failed for binding "
                << static_cast<std::underlying_type<BindingType>::type>(tracing_runtime_binding.first)
                << ". Disable Tracing!";
            // 2 -> disable tracing.
            atomic_state_.is_tracing_enabled = false;
        }
    }
}

void TracingRuntime::SetDataLossFlag(const BindingType binding_type) noexcept
{
    if (!atomic_state_.is_tracing_enabled)
    {
        return;
    }
    GetTracingRuntimeBinding(binding_type).SetDataLossFlag(true);
}

void TracingRuntime::RegisterShmObject(
    const BindingType binding_type,
    const ServiceElementInstanceIdentifierView service_element_instance_identifier_view,
    const memory::shared::ISharedMemoryResource::FileDescriptor shm_object_fd,
    void* const shm_memory_start_address) noexcept
{
    if (!atomic_state_.is_tracing_enabled)
    {
        return;
    }
    auto& runtime_binding = GetTracingRuntimeBinding(binding_type);
    const auto generic_trace_api_shm_handle =
        analysis::tracing::GenericTraceAPI::RegisterShmObject(runtime_binding.GetTraceClientId(), shm_object_fd);

    if (generic_trace_api_shm_handle.has_value())
    {
        runtime_binding.RegisterShmObject(
            service_element_instance_identifier_view, generic_trace_api_shm_handle.value(), shm_memory_start_address);
    }
    else
    {
        if (IsNonRecoverableError(generic_trace_api_shm_handle.error()))
        {
            if (IsTerminalFatalError(generic_trace_api_shm_handle.error()))
            {
                bmw::mw::log::LogWarn("lola") << "TracingRuntime: Disabling Tracing because of kTerminalFatal Error: "
                                              << generic_trace_api_shm_handle.error();
                atomic_state_.is_tracing_enabled = false;
            }
            else
            {
                bmw::mw::log::LogWarn("lola")
                    << "TracingRuntime: Non-recoverable error during call of RegisterShmObject() for "
                       "ServiceElementInstanceIdentifierView: "
                    << service_element_instance_identifier_view
                    << ". Related ShmObject will not be registered any any related Trace() calls will "
                       "be suppressed. Error: "
                    << generic_trace_api_shm_handle.error();
            }
        }
        else
        {
            bmw::mw::log::LogInfo("lola")
                << "TracingRuntime::RegisterShmObject: Registration of ShmObject for ServiceElementInstanceIdentifier "
                << service_element_instance_identifier_view
                << " failed with recoverable error: " << generic_trace_api_shm_handle.error()
                << ". Will retry once on next Trace call referring to this ShmObject.";
            runtime_binding.CacheFileDescriptorForReregisteringShmObject(
                service_element_instance_identifier_view, shm_object_fd, shm_memory_start_address);
        }
    }
}

void TracingRuntime::UnregisterShmObject(
    BindingType binding_type,
    ServiceElementInstanceIdentifierView service_element_instance_identifier_view) noexcept
{
    if (!atomic_state_.is_tracing_enabled)
    {
        return;
    }
    auto& runtime_binding = GetTracingRuntimeBinding(binding_type);
    const auto shm_object_handle_result = runtime_binding.GetShmObjectHandle(service_element_instance_identifier_view);
    if (!shm_object_handle_result.has_value())
    {
        // This shm-object was never successfully registered. Call is ok, since the upper-layer/skeleton doesn't
        // book-keep it. Still clear any eventually cached fd!
        runtime_binding.ClearCachedFileDescriptorForReregisteringShmObject(service_element_instance_identifier_view);
        return;
    }
    runtime_binding.UnregisterShmObject(service_element_instance_identifier_view);

    const auto generic_trace_api_unregister_shm_result = analysis::tracing::GenericTraceAPI::UnregisterShmObject(
        runtime_binding.GetTraceClientId(), shm_object_handle_result.value());
    if (!generic_trace_api_unregister_shm_result.has_value())
    {
        if (IsNonRecoverableError(generic_trace_api_unregister_shm_result.error()))
        {
            if (IsTerminalFatalError(generic_trace_api_unregister_shm_result.error()))
            {
                bmw::mw::log::LogWarn("lola")
                    << "TracingRuntime::UnregisterShmObject: Disabling Tracing because of kTerminalFatal Error: "
                    << generic_trace_api_unregister_shm_result.error();
                atomic_state_.is_tracing_enabled = false;
            }
            else
            {
                bmw::mw::log::LogWarn("lola")
                    << "TracingRuntime::UnregisterShmObject: Non-recoverable error during call for "
                       "ServiceElementInstanceIdentifierView: "
                    << service_element_instance_identifier_view
                    << ". Error: " << generic_trace_api_unregister_shm_result.error();
            }
        }
        else
        {
            bmw::mw::log::LogInfo("lola")
                << "TracingRuntime::UnregisterShmObject: Unregistering ShmObject for ServiceElementInstanceIdentifier "
                << service_element_instance_identifier_view
                << " failed with recoverable error: " << generic_trace_api_unregister_shm_result.error() << ".";
        }
    }
}

ResultBlank TracingRuntime::Trace(const BindingType binding_type,
                                  const impl::tracing::ITracingRuntimeBinding::TraceContextId trace_context_id,
                                  const ServiceElementInstanceIdentifierView service_element_instance_identifier,
                                  const TracePointType trace_point_type,
                                  const TracePointDataId trace_point_data_id,
                                  TypeErasedSamplePtr sample_ptr,
                                  const void* const shm_data_ptr,
                                  const std::size_t shm_data_size) noexcept
{
    if (!atomic_state_.is_tracing_enabled)
    {
        return MakeUnexpected(TraceErrorCode::TraceErrorDisableAllTracePoints);
    }
    auto& runtime_binding = GetTracingRuntimeBinding(binding_type);

    if (runtime_binding.IsServiceElementTracingActive(trace_context_id))
    {
        runtime_binding.SetDataLossFlag(true);
        return {};
    }

    auto shm_object_handle = runtime_binding.GetShmObjectHandle(service_element_instance_identifier);
    if (!shm_object_handle.has_value())
    {
        auto cached_file_descriptor =
            runtime_binding.GetCachedFileDescriptorForReregisteringShmObject(service_element_instance_identifier);

        if (!cached_file_descriptor.has_value())
        {
            // We also have no cached file descriptor for the shm-object! This means this shm-object/the trace call
            // related to it shall be ignored
            return MakeUnexpected(TraceErrorCode::TraceErrorDisableTracePointInstance);
        }

        // Try to re-register with cached_file_descriptor
        const memory::shared::ISharedMemoryResource::FileDescriptor shm_object_fd =
            cached_file_descriptor.value().first;
        void* const shm_memory_start_address = cached_file_descriptor.value().second;
        const auto register_shm_result =
            analysis::tracing::GenericTraceAPI::RegisterShmObject(runtime_binding.GetTraceClientId(), shm_object_fd);
        if (!register_shm_result.has_value())
        {
            if (IsTerminalFatalError(register_shm_result.error()))
            {
                bmw::mw::log::LogWarn("lola") << "TracingRuntime: Disabling Tracing because of kTerminalFatal Error: "
                                              << register_shm_result.error();
                atomic_state_.is_tracing_enabled = false;
                return MakeUnexpected(TraceErrorCode::TraceErrorDisableAllTracePoints);
            }
            // register failed, we won't try any further
            runtime_binding.ClearCachedFileDescriptorForReregisteringShmObject(service_element_instance_identifier);
            bmw::mw::log::LogError("lola")
                << "TracingRuntime::Trace: Re-registration of ShmObject for ServiceElementInstanceIdentifier "
                << service_element_instance_identifier
                << " failed. Any Trace-Call related to this ShmObject will now be ignored!";
            // we didn't get a shm-object-handle (no valid registration) and even re-registration
            // failed, so we skip this TRACE call according to  2, which requires only on register-retry.
            return MakeUnexpected(TraceErrorCode::TraceErrorDisableTracePointInstance);
        }
        shm_object_handle = register_shm_result.value();
        // we re-registered successfully at GenericTraceAPI -> now also register to the binding specific runtime.
        runtime_binding.RegisterShmObject(
            service_element_instance_identifier, shm_object_handle.value(), shm_memory_start_address);
    }

    const auto shm_region_start = runtime_binding.GetShmRegionStartAddress(service_element_instance_identifier);
    // a valid shm_object_handle ... a shm_region_start should also exist!
    AMP_ASSERT_PRD_MESSAGE(shm_region_start.has_value(),
                           "No shared-memory-region start address for shm-object in tracing runtime binding!");

    const auto meta_info =
        CreateMetaInfo(service_element_instance_identifier, trace_point_type, trace_point_data_id, runtime_binding);

    // Create ShmChunkList
    analysis::tracing::SharedMemoryLocation root_chunk_memory_location{
        shm_object_handle.value(),
        static_cast<size_t>(memory::shared::SubtractPointers(shm_data_ptr, shm_region_start.value()))};
    analysis::tracing::SharedMemoryChunk root_chunk{root_chunk_memory_location, shm_data_size};
    analysis::tracing::ShmDataChunkList chunk_list{root_chunk};

    runtime_binding.SetTypeErasedSamplePtr(std::move(sample_ptr), trace_context_id);
    const auto trace_result = analysis::tracing::GenericTraceAPI::Trace(
        runtime_binding.GetTraceClientId(), meta_info, chunk_list, trace_context_id);
    if (!trace_result.has_value())
    {
        runtime_binding.ClearTypeErasedSamplePtr(trace_context_id);
    }
    return ProcessTraceCallResult(service_element_instance_identifier, trace_result, runtime_binding);
}

ResultBlank TracingRuntime::Trace(const BindingType binding_type,
                                  const ServiceElementInstanceIdentifierView service_element_instance_identifier,
                                  const TracePointType trace_point_type,
                                  const amp::optional<TracePointDataId> trace_point_data_id,
                                  const void* const local_data_ptr,
                                  const std::size_t local_data_size) noexcept
{
    if (!atomic_state_.is_tracing_enabled)
    {
        return MakeUnexpected(TraceErrorCode::TraceErrorDisableAllTracePoints);
    }
    auto& runtime_binding = GetTracingRuntimeBinding(binding_type);
    const auto meta_info =
        CreateMetaInfo(service_element_instance_identifier, trace_point_type, trace_point_data_id, runtime_binding);

    // Create LocalChunkList
    const analysis::tracing::LocalDataChunk root_chunk{local_data_ptr, local_data_size};
    analysis::tracing::LocalDataChunkList chunk_list{root_chunk};

    const auto trace_result =
        analysis::tracing::GenericTraceAPI::Trace(runtime_binding.GetTraceClientId(), meta_info, chunk_list);
    return ProcessTraceCallResult(service_element_instance_identifier, trace_result, runtime_binding);
}

}  // namespace tracing
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
