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


#include "platform/aas/mw/com/impl/tracing/configuration/tracing_filter_config.h"

#include "platform/aas/mw/com/impl/tracing/configuration/service_element_identifier_view.h"
#include "platform/aas/mw/com/impl/tracing/configuration/service_element_type.h"

#include "platform/aas/mw/log/logging.h"

#include <exception>
#include <limits>
#include <unordered_set>

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

bool CompareStringWithStringView::operator()(const amp::string_view lhs_view,
                                             const std::string& rhs_string) const noexcept
{
    return lhs_view < amp::string_view{rhs_string.data(), rhs_string.size()};
}

bool CompareStringWithStringView::operator()(const std::string& lhs_string,
                                             const amp::string_view rhs_view) const noexcept
{
    return amp::string_view{lhs_string.data(), lhs_string.size()} < rhs_view;
}

}  // namespace detail_tracing_filter_config

namespace
{

using CompareStringWithStringView = detail_tracing_filter_config::CompareStringWithStringView;
using InstanceSpecifierView = ITracingFilterConfig::InstanceSpecifierView;

const amp::string_view GetOrInsertStringInSet(const amp::string_view key,
                                              std::set<std::string, CompareStringWithStringView>& search_set) noexcept
{
    const auto find_result = search_set.find(key);
    const bool does_string_already_exist{find_result != search_set.end()};
    if (does_string_already_exist)
    {
        return find_result->data();
    }

    auto insert_result = search_set.emplace(key.data(), key.size());
    if (!insert_result.second)
    {
        bmw::mw::log::LogFatal("lola") << "Failed to insert string" << key << "into config_names_map. Terminating.";
        std::terminate();
    }
    return insert_result.first->data();
}

void InsertTracePointIntoMap(
    const TracePointKey& trace_point_key,
    InstanceSpecifierView instance_specifier,
    std::unordered_map<TracePointKey, std::set<InstanceSpecifierView>>& trace_point_map) noexcept
{
    auto map_it = trace_point_map.find(trace_point_key);
    if (map_it == trace_point_map.end())
    {
        std::set<InstanceSpecifierView> instance_specifiers{instance_specifier};
        const auto map_insert_result = trace_point_map.insert({trace_point_key, std::move(instance_specifiers)});
        if (!map_insert_result.second)
        {
            bmw::mw::log::LogFatal("lola") << "Failed to insert trace point key into map. Terminating.";
            std::terminate();
        }
    }
    else
    {
        map_it->second.insert(instance_specifier);
    }
}

template <typename TracePointType>
void AddTracePointToMap(amp::string_view service_type,
                        amp::string_view service_element_name,
                        ServiceElementType service_element_type,
                        InstanceSpecifierView instance_specifier,
                        TracePointType trace_point_type,
                        std::unordered_map<TracePointKey, std::set<InstanceSpecifierView>>& trace_point_map,
                        std::set<std::string, CompareStringWithStringView>& config_names) noexcept
{
    if (trace_point_type == TracePointType::INVALID)
    {
        bmw::mw::log::LogFatal("lola") << "Invalid TracePointType: " << static_cast<int>(trace_point_type);
        std::terminate();
    }
    auto service_type_stored = GetOrInsertStringInSet(service_type, config_names);
    auto service_element_name_stored = GetOrInsertStringInSet(service_element_name, config_names);
    const ServiceElementIdentifierView service_element_identifer{
        service_type_stored, service_element_name_stored, service_element_type};

    auto trace_point_type_int = static_cast<std::uint8_t>(trace_point_type);
    const TracePointKey trace_point_key{service_element_identifer, trace_point_type_int};

    InsertTracePointIntoMap(trace_point_key, instance_specifier, trace_point_map);
}

template <typename TracePointType>
bool IsTracePointEnabledInMap(
    amp::string_view service_type,
    amp::string_view service_element_name,
    ServiceElementType service_element_type,
    InstanceSpecifierView instance_specifier,
    TracePointType trace_point_type,
    const std::unordered_map<TracePointKey, std::set<InstanceSpecifierView>>& trace_point_map) noexcept
{
    const ServiceElementIdentifierView service_element_identifer{
        service_type, service_element_name, service_element_type};
    auto trace_point_type_int = static_cast<std::uint8_t>(trace_point_type);
    const TracePointKey trace_point_key{service_element_identifer, trace_point_type_int};

    auto map_it = trace_point_map.find(trace_point_key);
    if (map_it == trace_point_map.end())
    {
        return false;
    }
    const auto instance_specifiers = map_it->second;
    const bool instance_specifier_in_vector{instance_specifiers.count(instance_specifier) != 0};
    return instance_specifier_in_vector;
}

bool DoesTracePointNeedTraceDoneCB(const SkeletonEventTracePointType trace_point_type) noexcept
{
    return ((trace_point_type == SkeletonEventTracePointType::SEND) ||
            (trace_point_type == SkeletonEventTracePointType::SEND_WITH_ALLOCATE));
}

bool DoesTracePointNeedTraceDoneCB(const SkeletonFieldTracePointType trace_point_type) noexcept
{
    return ((trace_point_type == SkeletonFieldTracePointType::UPDATE) ||
            (trace_point_type == SkeletonFieldTracePointType::UPDATE_WITH_ALLOCATE));
}

bool DoesTracePointNeedTraceDoneCB(const ProxyEventTracePointType) noexcept
{
    return false;
}

bool DoesTracePointNeedTraceDoneCB(const ProxyFieldTracePointType) noexcept
{
    return false;
}

template <typename TracePointType>
std::size_t FindNumberOfTracePointsNeedingTraceDoneCB(
    const std::unordered_map<TracePointKey, std::set<InstanceSpecifierView>>& trace_point_map,
    std::unordered_set<ServiceElementIdentifierView>& service_element_identifier_view_set) noexcept
{
    std::size_t trace_points_count{0U};
    for (const auto& trace_point_map_element : trace_point_map)
    {
        const auto trace_point_type = static_cast<TracePointType>(trace_point_map_element.first.trace_point_type);
        if (DoesTracePointNeedTraceDoneCB(trace_point_type))
        {
            const auto service_element = trace_point_map_element.first.service_element;
            if (service_element_identifier_view_set.count(service_element) == 0)
            {
                service_element_identifier_view_set.insert(service_element);
                const auto instance_specifier_view_set = trace_point_map_element.second;
                trace_points_count += instance_specifier_view_set.size();
            }
        }
    }
    return trace_points_count;
}

}  // namespace

