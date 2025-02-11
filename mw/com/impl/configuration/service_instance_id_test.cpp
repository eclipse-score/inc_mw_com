// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#include "platform/aas/mw/com/impl/configuration/service_instance_id.h"
#include "platform/aas/mw/com/impl/configuration/lola_service_instance_id.h"
#include "platform/aas/mw/com/impl/configuration/someip_service_instance_id.h"

#include "platform/aas/mw/com/impl/configuration/test/configuration_test_resources.h"

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

TEST(ServiceInstanceIdTest, LolaBindingCanBeCopiedAndEqualCompared)
{
    const auto unit = ServiceInstanceId{LolaServiceInstanceId{10U}};
    const auto unitCopy = unit;

    ASSERT_EQ(unit, unitCopy);
}

TEST(ServiceInstanceIdTest, SomeIpBindingCanBeCopiedAndEqualCompared)
{
    const auto unit = ServiceInstanceId{SomeIpServiceInstanceId{10U}};
    const auto unitCopy = unit;

    ASSERT_EQ(unit, unitCopy);
}

TEST(ServiceInstanceIdTest, EmptyBindingCanBeCopiedAndEqualCompared)
{
    const auto unit = ServiceInstanceId{amp::blank{}};
    const auto unitCopy = unit;

    ASSERT_EQ(unit, unitCopy);
}

TEST(ServiceInstanceIdTest, ComparingDifferentBindingsReturnsFalse)
{
    const auto unit = ServiceInstanceId{LolaServiceInstanceId{10U}};
    const auto unit_2 = ServiceInstanceId{SomeIpServiceInstanceId{10U}};

    ASSERT_FALSE(unit == unit_2);
}

TEST(ServiceInstanceIdTest, LessThanOperatorSameBindings)
{
    const auto unit = ServiceInstanceId{LolaServiceInstanceId{10U}};
    const auto unit_2 = ServiceInstanceId{LolaServiceInstanceId{11U}};

    EXPECT_TRUE(unit < unit_2);
    EXPECT_FALSE(unit_2 < unit);
}

TEST(ServiceInstanceIdTest, LessThanOperatorDifferentBindingsIsAlwaysFalse)
{
    const auto unit = ServiceInstanceId{LolaServiceInstanceId{10U}};
    const auto unit_2 = ServiceInstanceId{SomeIpServiceInstanceId{11U}};

    EXPECT_FALSE(unit < unit_2);
    EXPECT_FALSE(unit_2 < unit);
}

using ServiceInstanceIdFixture = ConfigurationStructsFixture;
TEST_F(ServiceInstanceIdFixture, CanConstructFromLolaServiceInstanceId)
{
    LolaServiceInstanceId lola_service_instance_id{10U};
    ServiceInstanceId unit{lola_service_instance_id};

    const auto* const binding_ptr = amp::get_if<LolaServiceInstanceId>(&unit.binding_info_);
    ASSERT_NE(binding_ptr, nullptr);
    ExpectLolaServiceInstanceIdObjectsEqual(*binding_ptr, lola_service_instance_id);
}

TEST_F(ServiceInstanceIdFixture, CanConstructFromSomeIpServiceInstanceId)
{
    SomeIpServiceInstanceId someip_service_instance_id{10U};
    ServiceInstanceId unit{someip_service_instance_id};

    const auto* const binding_ptr = amp::get_if<SomeIpServiceInstanceId>(&unit.binding_info_);
    ASSERT_NE(binding_ptr, nullptr);
    ExpectSomeIpServiceInstanceIdObjectsEqual(*binding_ptr, someip_service_instance_id);
}

TEST_F(ServiceInstanceIdFixture, CanConstructFromBlankInstanceDeployment)
{
    ServiceInstanceId unit{amp::blank{}};

    const auto* const binding_ptr = amp::get_if<amp::blank>(&unit.binding_info_);
    ASSERT_NE(binding_ptr, nullptr);
}

