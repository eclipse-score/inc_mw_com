// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#ifndef PLATFORM_AAS_MW_COM_IMPL_CONFIGURATION_TEST_CONFIGURATION_TEST_RESOURCES_H
#define PLATFORM_AAS_MW_COM_IMPL_CONFIGURATION_TEST_CONFIGURATION_TEST_RESOURCES_H

#include "platform/aas/mw/com/impl/configuration/lola_event_id.h"
#include "platform/aas/mw/com/impl/configuration/lola_event_instance_deployment.h"
#include "platform/aas/mw/com/impl/configuration/lola_field_id.h"
#include "platform/aas/mw/com/impl/configuration/lola_field_instance_deployment.h"
#include "platform/aas/mw/com/impl/configuration/lola_service_instance_deployment.h"
#include "platform/aas/mw/com/impl/configuration/lola_service_instance_id.h"
#include "platform/aas/mw/com/impl/configuration/lola_service_type_deployment.h"
#include "platform/aas/mw/com/impl/configuration/quality_type.h"
#include "platform/aas/mw/com/impl/configuration/service_identifier_type.h"
#include "platform/aas/mw/com/impl/configuration/service_instance_deployment.h"
#include "platform/aas/mw/com/impl/configuration/service_instance_id.h"
#include "platform/aas/mw/com/impl/configuration/service_type_deployment.h"
#include "platform/aas/mw/com/impl/configuration/someip_event_instance_deployment.h"
#include "platform/aas/mw/com/impl/configuration/someip_field_instance_deployment.h"
#include "platform/aas/mw/com/impl/configuration/someip_service_instance_id.h"

#include <amp_optional.hpp>
#include <amp_overload.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <cstdint>
#include <unordered_map>

namespace bmw
{
namespace mw
{
namespace com
{
namespace impl
{

LolaEventInstanceDeployment MakeLolaEventInstanceDeployment(
    const amp::optional<std::uint16_t> number_of_sample_slots = 12U,
    const amp::optional<std::uint8_t> max_subscribers = 13U,
    const amp::optional<std::uint8_t> max_concurrent_allocations = 14U,
    const amp::optional<bool> enforce_max_samples = true,
    bool is_tracing_enabled = true) noexcept;

LolaFieldInstanceDeployment MakeLolaFieldInstanceDeployment(
    const std::uint16_t max_samples = 12U,
    const amp::optional<std::uint8_t> max_subscribers = 13U,
    const amp::optional<std::uint8_t> max_concurrent_allocations = 14U,
    const amp::optional<bool> enforce_max_samples = true,
    bool is_tracing_enabled = true) noexcept;

LolaServiceInstanceDeployment MakeLolaServiceInstanceDeployment(
    const amp::optional<LolaServiceInstanceId> instance_id = 21U,
    const amp::optional<std::size_t> shared_memory_size = 2000U) noexcept;

SomeIpServiceInstanceDeployment MakeSomeIpServiceInstanceDeployment(
    const amp::optional<SomeIpServiceInstanceId> instance_id = 22U) noexcept;

LolaServiceTypeDeployment MakeLolaServiceTypeDeployment(const std::uint16_t service_id = 31U) noexcept;

class ConfigurationStructsFixture : public ::testing::Test
{
  public:
    void ExpectLolaEventInstanceDeploymentObjectsEqual(const LolaEventInstanceDeployment& lhs,
                                                       const LolaEventInstanceDeployment& rhs) const noexcept;

    void ExpectLolaFieldInstanceDeploymentObjectsEqual(const LolaFieldInstanceDeployment& lhs,
                                                       const LolaFieldInstanceDeployment& rhs) const noexcept;

    void ExpectSomeIpEventInstanceDeploymentObjectsEqual(const SomeIpEventInstanceDeployment& lhs,
                                                         const SomeIpEventInstanceDeployment& rhs) const noexcept;

    void ExpectSomeIpFieldInstanceDeploymentObjectsEqual(const SomeIpFieldInstanceDeployment& lhs,
                                                         const SomeIpFieldInstanceDeployment& rhs) const noexcept;

    void ExpectLolaServiceInstanceDeploymentObjectsEqual(const LolaServiceInstanceDeployment& lhs,
                                                         const LolaServiceInstanceDeployment& rhs) const noexcept;

    void ExpectSomeIpServiceInstanceDeploymentObjectsEqual(const SomeIpServiceInstanceDeployment& lhs,
                                                           const SomeIpServiceInstanceDeployment& rhs) const noexcept;

    void ExpectServiceInstanceDeploymentObjectsEqual(const ServiceInstanceDeployment& lhs,
                                                     const ServiceInstanceDeployment& rhs) const noexcept;

    void ExpectLolaServiceTypeDeploymentObjectsEqual(const LolaServiceTypeDeployment& lhs,
                                                     const LolaServiceTypeDeployment& rhs) const noexcept;

    void ExpectServiceTypeDeploymentObjectsEqual(const ServiceTypeDeployment& lhs,
                                                 const ServiceTypeDeployment& rhs) const noexcept;

    void ExpectServiceVersionTypeObjectsEqual(const ServiceVersionType& lhs,
                                              const ServiceVersionType& rhs) const noexcept;

    void ExpectServiceIdentifierTypeObjectsEqual(const ServiceIdentifierType& lhs,
                                                 const ServiceIdentifierType& rhs) const noexcept;

    void ExpectServiceInstanceIdObjectsEqual(const ServiceInstanceId& lhs, const ServiceInstanceId& rhs) const noexcept;

    void ExpectLolaServiceInstanceIdObjectsEqual(const LolaServiceInstanceId& lhs,
                                                 const LolaServiceInstanceId& rhs) const noexcept;

    void ExpectSomeIpServiceInstanceIdObjectsEqual(const SomeIpServiceInstanceId& lhs,
                                                   const SomeIpServiceInstanceId& rhs) const noexcept;
};

}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_CONFIGURATION_TEST_CONFIGURATION_TEST_RESOURCES_H
