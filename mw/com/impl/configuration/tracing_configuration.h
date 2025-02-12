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


#ifndef PLATFORM_AAS_MW_COM_IMPL_CONFIGURATION_TRACING_CONFIGURATION_H
#define PLATFORM_AAS_MW_COM_IMPL_CONFIGURATION_TRACING_CONFIGURATION_H

#include "platform/aas/mw/com/impl/instance_specifier.h"
#include "platform/aas/mw/com/impl/tracing/configuration/service_element_identifier.h"
#include "platform/aas/mw/com/impl/tracing/configuration/service_element_identifier_view.h"
#include "platform/aas/mw/com/impl/tracing/configuration/tracing_config.h"

#include <amp_string_view.hpp>

#include <map>
#include <unordered_set>

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

class CompareServiceElementIdentifierWithView
{
  public:
    using is_transparent = void;
    bool operator()(const tracing::ServiceElementIdentifier& lhs,
                    const tracing::ServiceElementIdentifier& rhs) const noexcept;
    bool operator()(const tracing::ServiceElementIdentifierView lhs_view,
                    const tracing::ServiceElementIdentifier& rhs) const noexcept;
    bool operator()(const tracing::ServiceElementIdentifier& lhs,
                    const tracing::ServiceElementIdentifierView rhs_view) const noexcept;
};

}  // namespace detail_tracing_configuration

class TracingConfiguration final
{
  public:
    TracingConfiguration() noexcept = default;

    /**
     * \brief Class is only move constructible
     */
    TracingConfiguration(const TracingConfiguration& other) = delete;
    TracingConfiguration(TracingConfiguration&& other) = default;
    TracingConfiguration& operator=(const TracingConfiguration& other) & = delete;
    TracingConfiguration& operator=(TracingConfiguration&& other) & = default;

    ~TracingConfiguration() noexcept = default;

    void SetTracingEnabled(const bool tracing_enabled) noexcept;
    void SetApplicationInstanceID(std::string application_instance_id) noexcept;
    void SetTracingTraceFilterConfigPath(std::string trace_filter_config_path) noexcept;

    bool IsTracingEnabled() const noexcept { return tracing_config_.enabled; }
    amp::string_view GetTracingFilterConfigPath() const noexcept { return tracing_config_.trace_filter_config_path; }
    amp::string_view GetApplicationInstanceID() const noexcept { return tracing_config_.application_instance_id; }

    void SetServiceElementTracingEnabled(tracing::ServiceElementIdentifier service_element_identifier,
                                         InstanceSpecifier instance_specifier) noexcept;
    bool IsServiceElementTracingEnabled(tracing::ServiceElementIdentifierView service_element_identifier_view,
                                        amp::string_view instance_specifier_view) const noexcept;

  private:
    std::map<tracing::ServiceElementIdentifier,
             std::unordered_set<InstanceSpecifier>,
             detail_tracing_configuration::CompareServiceElementIdentifierWithView>
        service_element_tracing_enabled_map_{};
    tracing::TracingConfig tracing_config_{};
};

}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_CONFIGURATION_TRACING_CONFIGURATION_H
