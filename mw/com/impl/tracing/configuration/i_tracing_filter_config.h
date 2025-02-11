// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#ifndef PLATFORM_AAS_MW_COM_IMPL_TRACING_CONFIGURATION_I_TRACING_FILTER_CONFIG_H
#define PLATFORM_AAS_MW_COM_IMPL_TRACING_CONFIGURATION_I_TRACING_FILTER_CONFIG_H

#include "platform/aas/mw/com/impl/tracing/configuration/proxy_event_trace_point_type.h"
#include "platform/aas/mw/com/impl/tracing/configuration/proxy_field_trace_point_type.h"
#include "platform/aas/mw/com/impl/tracing/configuration/skeleton_event_trace_point_type.h"
#include "platform/aas/mw/com/impl/tracing/configuration/skeleton_field_trace_point_type.h"

#include <amp_optional.hpp>
#include <amp_string_view.hpp>

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

class ITracingFilterConfig
{
  public:
    using InstanceSpecifierView = amp::string_view;

    virtual ~ITracingFilterConfig() = default;

    virtual bool IsTracePointEnabled(amp::string_view service_type,
                                     amp::string_view event_name,
                                     InstanceSpecifierView instance_specifier,
                                     SkeletonEventTracePointType skeleton_event_trace_point_type) const noexcept = 0;
    virtual bool IsTracePointEnabled(amp::string_view service_type,
                                     amp::string_view event_name,
                                     InstanceSpecifierView instance_specifier,
                                     SkeletonFieldTracePointType skeleton_field_trace_point_type) const noexcept = 0;
    virtual bool IsTracePointEnabled(amp::string_view service_type,
                                     amp::string_view event_name,
                                     InstanceSpecifierView instance_specifier,
                                     ProxyEventTracePointType proxy_event_trace_point_type) const noexcept = 0;
    virtual bool IsTracePointEnabled(amp::string_view service_type,
                                     amp::string_view event_name,
                                     InstanceSpecifierView instance_specifier,
                                     ProxyFieldTracePointType proxy_field_trace_point_type) const noexcept = 0;

    virtual void AddTracePoint(amp::string_view service_type,
                               amp::string_view event_name,
                               InstanceSpecifierView instance_specifier,
                               SkeletonEventTracePointType skeleton_event_trace_point_type) noexcept = 0;
    virtual void AddTracePoint(amp::string_view service_type,
                               amp::string_view field_name,
                               InstanceSpecifierView instance_specifier,
                               SkeletonFieldTracePointType skeleton_field_trace_point_type) noexcept = 0;
    virtual void AddTracePoint(amp::string_view service_type,
                               amp::string_view event_name,
                               InstanceSpecifierView instance_specifier,
                               ProxyEventTracePointType proxy_event_trace_point_type) noexcept = 0;
    virtual void AddTracePoint(amp::string_view service_type,
                               amp::string_view field_name,
                               InstanceSpecifierView instance_specifier,
                               ProxyFieldTracePointType proxy_field_trace_point_type) noexcept = 0;

    virtual std::uint16_t GetNumberOfServiceElementsWithTraceDoneCB() const noexcept = 0;
};

}  // namespace tracing
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_TRACING_CONFIGURATION_I_TRACING_FILTER_CONFIG_H
