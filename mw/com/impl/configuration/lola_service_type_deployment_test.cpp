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


#include "platform/aas/mw/com/impl/configuration/lola_service_type_deployment.h"

#include "platform/aas/mw/com/impl/configuration/test/configuration_test_resources.h"

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

using LolaServiceTypeDeploymentFixture = ConfigurationStructsFixture;
TEST_F(LolaServiceTypeDeploymentFixture, CanCreateFromSerializedObject)
{
    const LolaServiceTypeDeployment unit{MakeLolaServiceTypeDeployment()};

    const auto serialized_unit{unit.Serialize()};

    LolaServiceTypeDeployment reconstructed_unit{serialized_unit};

    ExpectLolaServiceTypeDeploymentObjectsEqual(reconstructed_unit, unit);
}

TEST(LolaServiceTypeDeploymentDeathTest, CreatingFromSerializedObjectWithMismatchedSerializationVersionTerminates)
{
    const LolaServiceTypeDeployment unit{MakeLolaServiceTypeDeployment()};

    const auto serialization_version_key = "serializationVersion";
    const std::uint32_t invalid_serialization_version = LolaServiceTypeDeployment::serializationVersion + 1;

    auto serialized_unit{unit.Serialize()};
    auto it = serialized_unit.find(serialization_version_key);
    ASSERT_NE(it, serialized_unit.end());
    it->second = json::Any{invalid_serialization_version};

    EXPECT_DEATH(LolaServiceTypeDeployment reconstructed_unit{serialized_unit}, ".*");
}

class LolaServiceTypeDeploymentHashFixture
    : public ::testing::TestWithParam<std::tuple<LolaServiceTypeDeployment, amp::string_view>>
{
};

TEST_P(LolaServiceTypeDeploymentHashFixture, ToHashString)
{
    const auto param_tuple = GetParam();

    const auto unit = std::get<LolaServiceTypeDeployment>(param_tuple);
    const auto expected_hash_string = std::get<amp::string_view>(param_tuple);

    const auto actual_hash_string = unit.ToHashString();

    EXPECT_EQ(actual_hash_string, expected_hash_string);
    EXPECT_EQ(actual_hash_string.size(), LolaServiceTypeDeployment::hashStringSize);
}

const std::vector<std::tuple<LolaServiceTypeDeployment, amp::string_view>> instance_id_to_hash_string_variations{
    {LolaServiceTypeDeployment{0U}, "0000"},
    {LolaServiceTypeDeployment{1U}, "0001"},
    {LolaServiceTypeDeployment{10U}, "000a"},
    {LolaServiceTypeDeployment{255U}, "00ff"},
    {LolaServiceTypeDeployment{std::numeric_limits<LolaServiceId>::max()}, "ffff"}};
INSTANTIATE_TEST_SUITE_P(LolaServiceTypeDeploymentHashFixture,
                         LolaServiceTypeDeploymentHashFixture,
                         ::testing::ValuesIn(instance_id_to_hash_string_variations));

}  // namespace
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
