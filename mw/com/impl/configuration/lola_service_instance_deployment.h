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


#ifndef PLATFORM_AAS_MW_COM_IMPL_CONFIGURATION_LOLA_SERVICE_INSTANCE_DEPLOYMENT_H
#define PLATFORM_AAS_MW_COM_IMPL_CONFIGURATION_LOLA_SERVICE_INSTANCE_DEPLOYMENT_H

#include "platform/aas/mw/com/impl/com_error.h"
#include "platform/aas/mw/com/impl/configuration/lola_event_instance_deployment.h"
#include "platform/aas/mw/com/impl/configuration/lola_field_instance_deployment.h"
#include "platform/aas/mw/com/impl/configuration/lola_service_instance_id.h"
#include "platform/aas/mw/com/impl/configuration/quality_type.h"

#include "platform/aas/lib/json/json_parser.h"

#include <amp_optional.hpp>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace bmw
{
namespace mw
{
namespace com
{
namespace impl
{

// Keep struct type even if it breaks Autosar rule A11-0-2 that says: not provide any special member functions or
// methods (such as user defined constructor, etc.).
// Note the struct is not compliant to POD type containing non-POD member.
// Public access is required by the implementation to reach the following member of the struct.
// NOLINTBEGIN(bmw-struct-usage-compliance): Intended struct semantic.
// coverity [autosar_cpp14_a11_0_2_violation]
struct LolaServiceInstanceDeployment
// NOLINTEND(bmw-struct-usage-compliance): see above
{
    using EventInstanceMapping = std::unordered_map<std::string, LolaEventInstanceDeployment>;
    using FieldInstanceMapping = std::unordered_map<std::string, LolaFieldInstanceDeployment>;

    LolaServiceInstanceDeployment() = default;
    explicit LolaServiceInstanceDeployment(const bmw::json::Object& json_object) noexcept;
    explicit LolaServiceInstanceDeployment(const LolaServiceInstanceId instance_id,
                                           EventInstanceMapping events = {},
                                           FieldInstanceMapping fields = {},
                                           const bool strict_permission = false) noexcept;

    constexpr static std::uint32_t serializationVersion{1U};
    amp::optional<LolaServiceInstanceId> instance_id_;
    amp::optional<std::size_t> shared_memory_size_;
    EventInstanceMapping events_;  // key = event name
    FieldInstanceMapping fields_;  // key = field name
    bool strict_permissions_{false};
    std::unordered_map<QualityType, std::vector<uid_t>> allowed_consumer_;
    std::unordered_map<QualityType, std::vector<uid_t>> allowed_provider_;

    bmw::json::Object Serialize() const noexcept;
    bool ContainsEvent(const std::string& event_name) const noexcept;
    bool ContainsField(const std::string& field_name) const noexcept;
};


bool areCompatible(const LolaServiceInstanceDeployment& lhs, const LolaServiceInstanceDeployment& rhs) noexcept;
bool operator==(const LolaServiceInstanceDeployment& lhs, const LolaServiceInstanceDeployment& rhs) noexcept;


}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_CONFIGURATION_LOLA_SERVICE_INSTANCE_DEPLOYMENT_H
