// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#ifndef PLATFORM_AAS_MW_COM_IMPL_TRACING_TRACING_RUNTIME_H
#define PLATFORM_AAS_MW_COM_IMPL_TRACING_TRACING_RUNTIME_H

#include "platform/aas/mw/com/impl/binding_type.h"
#include "platform/aas/mw/com/impl/tracing/configuration/proxy_event_trace_point_type.h"
#include "platform/aas/mw/com/impl/tracing/configuration/proxy_field_trace_point_type.h"
#include "platform/aas/mw/com/impl/tracing/configuration/skeleton_event_trace_point_type.h"
#include "platform/aas/mw/com/impl/tracing/configuration/skeleton_field_trace_point_type.h"
#include "platform/aas/mw/com/impl/tracing/i_tracing_runtime.h"
#include "platform/aas/mw/com/impl/tracing/i_tracing_runtime_binding.h"

#include <amp_variant.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <unordered_map>

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

namespace detail_tracing_runtime
{

class TracingRuntimeAtomicState
{
  public:
    TracingRuntimeAtomicState() noexcept;
    TracingRuntimeAtomicState(const TracingRuntimeAtomicState&) = delete;
    TracingRuntimeAtomicState(TracingRuntimeAtomicState&& other) noexcept;
    ~TracingRuntimeAtomicState() noexcept = default;
    TracingRuntimeAtomicState& operator=(const TracingRuntimeAtomicState&) = delete;
    TracingRuntimeAtomicState& operator=(TracingRuntimeAtomicState&& other) noexcept;

    /// \brief consecutive trace-call failure counter, gets initialized to 0 on construction
    std::atomic_uint32_t consecutive_failure_counter;
    /// \brief flag, whether tracing is enabled. Gets initially set to true (we are only creating TracingRuntime in case
    ///        tracing is globally enabled and we have a valid filter config for it. But during runtime this flag can
    ///        be switched to false, because of tracing subsystem errors and then it stays with false for the rest of
    ///        the life-cycle.
    std::atomic_bool is_tracing_enabled;
};

}  // namespace detail_tracing_runtime

class TracingRuntime : public ITracingRuntime
{
    friend class TracingRuntimeAttorney;

  public:
    /// how many consecutive non-recoverable errors in trace-calls shall lead to disabling of tracing.
    /// @todo In the future we will make this value configurable via mw_com_config.json.
    constexpr static std::uint32_t MAX_CONSECUTIVE_ACCEPTABLE_TRACE_FAILURES{std::numeric_limits<uint32_t>::max()};

    explicit TracingRuntime(
        std::unordered_map<BindingType, ITracingRuntimeBinding*>&& tracing_runtime_bindings) noexcept;

    /// \brief TracingRuntime shall not be copyable/copy-assignable
    TracingRuntime(const TracingRuntime& other) = delete;
    TracingRuntime(TracingRuntime&& other) noexcept = default;
    ~TracingRuntime() noexcept = default;

    TracingRuntime& operator=(const TracingRuntime& other) = delete;
    TracingRuntime& operator=(TracingRuntime&& other) = default;

    void DisableTracing() noexcept override;

    impl::tracing::ITracingRuntimeBinding::TraceContextId RegisterServiceElement(
        const BindingType binding_type) noexcept override;

    void SetDataLossFlag(const BindingType binding_type) noexcept override;

    void RegisterShmObject(const BindingType binding_type,
                           const ServiceElementInstanceIdentifierView service_element_instance_identifier_view,
                           const memory::shared::ISharedMemoryResource::FileDescriptor shm_object_fd,
                           void* const shm_memory_start_address) noexcept override;

    void UnregisterShmObject(
        BindingType binding_type,
        ServiceElementInstanceIdentifierView service_element_instance_identifier_view) noexcept override;