TEST_F(ServiceInstanceIdFixture, CanCreateFromSerializedLolaObject)
{
    LolaServiceInstanceId lola_service_instance_deployment{LolaServiceInstanceId{10U}};

    ServiceInstanceId unit{lola_service_instance_deployment};

    const auto serialized_unit{unit.Serialize()};

    ServiceInstanceId reconstructed_unit{serialized_unit};

    ExpectServiceInstanceIdObjectsEqual(reconstructed_unit, unit);
}

TEST_F(ServiceInstanceIdFixture, CanCreateFromSerializedSomeIpObject)
{
    const std::uint16_t instance_id{123U};

    SomeIpServiceInstanceId someip_service_instance_deployment{instance_id};

    ServiceInstanceId unit{someip_service_instance_deployment};

    const auto serialized_unit{unit.Serialize()};

    ServiceInstanceId reconstructed_unit{serialized_unit};

    ExpectServiceInstanceIdObjectsEqual(reconstructed_unit, unit);
}

TEST_F(ServiceInstanceIdFixture, CanCreateFromSerializedBlankObject)
{
    ServiceInstanceId unit{amp::blank{}};

    const auto serialized_unit{unit.Serialize()};

    ServiceInstanceId reconstructed_unit{serialized_unit};
}

TEST(ServiceInstanceIdDeathTest, CreatingFromSerializedObjectWithMismatchedSerializationVersionTerminates)
{
    const ServiceInstanceId unit{LolaServiceInstanceId{10U}};

    const auto serialization_version_key = "serializationVersion";
    const std::uint32_t invalid_serialization_version = ServiceInstanceId::serializationVersion + 1;

    auto serialized_unit{unit.Serialize()};
    auto it = serialized_unit.find(serialization_version_key);
    ASSERT_NE(it, serialized_unit.end());
    it->second = json::Any{invalid_serialization_version};

    EXPECT_DEATH(ServiceInstanceId reconstructed_unit{serialized_unit}, ".*");
}

class ServiceInstanceIdHashFixture : public ::testing::TestWithParam<std::tuple<ServiceInstanceId, amp::string_view>>
{
};

TEST_P(ServiceInstanceIdHashFixture, ToHashString)
{
    const auto param_tuple = GetParam();

    const auto unit = std::get<ServiceInstanceId>(param_tuple);
    const auto expected_hash_string = std::get<amp::string_view>(param_tuple);

    const auto actual_hash_string = unit.ToHashString();

    EXPECT_EQ(actual_hash_string, expected_hash_string);
    EXPECT_EQ(actual_hash_string.size(), ServiceInstanceId::hashStringSize);
}

const std::vector<std::tuple<ServiceInstanceId, amp::string_view>> instance_id_to_hash_string_variations{
    {ServiceInstanceId{LolaServiceInstanceId{0U}}, "00000"},
    {ServiceInstanceId{LolaServiceInstanceId{1U}}, "00001"},
    {ServiceInstanceId{LolaServiceInstanceId{10U}}, "0000a"},
    {ServiceInstanceId{LolaServiceInstanceId{255U}}, "000ff"},
    {ServiceInstanceId{LolaServiceInstanceId{std::numeric_limits<LolaServiceInstanceId::InstanceId>::max()}}, "0ffff"},
    {ServiceInstanceId{SomeIpServiceInstanceId{0U}}, "10000"},
    {ServiceInstanceId{SomeIpServiceInstanceId{1U}}, "10001"},
    {ServiceInstanceId{SomeIpServiceInstanceId{10U}}, "1000a"},
    {ServiceInstanceId{SomeIpServiceInstanceId{255U}}, "100ff"},
    {ServiceInstanceId{SomeIpServiceInstanceId{std::numeric_limits<SomeIpServiceInstanceId::InstanceId>::max()}},
     "1ffff"}};
INSTANTIATE_TEST_SUITE_P(ServiceInstanceIdHashFixture,
                         ServiceInstanceIdHashFixture,
                         ::testing::ValuesIn(instance_id_to_hash_string_variations));

}  // namespace
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
