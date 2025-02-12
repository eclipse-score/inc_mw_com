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


#ifndef PLATFORM_AAS_MW_COM_IMPL_CONFIGURATION_SOMEIP_SERVICE_INSTANCE_DEPLOYMENT_H
#define PLATFORM_AAS_MW_COM_IMPL_CONFIGURATION_SOMEIP_SERVICE_INSTANCE_DEPLOYMENT_H

#include "platform/aas/mw/com/impl/configuration/someip_event_instance_deployment.h"
#include "platform/aas/mw/com/impl/configuration/someip_field_instance_deployment.h"
#include "platform/aas/mw/com/impl/configuration/someip_service_instance_id.h"

#include "platform/aas/lib/json/json_parser.h"

#include <amp_optional.hpp>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <utility>

namespace bmw
{
namespace mw
{
namespace com
{
namespace impl
{

// Keep struct type even if it breaks Autosar rule A11-0-2 that says: not provide any special member functions or
// methods (such as ToString()).
// Note the struct is not compliant to POD type containing non-POD member.
// The struct is used as a config storage obtained by performing the parsing json object.
// Public access is required by the implementation to reach the following members of the struct.
// NOLINTBEGIN(bmw-struct-usage-compliance): Intended struct semantic.
// coverity [autosar_cpp14_a11_0_2_violation]
struct SomeIpServiceInstanceDeployment
// NOLINTEND(bmw-struct-usage-compliance): see above
{
  public:
    using EventInstanceMapping = std::unordered_map<std::string, SomeIpEventInstanceDeployment>;
    using FieldInstanceMapping = std::unordered_map<std::string, SomeIpFieldInstanceDeployment>;

    explicit SomeIpServiceInstanceDeployment(const bmw::json::Object& json_object) noexcept;
    explicit SomeIpServiceInstanceDeployment(amp::optional<SomeIpServiceInstanceId> instance_id = {},
                                             EventInstanceMapping events = {},
                                             FieldInstanceMapping fields = {})
        : instance_id_{instance_id}, events_{std::move(events)}, fields_{std::move(fields)}
    {
    }

    constexpr static std::uint32_t serializationVersion{1U};
    amp::optional<SomeIpServiceInstanceId> instance_id_;
    EventInstanceMapping events_;  // key = event name
    FieldInstanceMapping fields_;  // key = field name

    bmw::json::Object Serialize() const noexcept;
};

bool areCompatible(const SomeIpServiceInstanceDeployment& lhs, const SomeIpServiceInstanceDeployment& rhs);
bool operator==(const SomeIpServiceInstanceDeployment& lhs, const SomeIpServiceInstanceDeployment& rhs) noexcept;

}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_CONFIGURATION_SOMEIP_SERVICE_INSTANCE_DEPLOYMENT_H
