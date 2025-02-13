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


///
/// @file
/// 
///

#ifndef PLATFORM_AAS_MW_COM_IMPL_PROXY_EVENT_TRACING_H
#define PLATFORM_AAS_MW_COM_IMPL_PROXY_EVENT_TRACING_H

#include "platform/aas/mw/com/impl/event_receive_handler.h"
#include "platform/aas/mw/com/impl/generic_proxy_event_binding.h"
#include "platform/aas/mw/com/impl/instance_identifier.h"
#include "platform/aas/mw/com/impl/proxy_event_binding.h"
#include "platform/aas/mw/com/impl/proxy_event_binding_base.h"
#include "platform/aas/mw/com/impl/tracing/proxy_event_tracing_data.h"

#include <amp_callback.hpp>
#include <amp_string_view.hpp>

#include <cstddef>

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

ProxyEventTracingData GenerateProxyTracingStructFromEventConfig(const InstanceIdentifier& instance_identifier,
                                                                const amp::string_view event_name) noexcept;
ProxyEventTracingData GenerateProxyTracingStructFromFieldConfig(const InstanceIdentifier& instance_identifier,
                                                                const amp::string_view field_name) noexcept;

void TraceSubscribe(ProxyEventTracingData& proxy_event_tracing_data,
                    const ProxyEventBindingBase& proxy_event_binding_base,
                    const std::size_t max_sample_count) noexcept;
void TraceUnsubscribe(ProxyEventTracingData& proxy_event_tracing_data,
                      const ProxyEventBindingBase& proxy_event_binding_base) noexcept;
void TraceSetReceiveHandler(ProxyEventTracingData& proxy_event_tracing_data,
                            const ProxyEventBindingBase& proxy_event_binding_base) noexcept;
void TraceUnsetReceiveHandler(ProxyEventTracingData& proxy_event_tracing_data,
                              const ProxyEventBindingBase& proxy_event_binding_base) noexcept;
void TraceGetNewSamples(ProxyEventTracingData& proxy_event_tracing_data,
                        const ProxyEventBindingBase& proxy_event_binding_base) noexcept;
void TraceCallGetNewSamplesCallback(ProxyEventTracingData& proxy_event_tracing_data,
                                    const ProxyEventBindingBase& proxy_event_binding_base,
                                    ITracingRuntime::TracePointDataId trace_point_data_id) noexcept;
void TraceCallReceiveHandler(ProxyEventTracingData& proxy_event_tracing_data,
                             const ProxyEventBindingBase& proxy_event_binding_base) noexcept;

amp::callback<void(void), 128U> CreateTracingReceiveHandler(ProxyEventTracingData& proxy_event_tracing_data,
                                                            const ProxyEventBindingBase& proxy_event_binding_base,
                                                            EventReceiveHandler handler) noexcept;

template <typename SampleType, typename ReceiverType>
typename ProxyEventBinding<SampleType>::Callback CreateTracingGetNewSamplesCallback(
    ProxyEventTracingData& proxy_event_tracing_data,
    const ProxyEventBindingBase& proxy_event_binding_base,
    ReceiverType&& receiver) noexcept
{
    if (proxy_event_tracing_data.enable_new_samples_callback)
    {
        typename ProxyEventBinding<SampleType>::Callback tracing_receiver =
            [&proxy_event_tracing_data, &proxy_event_binding_base, receiver = std::forward<ReceiverType>(receiver)](
                SamplePtr<SampleType> sample_ptr, ITracingRuntime::TracePointDataId trace_point_data_id) noexcept {
                TraceCallGetNewSamplesCallback(proxy_event_tracing_data, proxy_event_binding_base, trace_point_data_id);
                receiver(std::move(sample_ptr));
            };
        return tracing_receiver;
    }
    else
    {
        typename ProxyEventBinding<SampleType>::Callback tracing_receiver =
            [receiver = std::forward<ReceiverType>(receiver)](SamplePtr<SampleType> sample_ptr,
                                                              ITracingRuntime::TracePointDataId) noexcept {
                receiver(std::move(sample_ptr));
            };
        return tracing_receiver;
    }
}

template <typename ReceiverType>
typename GenericProxyEventBinding::Callback CreateTracingGenericGetNewSamplesCallback(
    ProxyEventTracingData& /* proxy_event_tracing_data */,
    ReceiverType&& receiver) noexcept
{
    typename GenericProxyEventBinding::Callback tracing_receiver = [receiver = std::forward<ReceiverType>(receiver)](
                                                                       SamplePtr<void> sample_ptr,
                                                                       ITracingRuntime::TracePointDataId) noexcept {
        receiver(std::move(sample_ptr));
    };
    return tracing_receiver;
}

}  // namespace tracing
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_PROXY_EVENT_TRACING_H
