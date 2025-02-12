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


#ifndef PLATFORM_AAS_MW_COM_IMPL_SKELETON_EVENT_TRACING_H
#define PLATFORM_AAS_MW_COM_IMPL_SKELETON_EVENT_TRACING_H

#include "platform/aas/mw/com/impl/binding_type.h"
#include "platform/aas/mw/com/impl/bindings/lola/event_data_control.h"
#include "platform/aas/mw/com/impl/bindings/lola/sample_allocatee_ptr.h"
#include "platform/aas/mw/com/impl/bindings/lola/sample_ptr.h"
#include "platform/aas/mw/com/impl/bindings/lola/transaction_log_set.h"
#include "platform/aas/mw/com/impl/bindings/mock_binding/sample_ptr.h"
#include "platform/aas/mw/com/impl/instance_identifier.h"
#include "platform/aas/mw/com/impl/plumbing/sample_allocatee_ptr.h"
#include "platform/aas/mw/com/impl/skeleton_event_binding.h"
#include "platform/aas/mw/com/impl/tracing/common_event_tracing.h"
#include "platform/aas/mw/com/impl/tracing/configuration/service_element_instance_identifier_view.h"
#include "platform/aas/mw/com/impl/tracing/skeleton_event_tracing_data.h"

#include <amp_optional.hpp>
#include <amp_string_view.hpp>

#include <cstdint>

