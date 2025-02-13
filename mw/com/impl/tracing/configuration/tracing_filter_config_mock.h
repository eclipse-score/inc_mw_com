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


#ifndef PLATFORM_AAS_MW_COM_IMPL_TRACING_CONFIGURATION_TRACING_FILTER_CONFIG_MOCK_H
#define PLATFORM_AAS_MW_COM_IMPL_TRACING_CONFIGURATION_TRACING_FILTER_CONFIG_MOCK_H

#include "platform/aas/mw/com/impl/tracing/configuration/i_tracing_filter_config.h"

#include <gmock/gmock.h>

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

class TracingFilterConfigMock : public ITracingFilterConfig
{
  public:
    MOCK_METHOD(bool,
                IsTracePointEnabled,
                (amp::string_view service_type,
                 amp::string_view event_name,
                 InstanceSpecifierView instance_specifier,
                 SkeletonEventTracePointType skeleton_event_trace_point_type),
                (const, noexcept, override));
    MOCK_METHOD(bool,
                IsTracePointEnabled,
                (amp::string_view service_type,
                 amp::string_view event_name,
                 InstanceSpecifierView instance_specifier,
                 SkeletonFieldTracePointType skeleton_field_trace_point_type),
                (const, noexcept, override));
    MOCK_METHOD(bool,
                IsTracePointEnabled,
                (amp::string_view service_type,
                 amp::string_view event_name,
                 InstanceSpecifierView instance_specifier,
                 ProxyEventTracePointType proxy_event_trace_point_type),
                (const, noexcept, override));
    MOCK_METHOD(bool,
                IsTracePointEnabled,
                (amp::string_view service_type,
                 amp::string_view event_name,
                 InstanceSpecifierView instance_specifier,
                 ProxyFieldTracePointType proxy_field_trace_point_type),
                (const, noexcept, override));

    MOCK_METHOD(void,
                AddTracePoint,
                (amp::string_view service_type,
                 amp::string_view event_name,
                 InstanceSpecifierView instance_specifier,
                 SkeletonEventTracePointType skeleton_event_trace_point_type),
                (noexcept, override));
    MOCK_METHOD(void,
                AddTracePoint,
                (amp::string_view service_type,
                 amp::string_view event_name,
                 InstanceSpecifierView instance_specifier,
                 SkeletonFieldTracePointType skeleton_field_trace_point_type),
                (noexcept, override));
    MOCK_METHOD(void,
                AddTracePoint,
                (amp::string_view service_type,
                 amp::string_view event_name,
                 InstanceSpecifierView instance_specifier,
                 ProxyEventTracePointType proxy_event_trace_point_type),
                (noexcept, override));
    MOCK_METHOD(void,
                AddTracePoint,
                (amp::string_view service_type,
                 amp::string_view event_name,
                 InstanceSpecifierView instance_specifier,
                 ProxyFieldTracePointType proxy_field_trace_point_type),
                (noexcept, override));
    MOCK_METHOD(std::uint16_t, GetNumberOfServiceElementsWithTraceDoneCB, (), (const, noexcept, override));
};

}  // namespace tracing
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_TRACING_CONFIGURATION_TRACING_FILTER_CONFIG_MOCK_H