    /// \brief Trace call for data residing in shared-memory being handled async via TraceDoneCallback. So this API
    ///        is only called by SkeletonEvents/Fields emitting data (send/update).
    /// \details The implementation builds up the AraComMetaInfo and ShmChunkList for the call to GenericTraceAPI::Trace
    ///          from the given arguments. I.e. based on the given service_element_instance_identifier it builds up the
    ///          AraComMetaInfo and based on shm_data_ptr (which is an absolute pointer), it finds
    ///          out, which shm-object is affected and builds up the ShmChunkList accordingly.
    ///          Since we currently do not support dynamic data types, the ShmChunkLists used by mw::com/LoLa only
    ///          consist of one ShmChunk! When we introduce support for dynamic data types, we may have to revisit this
    ///          interface.
    /// \param binding_type identification of binding, which does the call.
    /// \param trace_context_id trace context id for this trace call. The async TraceDoneCallBack will use this id to
    ///        notify, that tracing of the related data has been done.
    /// \param service_element_instance_identifier identification of service element and instance in which context the
    ///        call happens.
    /// \param trace_point_type type of trace point
    /// \param trace_point_data_id data identification of the data being traced (in LoLa this is the timestamp of an
    ///        event-slot.
    /// \param sample_ptr sample pointer of the event/field data to be traced. As long as it lives, it blocks the slot
    ///        from being reused. Should be destrucetd in the context of the trace done callback to free slot again.
    /// \param shm_data_ptr address of/pointer to the data residing in shm.
    /// \param shm_data_size size of the data, where shm_data_ptr points to
    /// \return blank in case of success, else either an error with code TraceErrorDisableAllTracePoints or
    ///         TraceErrorDisableTracePointInstance
    ResultBlank Trace(const BindingType binding_type,
                      const impl::tracing::ITracingRuntimeBinding::TraceContextId trace_context_id,
                      const ServiceElementInstanceIdentifierView service_element_instance_identifier,
                      const TracePointType trace_point_type,
                      const TracePointDataId trace_point_data_id,
                      TypeErasedSamplePtr sample_ptr,
                      const void* const shm_data_ptr,
                      const std::size_t shm_data_size) noexcept override;

    /// \brief Trace call for data residing locally (not in shared-memory) being synchronously copied for tracing.
    /// \param binding_type identification of binding, which does the call.
    /// \param service_element_instance_identifier identification of service element and instance in which context the
    ///        call happens.
    /// \param trace_point_type type of trace point
    /// \param trace_point_data_id optional data identification. Only trace_point_type, which deals with data reception
    ///        needs to set this id. (e.g. a ProxyEventTracePointType::GET_NEW_SAMPLES_CALLBACK would use it)
    /// \param local_data_ptr pointer to local data
    /// \param local_data_size size of the data, where local_data_ptr points to
    /// \return blank in case of success, else either an error with code TraceErrorDisableAllTracePoints or
    ///         TraceErrorDisableTracePointInstance
    ResultBlank Trace(const BindingType binding_type,
                      const ServiceElementInstanceIdentifierView service_element_instance_identifier,
                      const TracePointType trace_point_type,
                      const amp::optional<TracePointDataId> trace_point_data_id,
                      const void* const local_data_ptr,
                      const std::size_t local_data_size) noexcept override;

  private:
    detail_tracing_runtime::TracingRuntimeAtomicState atomic_state_;

    /// \brief Updates internal state, whether to e.g. to disable tracing, because of non-recoverable trace error or
    ///        to many consecutive recoverable errors. Will be called after each call to Trace with the given result.
    /// \param trace_call_result result of last call to Generic Trace API trace method.
    /// \return Result to be forwarded to the caller of Trace()
    ResultBlank ProcessTraceCallResult(const ServiceElementInstanceIdentifierView& service_element_instance_identifier,
                                       const analysis::tracing::TraceResult& trace_call_result,
                                       ITracingRuntimeBinding& tracing_runtime_binding) noexcept;
    ITracingRuntimeBinding& GetTracingRuntimeBinding(const BindingType binding_type) const noexcept;

    std::unordered_map<BindingType, ITracingRuntimeBinding*> tracing_runtime_bindings_;
};

}  // namespace tracing
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_TRACING_TRACING_RUNTIME_H