namespace bmw::mw::com::impl::tracing
{

namespace detail_skeleton_event_tracing
{

struct TracingData
{
    impl::tracing::ITracingRuntime::TracePointDataId trace_point_data_id;
    const std::pair<const void*, std::size_t> shm_data_chunk;
};

template <typename SampleType>
TracingData ExtractBindingTracingData(const impl::SampleAllocateePtr<SampleType>& sample_data_ptr) noexcept
{
    const auto& binding_ptr_variant = SampleAllocateePtrView{sample_data_ptr}.GetUnderlyingVariant();
    auto visitor = amp::overload(
        [](const lola::SampleAllocateePtr<SampleType>& lola_ptr) -> TracingData {
            const auto& event_data_control_composite =
                lola::SampleAllocateePtrView{lola_ptr}.GetEventDataControlComposite();
            AMP_ASSERT_PRD(event_data_control_composite.has_value());
            const auto referenced_slot = lola_ptr.GetReferencedSlot();
            const auto sample_timestamp = event_data_control_composite->GetEventSlotTimestamp(referenced_slot);
            static_assert(
                sizeof(lola::EventSlotStatus::EventTimeStamp) ==
                    sizeof(impl::tracing::ITracingRuntime::TracePointDataId),
                "Event timestamp is used for the trace point data id, therefore, the types should be the same.");

            const auto trace_point_data_id =
                static_cast<impl::tracing::ITracingRuntime::TracePointDataId>(sample_timestamp);

            return {trace_point_data_id, {lola_ptr.get(), sizeof(SampleType)}};
        },
        [](const std::unique_ptr<SampleType>& ptr) -> TracingData {
            return {0U, {ptr.get(), sizeof(SampleType)}};
        },
        [](const amp::blank&) -> TracingData { std::terminate(); });
    return amp::visit(visitor, binding_ptr_variant);
}

template <typename SampleType>
amp::optional<TypeErasedSamplePtr> CreateTypeErasedSamplePtr(
    impl::SampleAllocateePtr<SampleType>& sample_data_ptr) noexcept
{
    auto& binding_ptr_variant = SampleAllocateePtrMutableView{sample_data_ptr}.GetUnderlyingVariant();
    auto visitor = amp::overload(
        [](lola::SampleAllocateePtr<SampleType>& lola_ptr) -> amp::optional<TypeErasedSamplePtr> {
            auto& event_data_control_composite_result =
                lola::SampleAllocateePtrMutableView{lola_ptr}.GetEventDataControlComposite();
            AMP_ASSERT_PRD(event_data_control_composite_result.has_value());

            auto& event_data_control = event_data_control_composite_result->GetQmEventDataControl();

            const auto event_slot_index = lola_ptr.GetReferencedSlot();
            const auto was_event_referenced = event_data_control.ReferenceSpecificEvent(
                event_slot_index, lola::TransactionLogSet::kSkeletonIndexSentinel);
            if (!was_event_referenced)
            {
                return {};
            }
            const auto* const managed_object = lola::SampleAllocateePtrView{lola_ptr}.GetManagedObject();
            const lola::EventSlotStatus event_slot_status{event_data_control[event_slot_index]};

            lola::SamplePtr<SampleType> sample_ptr{
                managed_object, event_data_control, event_slot_index, lola::TransactionLogSet::kSkeletonIndexSentinel};
            return impl::tracing::TypeErasedSamplePtr{std::move(sample_ptr)};
        },
        [](std::unique_ptr<SampleType>& ptr) -> amp::optional<TypeErasedSamplePtr> {
            mock_binding::SamplePtr<SampleType> sample_ptr = std::make_unique<SampleType>(*ptr);
            impl::tracing::TypeErasedSamplePtr type_erased_sample_ptr{std::move(sample_ptr)};
            return type_erased_sample_ptr;
        },
        [](amp::blank&) -> amp::optional<TypeErasedSamplePtr> { std::terminate(); });
    return amp::visit(visitor, binding_ptr_variant);
}

void UpdateTracingDataFromTraceResult(const ResultBlank trace_result,
                                      SkeletonEventTracingData& skeleton_event_tracing_data,
                                      bool& skeleton_event_trace_point) noexcept;

}  // namespace detail_skeleton_event_tracing

struct __attribute__((packed)) SubscribeInfo
{
    std::uint16_t max_sample_count;
    std::uint8_t subscription_result;
};

tracing::SkeletonEventTracingData GenerateSkeletonTracingStructFromEventConfig(
    const InstanceIdentifier& instance_identifier,
    const BindingType binding_type,
    const amp::string_view event_name) noexcept;
tracing::SkeletonEventTracingData GenerateSkeletonTracingStructFromFieldConfig(
    const InstanceIdentifier& instance_identifier,
    const BindingType binding_type,
    const amp::string_view field_name) noexcept;

void RegisterTracingTransactionLog(amp::optional<impl::tracing::SkeletonEventTracingData> skeleton_event_tracing_data,
                                   lola::EventDataControl& event_data_control_qm) noexcept;
void UnRegisterTracingTransactionLog(lola::EventDataControl& event_data_control_qm) noexcept;

template <typename SampleType>
void TraceSend(SkeletonEventTracingData& skeleton_event_tracing_data,
               const SkeletonEventBindingBase& skeleton_event_binding_base,
               impl::SampleAllocateePtr<SampleType>& sample_data_ptr) noexcept
{
    if (skeleton_event_tracing_data.enable_send)
    {
        const auto service_element_instance_identifier =
            skeleton_event_tracing_data.service_element_instance_identifier_view;
        const auto service_element_type =
            service_element_instance_identifier.service_element_identifier_view.service_element_type;
        tracing::TracingRuntime::TracePointType trace_point{};
        if (service_element_type == tracing::ServiceElementType::EVENT)
        {
            trace_point = tracing::SkeletonEventTracePointType::SEND;
        }
        else if (service_element_type == tracing::ServiceElementType::FIELD)
        {
            trace_point = tracing::SkeletonFieldTracePointType::UPDATE;
        }
        else
        {
            AMP_ASSERT_PRD_MESSAGE(0, "Service element type must be EVENT or FIELD");
        }

        const auto tracing_data = detail_skeleton_event_tracing::ExtractBindingTracingData(sample_data_ptr);
        auto type_erased_sample_ptr_result = detail_skeleton_event_tracing::CreateTypeErasedSamplePtr(sample_data_ptr);

        const auto binding_type = skeleton_event_binding_base.GetBindingType();
        const auto trace_context_id = skeleton_event_tracing_data.trace_context_id;
        const auto trace_result = TraceShmData(binding_type,
                                               trace_context_id,
                                               service_element_instance_identifier,
                                               trace_point,
                                               tracing_data.trace_point_data_id,
                                               std::move(type_erased_sample_ptr_result),
                                               tracing_data.shm_data_chunk);
        detail_skeleton_event_tracing::UpdateTracingDataFromTraceResult(
            trace_result, skeleton_event_tracing_data, skeleton_event_tracing_data.enable_send);
    }
}

template <typename SampleType>
void TraceSendWithAllocate(SkeletonEventTracingData& skeleton_event_tracing_data,
                           const SkeletonEventBindingBase& skeleton_event_binding_base,
                           impl::SampleAllocateePtr<SampleType>& sample_data_ptr) noexcept
{
    if (skeleton_event_tracing_data.enable_send_with_allocate)
    {
        const auto service_element_instance_identifier =
            skeleton_event_tracing_data.service_element_instance_identifier_view;
        const auto service_element_type =
            service_element_instance_identifier.service_element_identifier_view.service_element_type;
        tracing::TracingRuntime::TracePointType trace_point{};
        if (service_element_type == tracing::ServiceElementType::EVENT)
        {
            trace_point = tracing::SkeletonEventTracePointType::SEND_WITH_ALLOCATE;
        }
        else if (service_element_type == tracing::ServiceElementType::FIELD)
        {
            trace_point = tracing::SkeletonFieldTracePointType::UPDATE_WITH_ALLOCATE;
        }
        else
        {
            AMP_ASSERT_PRD_MESSAGE(0, "Service element type must be EVENT or FIELD");
        }

        const auto tracing_data = detail_skeleton_event_tracing::ExtractBindingTracingData(sample_data_ptr);
        auto type_erased_sample_ptr_result = detail_skeleton_event_tracing::CreateTypeErasedSamplePtr(sample_data_ptr);

        if (!type_erased_sample_ptr_result.has_value())
        {
            return;
        }

        const auto binding_type = skeleton_event_binding_base.GetBindingType();
        const auto trace_context_id = skeleton_event_tracing_data.trace_context_id;
        const auto trace_result = TraceShmData(binding_type,
                                               trace_context_id,
                                               service_element_instance_identifier,
                                               trace_point,
                                               tracing_data.trace_point_data_id,
                                               std::move(type_erased_sample_ptr_result.value()),
                                               tracing_data.shm_data_chunk);
        detail_skeleton_event_tracing::UpdateTracingDataFromTraceResult(
            trace_result, skeleton_event_tracing_data, skeleton_event_tracing_data.enable_send_with_allocate);
    }
}

template <typename SampleType>
amp::optional<typename SkeletonEventBinding<SampleType>::SendTraceCallback> CreateTracingSendCallback(
    SkeletonEventTracingData& skeleton_event_tracing_data,
    const SkeletonEventBindingBase& skeleton_event_binding_base) noexcept
{
    amp::optional<typename SkeletonEventBinding<SampleType>::SendTraceCallback> tracing_handler{};
    if (skeleton_event_tracing_data.enable_send)
    {
        tracing_handler = [&skeleton_event_tracing_data, &skeleton_event_binding_base](
                              impl::SampleAllocateePtr<SampleType>& sample_data_ptr) mutable noexcept {
            TraceSend<SampleType>(skeleton_event_tracing_data, skeleton_event_binding_base, sample_data_ptr);
        };
    }
    return tracing_handler;
}

template <typename SampleType>
amp::optional<typename SkeletonEventBinding<SampleType>::SendTraceCallback> CreateTracingSendWithAllocateCallback(
    SkeletonEventTracingData& skeleton_event_tracing_data,
    const SkeletonEventBindingBase& skeleton_event_binding_base) noexcept
{
    amp::optional<typename SkeletonEventBinding<SampleType>::SendTraceCallback> tracing_handler{};
    if (skeleton_event_tracing_data.enable_send_with_allocate)
    {
        tracing_handler = [&skeleton_event_tracing_data, &skeleton_event_binding_base](
                              impl::SampleAllocateePtr<SampleType>& sample_data_ptr) mutable noexcept {
            TraceSendWithAllocate<SampleType>(
                skeleton_event_tracing_data, skeleton_event_binding_base, sample_data_ptr);
        };
    }
    return tracing_handler;
}

}  // namespace bmw::mw::com::impl::tracing

#endif  // PLATFORM_AAS_MW_COM_IMPL_SKELETON_EVENT_TRACING_H
