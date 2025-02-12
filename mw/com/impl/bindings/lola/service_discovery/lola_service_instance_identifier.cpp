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


#include "platform/aas/mw/com/impl/bindings/lola/service_discovery/lola_service_instance_identifier.h"
#include "platform/aas/mw/com/impl/configuration/lola_service_type_deployment.h"

namespace bmw::mw::com::impl::lola
{

LolaServiceInstanceIdentifier::LolaServiceInstanceIdentifier(LolaServiceId service_id) noexcept
    : service_id_{service_id}, instance_id_{std::nullopt}
{
}

LolaServiceInstanceIdentifier::LolaServiceInstanceIdentifier(
    const EnrichedInstanceIdentifier& enriched_instance_identifier) noexcept
    : LolaServiceInstanceIdentifier{
          enriched_instance_identifier.GetBindingSpecificServiceId<LolaServiceTypeDeployment>().value()}
{
    const auto& expected_instance_id =
        enriched_instance_identifier.GetBindingSpecificInstanceId<LolaServiceInstanceId>();

    if (expected_instance_id.has_value())
    {
        instance_id_ = expected_instance_id.value();
    }
}

LolaServiceId LolaServiceInstanceIdentifier::GetServiceId() const noexcept
{
    return service_id_;
}

std::optional<LolaServiceInstanceId::InstanceId> LolaServiceInstanceIdentifier::GetInstanceId() const noexcept
{
    return instance_id_;
}

bool operator==(const LolaServiceInstanceIdentifier& lhs, const LolaServiceInstanceIdentifier& rhs) noexcept
{
    return (lhs.GetServiceId() == rhs.GetServiceId()) && (lhs.GetInstanceId() == rhs.GetInstanceId());
}

}  // namespace bmw::mw::com::impl::lola

std::size_t std::hash<bmw::mw::com::impl::lola::LolaServiceInstanceIdentifier>::operator()(
    const bmw::mw::com::impl::lola::LolaServiceInstanceIdentifier& identifier) const noexcept
{
    static_assert(sizeof(bmw::mw::com::impl::LolaServiceId) <= 4U);
    static_assert(sizeof(bmw::mw::com::impl::LolaServiceInstanceId::InstanceId) <= 2U);
    std::size_t result{static_cast<std::size_t>(identifier.GetServiceId()) << 32U};
    if (identifier.GetInstanceId().has_value())
    {
        result += static_cast<std::size_t>(identifier.GetInstanceId().value()) << 16U;
        result += 1U;
    }
    return result;
}
