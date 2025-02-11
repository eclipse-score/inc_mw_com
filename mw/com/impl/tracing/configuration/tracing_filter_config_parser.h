// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#ifndef PLATFORM_AAS_MW_COM_IMPL_TRACING_CONFIGURATION_TRACING_FILTER_CONFIG_PARSER_H
#define PLATFORM_AAS_MW_COM_IMPL_TRACING_CONFIGURATION_TRACING_FILTER_CONFIG_PARSER_H

#include "platform/aas/lib/json/json_parser.h"
#include "platform/aas/mw/com/impl/configuration/configuration.h"
#include "platform/aas/mw/com/impl/tracing/configuration/tracing_filter_config.h"

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

/// \brief Parses a given trace-filter-configuration json file under the given path
/// \param path location, where the json file resides.
/// \return on success a valid tracing filter config will be returned
bmw::Result<TracingFilterConfig> Parse(const amp::string_view path, const Configuration& configuration) noexcept;

/// \brief Parses a trace-filter-configuration json from the given json object.
/// \param json
/// \return on success a valid tracing filter config will be returned
bmw::Result<TracingFilterConfig> Parse(bmw::json::Any json, const Configuration& configuration) noexcept;

}  // namespace tracing
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_TRACING_CONFIGURATION_TRACING_FILTER_CONFIG_PARSER_H
