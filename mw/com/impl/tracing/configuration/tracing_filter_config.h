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


#ifndef PLATFORM_AAS_MW_COM_IMPL_TRACING_CONFIGURATION_TRACING_FILTER_CONFIG_H
#define PLATFORM_AAS_MW_COM_IMPL_TRACING_CONFIGURATION_TRACING_FILTER_CONFIG_H

#include "platform/aas/mw/com/impl/tracing/configuration/i_tracing_filter_config.h"
#include "platform/aas/mw/com/impl/tracing/configuration/trace_point_key.h"

#include <amp_string_view.hpp>

#include <set>
#include <string>
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

namespace detail_tracing_filter_config
{

class CompareStringWithStringView
{
  public:
    using is_transparent = void;
    bool operator()(const std::string& lhs, const std::string& rhs) const noexcept { return lhs < rhs; }
    bool operator()(const amp::string_view lhs_view, const std::string& rhs_string) const noexcept;
    bool operator()(const std::string& lhs_string, const amp::string_view rhs_view) const noexcept;
};

}  // namespace detail_tracing_filter_config

class TracingFilterConfig : public ITracingFilterConfig
{
  public:
    bool IsTracePointEnabled(amp::string_view service_type,
                             amp::string_view event_name,
                             InstanceSpecifierView instance_specifier,
                             SkeletonEventTracePointType skeleton_event_trace_point_type) const noexcept override;
    bool IsTracePointEnabled(amp::string_view service_type,
                             amp::string_view field_name,
                             InstanceSpecifierView instance_specifier,
                             SkeletonFieldTracePointType skeleton_field_trace_point_type) const noexcept override;
    bool IsTracePointEnabled(amp::string_view service_type,
                             amp::string_view event_name,
                             InstanceSpecifierView instance_specifier,
                             ProxyEventTracePointType proxy_event_trace_point_type) const noexcept override;
    bool IsTracePointEnabled(amp::string_view service_type,
                             amp::string_view field_name,
                             InstanceSpecifierView instance_specifier,
                             ProxyFieldTracePointType proxy_field_trace_point_type) const noexcept override;

    void AddTracePoint(amp::string_view service_type,
                       amp::string_view event_name,
                       InstanceSpecifierView instance_specifier,
                       SkeletonEventTracePointType skeleton_event_trace_point_type) noexcept override;
    void AddTracePoint(amp::string_view service_type,
                       amp::string_view field_name,
                       InstanceSpecifierView instance_specifier,
                       SkeletonFieldTracePointType skeleton_field_trace_point_type) noexcept override;
    void AddTracePoint(amp::string_view service_type,
                       amp::string_view event_name,
                       InstanceSpecifierView instance_specifier,
                       ProxyEventTracePointType proxy_event_trace_point_type) noexcept override;
    void AddTracePoint(amp::string_view service_type,
                       amp::string_view field_name,
                       InstanceSpecifierView instance_specifier,
                       ProxyFieldTracePointType proxy_field_trace_point_type) noexcept override;

    std::uint16_t GetNumberOfServiceElementsWithTraceDoneCB() const noexcept override;

  private:
    std::set<std::string, detail_tracing_filter_config::CompareStringWithStringView> config_names_;

    using TracePointMapType = std::unordered_map<TracePointKey, std::set<InstanceSpecifierView>>;
    TracePointMapType skeleton_event_trace_points_;
    TracePointMapType skeleton_field_trace_points_;
    TracePointMapType proxy_event_trace_points_;
    TracePointMapType proxy_field_trace_points_;
};

}  // namespace tracing
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_TRACING_CONFIGURATION_TRACING_FILTER_CONFIG_H
