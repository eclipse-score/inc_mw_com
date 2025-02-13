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


#include "platform/aas/mw/com/impl/tracing/skeleton_event_tracing.h"

#include "platform/aas/mw/com/impl/bindings/lola/transaction_log_set.h"
#include "platform/aas/mw/com/impl/runtime.h"
#include "platform/aas/mw/com/impl/tracing/common_event_tracing.h"
#include "platform/aas/mw/com/impl/tracing/configuration/proxy_event_trace_point_type.h"
#include "platform/aas/mw/com/impl/tracing/configuration/proxy_field_trace_point_type.h"
#include "platform/aas/mw/com/impl/tracing/configuration/service_element_instance_identifier_view.h"
#include "platform/aas/mw/com/impl/tracing/configuration/skeleton_event_trace_point_type.h"
#include "platform/aas/mw/com/impl/tracing/configuration/skeleton_field_trace_point_type.h"
#include "platform/aas/mw/com/impl/tracing/configuration/tracing_filter_config.h"
#include "platform/aas/mw/com/impl/tracing/skeleton_event_tracing_data.h"
#include "platform/aas/mw/com/impl/tracing/trace_error.h"

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

namespace detail_skeleton_event_tracing
{

void UpdateTracingDataFromTraceResult(const ResultBlank trace_result,
                                      SkeletonEventTracingData& skeleton_event_tracing_data,
                                      bool& skeleton_event_trace_point) noexcept
{
    if (!trace_result.has_value())
    {
        if (trace_result.error() == TraceErrorCode::TraceErrorDisableTracePointInstance)
        {
            skeleton_event_trace_point = false;
        }
        else if (trace_result.error() == TraceErrorCode::TraceErrorDisableAllTracePoints)
        {
            DisableAllTracePoints(skeleton_event_tracing_data);
        }
        else
        {
            ::bmw::mw::log::LogError("lola")
                << "Unexpected error received from trace call:" << trace_result.error() << ". Ignoring.";
        }
    }
}

}  // namespace detail_skeleton_event_tracing

SkeletonEventTracingData GenerateSkeletonTracingStructFromEventConfig(const InstanceIdentifier& instance_identifier,
                                                                      const BindingType binding_type,
                                                                      const amp::string_view event_name) noexcept
{
    auto& runtime = Runtime::getInstance();
    const auto* const tracing_config = runtime.GetTracingFilterConfig();
    auto* const tracing_runtime = runtime.GetTracingRuntime();
    SkeletonEventTracingData skeleton_event_tracing_data{};
    if (tracing_config != nullptr && tracing_runtime != nullptr)
    {
        const auto service_element_instance_identifier_view =
            GetServiceElementInstanceIdentifierView(instance_identifier, event_name, ServiceElementType::EVENT);
        const auto instance_specifier_view = service_element_instance_identifier_view.instance_specifier;
        const auto service_type =
            service_element_instance_identifier_view.service_element_identifier_view.service_type_name;
        skeleton_event_tracing_data.service_element_instance_identifier_view = service_element_instance_identifier_view;

        skeleton_event_tracing_data.enable_send = tracing_config->IsTracePointEnabled(
            service_type, event_name, instance_specifier_view, SkeletonEventTracePointType::SEND);
        skeleton_event_tracing_data.enable_send_with_allocate = tracing_config->IsTracePointEnabled(
            service_type, event_name, instance_specifier_view, SkeletonEventTracePointType::SEND_WITH_ALLOCATE);

        // only register this service element at Runtime, in case TraceDoneCB relevant trace-point are enabled:
        const auto isTraceDoneCallbackNeeded =
            skeleton_event_tracing_data.enable_send || skeleton_event_tracing_data.enable_send_with_allocate;
        if (isTraceDoneCallbackNeeded)
        {
            const auto trace_context_id = tracing_runtime->RegisterServiceElement(binding_type);
            skeleton_event_tracing_data.trace_context_id = trace_context_id;
        }
    }
    return skeleton_event_tracing_data;
}

SkeletonEventTracingData GenerateSkeletonTracingStructFromFieldConfig(const InstanceIdentifier& instance_identifier,
                                                                      const BindingType binding_type,
                                                                      const amp::string_view field_name) noexcept
{
    auto& runtime = Runtime::getInstance();
    const auto* const tracing_config = runtime.GetTracingFilterConfig();
    auto* const tracing_runtime = runtime.GetTracingRuntime();
    SkeletonEventTracingData skeleton_event_tracing_data{};
    if (tracing_config != nullptr && tracing_runtime != nullptr)
    {
        const auto service_element_instance_identifier_view =
            GetServiceElementInstanceIdentifierView(instance_identifier, field_name, ServiceElementType::FIELD);
        const auto instance_specifier_view = service_element_instance_identifier_view.instance_specifier;
        const auto service_type =
            service_element_instance_identifier_view.service_element_identifier_view.service_type_name;
        skeleton_event_tracing_data.service_element_instance_identifier_view = service_element_instance_identifier_view;

        skeleton_event_tracing_data.enable_send = tracing_config->IsTracePointEnabled(
            service_type, field_name, instance_specifier_view, SkeletonFieldTracePointType::UPDATE);
        skeleton_event_tracing_data.enable_send_with_allocate = tracing_config->IsTracePointEnabled(
            service_type, field_name, instance_specifier_view, SkeletonFieldTracePointType::UPDATE_WITH_ALLOCATE);

        // only register this service element at Runtime, in case TraceDoneCB relevant trace-point are enabled:
        const auto isTraceDoneCallbackNeeded =
            skeleton_event_tracing_data.enable_send || skeleton_event_tracing_data.enable_send_with_allocate;
        if (isTraceDoneCallbackNeeded)
        {
            const auto trace_context_id = tracing_runtime->RegisterServiceElement(binding_type);
            skeleton_event_tracing_data.trace_context_id = trace_context_id;
        }
    }
    return skeleton_event_tracing_data;
}

void RegisterTracingTransactionLog(amp::optional<impl::tracing::SkeletonEventTracingData> skeleton_event_tracing_data,
                                   lola::EventDataControl& event_data_control_qm) noexcept
{
    if (!skeleton_event_tracing_data.has_value())
    {
        return;
    }
    const auto skeleton_event_tracing_data_value = skeleton_event_tracing_data.value();
    if (skeleton_event_tracing_data_value.enable_send || skeleton_event_tracing_data_value.enable_send_with_allocate)
    {
        amp::ignore = event_data_control_qm.GetTransactionLogSet().RegisterSkeletonTracingElement();
    }
}

void UnRegisterTracingTransactionLog(lola::EventDataControl& event_data_control_qm) noexcept
{
    event_data_control_qm.GetTransactionLogSet().Unregister(lola::TransactionLogSet::kSkeletonIndexSentinel);
}

}  // namespace tracing
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