bool TracingFilterConfig::IsTracePointEnabled(
    amp::string_view service_type,
    amp::string_view event_name,
    InstanceSpecifierView instance_specifier,
    SkeletonEventTracePointType skeleton_event_trace_point_type) const noexcept
{
    return IsTracePointEnabledInMap(service_type,
                                    event_name,
                                    ServiceElementType::EVENT,
                                    instance_specifier,
                                    skeleton_event_trace_point_type,
                                    skeleton_event_trace_points_);
}

bool TracingFilterConfig::IsTracePointEnabled(
    amp::string_view service_type,
    amp::string_view field_name,
    InstanceSpecifierView instance_specifier,
    SkeletonFieldTracePointType skeleton_field_trace_point_type) const noexcept
{
    return IsTracePointEnabledInMap(service_type,
                                    field_name,
                                    ServiceElementType::FIELD,
                                    instance_specifier,
                                    skeleton_field_trace_point_type,
                                    skeleton_field_trace_points_);
}

bool TracingFilterConfig::IsTracePointEnabled(amp::string_view service_type,
                                              amp::string_view event_name,
                                              InstanceSpecifierView instance_specifier,
                                              ProxyEventTracePointType proxy_event_trace_point_type) const noexcept
{
    return IsTracePointEnabledInMap(service_type,
                                    event_name,
                                    ServiceElementType::EVENT,
                                    instance_specifier,
                                    proxy_event_trace_point_type,
                                    proxy_event_trace_points_);
}

