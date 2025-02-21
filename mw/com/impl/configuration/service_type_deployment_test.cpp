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


#include "platform/aas/mw/com/impl/configuration/service_type_deployment.h"

#include "platform/aas/mw/com/impl/configuration/test/configuration_test_resources.h"

#include <amp_variant.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace bmw
{
namespace mw
{
namespace com
{
namespace impl
{
namespace
{

using ::testing::Pair;
using ::testing::UnorderedElementsAre;

ServiceIdentifierType dummy_service = make_ServiceIdentifierType("foo", 1U, 0U);

TEST(ServiceTypeDeploymentTest, CanConstructFromLolaServiceTypeDeployment)
{
    ServiceTypeDeployment unit{MakeLolaServiceTypeDeployment()};

    const auto* const binding_ptr = amp::get_if<LolaServiceTypeDeployment>(&unit.binding_info_);
    ASSERT_NE(binding_ptr, nullptr);
}

TEST(ServiceTypeDeploymentTest, CanConstructFromBlankBindingDeployment)
{
    ServiceTypeDeployment unit{amp::blank{}};

    const auto* const binding_ptr = amp::get_if<amp::blank>(&unit.binding_info_);
    ASSERT_NE(binding_ptr, nullptr);
}

using ServiceTypeDeploymentFixture = ConfigurationStructsFixture;
TEST_F(ServiceTypeDeploymentFixture, CanCreateFromSerializedLolaObject)
{
    ServiceTypeDeployment unit{MakeLolaServiceTypeDeployment()};

    const auto serialized_unit{unit.Serialize()};

    ServiceTypeDeployment reconstructed_unit{serialized_unit};

    ExpectServiceTypeDeploymentObjectsEqual(reconstructed_unit, unit);
}

TEST(ServiceTypeDeploymentTest, CanCreateFromSerializedBlankObject)
{
    ServiceTypeDeployment unit{amp::blank{}};

    const auto serialized_unit{unit.Serialize()};

    ServiceTypeDeployment reconstructed_unit{serialized_unit};
}

TEST(ServiceTypeDeploymentDeathTest, CreatingFromSerializedObjectWithMismatchedSerializationVersionTerminates)
{
    const ServiceTypeDeployment unit{MakeLolaServiceTypeDeployment()};

    const auto serialization_version_key = "serializationVersion";
    const std::uint32_t invalid_serialization_version = ServiceTypeDeployment::serializationVersion + 1;

    auto serialized_unit{unit.Serialize()};
    auto it = serialized_unit.find(serialization_version_key);
    ASSERT_NE(it, serialized_unit.end());
    it->second = json::Any{invalid_serialization_version};

    EXPECT_DEATH(ServiceTypeDeployment reconstructed_unit{serialized_unit}, ".*");
}

class ServiceTypeDeploymentHashFixture
    : public ::testing::TestWithParam<std::tuple<ServiceTypeDeployment, amp::string_view>>
{
};

TEST_P(ServiceTypeDeploymentHashFixture, ToHashString)
{
    const auto param_tuple = GetParam();

    const auto unit = std::get<ServiceTypeDeployment>(param_tuple);
    const auto expected_hash_string = std::get<amp::string_view>(param_tuple);

    const auto actual_hash_string = unit.ToHashString();

    EXPECT_EQ(actual_hash_string, expected_hash_string);
    EXPECT_EQ(actual_hash_string.size(), ServiceTypeDeployment::hashStringSize);
}

const std::vector<std::tuple<ServiceTypeDeployment, amp::string_view>> instance_id_to_hash_string_variations{
    {ServiceTypeDeployment{LolaServiceTypeDeployment{0U}}, "00000"},
    {ServiceTypeDeployment{LolaServiceTypeDeployment{1U}}, "00001"},
    {ServiceTypeDeployment{LolaServiceTypeDeployment{10U}}, "0000a"},
    {ServiceTypeDeployment{LolaServiceTypeDeployment{255U}}, "000ff"},
    {ServiceTypeDeployment{LolaServiceTypeDeployment{std::numeric_limits<LolaServiceId>::max()}}, "0ffff"}};
INSTANTIATE_TEST_SUITE_P(ServiceTypeDeploymentHashFixture,
                         ServiceTypeDeploymentHashFixture,
                         ::testing::ValuesIn(instance_id_to_hash_string_variations));

}  // namespace
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
