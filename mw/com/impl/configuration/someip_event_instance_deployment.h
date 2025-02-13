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


#ifndef PLATFORM_AAS_MW_COM_IMPL_CONFIGURATION_SOMEIP_EVENT_INSTANCE_DEPLOYMENT_H
#define PLATFORM_AAS_MW_COM_IMPL_CONFIGURATION_SOMEIP_EVENT_INSTANCE_DEPLOYMENT_H

#include "platform/aas/lib/json/json_parser.h"

namespace bmw
{
namespace mw
{
namespace com
{
namespace impl
{

class SomeIpEventInstanceDeployment
{
  public:
    SomeIpEventInstanceDeployment() noexcept = default;
    explicit SomeIpEventInstanceDeployment(const bmw::json::Object& /* json_object */) noexcept {}

    json::Object Serialize() const noexcept { return json::Object{}; }
};

bool operator==(const SomeIpEventInstanceDeployment& lhs, const SomeIpEventInstanceDeployment& rhs) noexcept;

}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_CONFIGURATION_SOMEIP_EVENT_INSTANCE_DEPLOYMENT_H