bool TracingFilterConfig::IsTracePointEnabled(amp::string_view service_type,
                                              amp::string_view field_name,
                                              InstanceSpecifierView instance_specifier,
                                              ProxyFieldTracePointType proxy_field_trace_point_type) const noexcept
{
    return IsTracePointEnabledInMap(service_type,
                                    field_name,
                                    ServiceElementType::FIELD,
                                    instance_specifier,
                                    proxy_field_trace_point_type,
                                    proxy_field_trace_points_);
}

void TracingFilterConfig::AddTracePoint(amp::string_view service_type,
                                        amp::string_view event_name,
                                        InstanceSpecifierView instance_specifier,
                                        SkeletonEventTracePointType skeleton_event_trace_point_type) noexcept
{
    AddTracePointToMap(service_type,
                       event_name,
                       ServiceElementType::EVENT,
                       instance_specifier,
                       skeleton_event_trace_point_type,
                       skeleton_event_trace_points_,
                       config_names_);
}

void TracingFilterConfig::AddTracePoint(amp::string_view service_type,
                                        amp::string_view field_name,
                                        InstanceSpecifierView instance_specifier,
                                        SkeletonFieldTracePointType skeleton_field_trace_point_type) noexcept
{
    AddTracePointToMap(service_type,
                       field_name,
                       ServiceElementType::FIELD,
                       instance_specifier,
                       skeleton_field_trace_point_type,
                       skeleton_field_trace_points_,
                       config_names_);
}

void TracingFilterConfig::AddTracePoint(amp::string_view service_type,
                                        amp::string_view event_name,
                                        InstanceSpecifierView instance_specifier,
                                        ProxyEventTracePointType proxy_event_trace_point_type) noexcept
{
    AddTracePointToMap(service_type,
                       event_name,
                       ServiceElementType::EVENT,
                       instance_specifier,
                       proxy_event_trace_point_type,
                       proxy_event_trace_points_,
                       config_names_);
}

void TracingFilterConfig::AddTracePoint(amp::string_view service_type,
                                        amp::string_view field_name,
                                        InstanceSpecifierView instance_specifier,
                                        ProxyFieldTracePointType proxy_field_trace_point_type) noexcept
{
    AddTracePointToMap(service_type,
                       field_name,
                       ServiceElementType::FIELD,
                       instance_specifier,
                       proxy_field_trace_point_type,
                       proxy_field_trace_points_,
                       config_names_);
}

std::uint16_t TracingFilterConfig::GetNumberOfServiceElementsWithTraceDoneCB() const noexcept
{
    std::unordered_set<ServiceElementIdentifierView> service_element_identifier_view_set{};
    std::size_t number_trace_points{0U};
    number_trace_points += FindNumberOfTracePointsNeedingTraceDoneCB<SkeletonEventTracePointType>(
        skeleton_event_trace_points_, service_element_identifier_view_set);
    number_trace_points += FindNumberOfTracePointsNeedingTraceDoneCB<SkeletonFieldTracePointType>(
        skeleton_field_trace_points_, service_element_identifier_view_set);
    number_trace_points += FindNumberOfTracePointsNeedingTraceDoneCB<ProxyEventTracePointType>(
        proxy_event_trace_points_, service_element_identifier_view_set);
    number_trace_points += FindNumberOfTracePointsNeedingTraceDoneCB<ProxyFieldTracePointType>(
        proxy_field_trace_points_, service_element_identifier_view_set);
    AMP_ASSERT_PRD(number_trace_points < std::numeric_limits<std::uint16_t>::max());
    return static_cast<std::uint16_t>(number_trace_points);
}

}  // namespace tracing
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
