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


#include "platform/aas/mw/com/impl/configuration/tracing_configuration.h"

#include "platform/aas/mw/log/logging.h"

#include <exception>

namespace bmw
{
namespace mw
{
namespace com
{
namespace impl
{

namespace detail_tracing_configuration
{

bool CompareServiceElementIdentifierWithView::operator()(const tracing::ServiceElementIdentifier& lhs,
                                                         const tracing::ServiceElementIdentifier& rhs) const noexcept
{
    return lhs < rhs;
}

bool CompareServiceElementIdentifierWithView::operator()(const tracing::ServiceElementIdentifierView lhs_view,
                                                         const tracing::ServiceElementIdentifier& rhs) const noexcept
{
    const tracing::ServiceElementIdentifierView rhs_view{
        amp::string_view{rhs.service_type_name}, amp::string_view{rhs.service_element_name}, rhs.service_element_type};
    return lhs_view < rhs_view;
}

bool CompareServiceElementIdentifierWithView::operator()(
    const tracing::ServiceElementIdentifier& lhs,
    const tracing::ServiceElementIdentifierView rhs_view) const noexcept
{
    const tracing::ServiceElementIdentifierView lhs_view{
        amp::string_view{lhs.service_type_name}, amp::string_view{lhs.service_element_name}, lhs.service_element_type};
    return lhs_view < rhs_view;
}

}  // namespace detail_tracing_configuration

void TracingConfiguration::SetTracingEnabled(const bool tracing_enabled) noexcept
{
    tracing_config_.enabled = tracing_enabled;
}

void TracingConfiguration::SetApplicationInstanceID(std::string application_instance_id) noexcept
{
    tracing_config_.application_instance_id = application_instance_id;
}

void TracingConfiguration::SetTracingTraceFilterConfigPath(std::string trace_filter_config_path) noexcept
{
    tracing_config_.trace_filter_config_path = trace_filter_config_path;
}

void TracingConfiguration::SetServiceElementTracingEnabled(tracing::ServiceElementIdentifier service_element_identifier,
                                                           InstanceSpecifier instance_specifier) noexcept
{
    auto find_result = service_element_tracing_enabled_map_.find(service_element_identifier);
    if (find_result == service_element_tracing_enabled_map_.end())
    {
        const auto insert_result = service_element_tracing_enabled_map_.emplace(
            std::make_pair(std::move(service_element_identifier),
                           std::unordered_set<InstanceSpecifier>{std::move(instance_specifier)}));
        if (!insert_result.second)
        {
            ::bmw::mw::log::LogFatal("lola")
                << "Could not insert Service element into service element tracing enabled map. Terminating";
            std::terminate();
        }
    }
    else
    {
        const auto insert_result = find_result->second.insert(std::move(instance_specifier));
        if (!insert_result.second)
        {
            ::bmw::mw::log::LogFatal("lola")
                << "Could not insert instance specifier into service element tracing enabled map. Terminating";
            std::terminate();
        }
    }
}

bool TracingConfiguration::IsServiceElementTracingEnabled(
    tracing::ServiceElementIdentifierView service_element_identifier_view,
    amp::string_view instance_specifier_view) const noexcept
{
    const auto find_result = service_element_tracing_enabled_map_.find(service_element_identifier_view);
    if (find_result == service_element_tracing_enabled_map_.end())
    {
        return false;
    }

    const auto instance_specifier = InstanceSpecifier::Create(instance_specifier_view).value();
    const auto& instance_specifier_set = find_result->second;
    return instance_specifier_set.count(instance_specifier) != 0U;
}

}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
