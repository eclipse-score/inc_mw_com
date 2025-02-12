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


#ifndef PLATFORM_AAS_MW_COM_IMPL_CONFIGURATION_CONFIG_PARSER_H
#define PLATFORM_AAS_MW_COM_IMPL_CONFIGURATION_CONFIG_PARSER_H

#include "platform/aas/lib/json/json_parser.h"
#include "platform/aas/mw/com/impl/configuration/configuration.h"

#include <amp_string_view.hpp>

namespace bmw
{
namespace mw
{
namespace com
{
namespace impl
{
namespace configuration
{

Configuration Parse(const amp::string_view path) noexcept;
Configuration Parse(bmw::json::Any json) noexcept;

}  // namespace configuration
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_CONFIGURATION_CONFIG_PARSER_H
