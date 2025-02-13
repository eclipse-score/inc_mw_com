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


#include "platform/aas/mw/com/impl/configuration/lola_event_instance_deployment.h"

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

using LolaEventInstanceDeploymentFixture = ConfigurationStructsFixture;
TEST_F(LolaEventInstanceDeploymentFixture, CanCreateFromSerializedObjectWithOptionals)
{
    LolaEventInstanceDeployment unit{MakeLolaEventInstanceDeployment()};

    const auto serialized_unit{unit.Serialize()};

    LolaEventInstanceDeployment reconstructed_unit{serialized_unit};

    ExpectLolaEventInstanceDeploymentObjectsEqual(reconstructed_unit, unit);
}

TEST_F(LolaEventInstanceDeploymentFixture, CanCreateFromSerializedObjectWithoutOptionals)
{
    const std::uint16_t number_of_sample_slots{};
    const amp::optional<std::uint8_t> max_subscribers{13};
    const amp::optional<std::uint8_t> max_concurrent_allocations{};
    const amp::optional<bool> enforce_max_samples{};
    const bool is_tracing_enabled{false};

    LolaEventInstanceDeployment unit{MakeLolaEventInstanceDeployment(
        number_of_sample_slots, max_subscribers, max_concurrent_allocations, enforce_max_samples, is_tracing_enabled)};

    const auto serialized_unit{unit.Serialize()};

    LolaEventInstanceDeployment reconstructed_unit{serialized_unit};

    ExpectLolaEventInstanceDeploymentObjectsEqual(reconstructed_unit, unit);
}

TEST(LolaEventInstanceDeploymentDeathTest, CreatingFromSerializedObjectWithMismatchedSerializationVersionTerminates)
{
    LolaEventInstanceDeployment unit{MakeLolaEventInstanceDeployment()};

    const auto serialization_version_key = "serializationVersion";
    const std::uint32_t invalid_serialization_version = LolaEventInstanceDeployment::serializationVersion + 1;

    auto serialized_unit{unit.Serialize()};
    auto it = serialized_unit.find(serialization_version_key);
    ASSERT_NE(it, serialized_unit.end());
    it->second = json::Any{invalid_serialization_version};

    EXPECT_DEATH(LolaEventInstanceDeployment reconstructed_unit{serialized_unit}, ".*");
}

TEST(LolaEventInstanceDeploymentEqualityTest, EqualityOperatorForEqualStructs)
{
    const std::uint16_t number_of_sample_slots{};
    const amp::optional<std::uint8_t> max_subscribers{13};
    const amp::optional<std::uint8_t> max_concurrent_allocations{};
    const amp::optional<bool> enforce_max_samples{};
    const bool is_tracing_enabled{false};

    LolaEventInstanceDeployment unit{MakeLolaEventInstanceDeployment(
        number_of_sample_slots, max_subscribers, max_concurrent_allocations, enforce_max_samples, is_tracing_enabled)};
    LolaEventInstanceDeployment unit_2{MakeLolaEventInstanceDeployment(
        number_of_sample_slots, max_subscribers, max_concurrent_allocations, enforce_max_samples, is_tracing_enabled)};

    EXPECT_TRUE(unit == unit_2);
}

class LolaEventInstanceDeploymentEqualityFixture
    : public ::testing::TestWithParam<std::pair<LolaEventInstanceDeployment, LolaEventInstanceDeployment>>
{
};

TEST_P(LolaEventInstanceDeploymentEqualityFixture, EqualityOperatorForUnequalStructs)
{
    const auto param_pair = GetParam();

    const auto unit = param_pair.first;
    const auto unit_2 = param_pair.second;

    EXPECT_FALSE(unit == unit_2);
}

INSTANTIATE_TEST_SUITE_P(
    LolaEventInstanceDeploymentEqualityFixture,
    LolaEventInstanceDeploymentEqualityFixture,
    ::testing::ValuesIn({std::make_pair(LolaEventInstanceDeployment{}, LolaEventInstanceDeployment{1}),
                         std::make_pair(LolaEventInstanceDeployment{10U, 11U, 12U, true, true},
                                        LolaEventInstanceDeployment{11U, 11U, 12U, true, true}),
                         std::make_pair(LolaEventInstanceDeployment{10U, 11U, 12U, true, true},
                                        LolaEventInstanceDeployment{10U, 12U, 12U, true, true}),
                         std::make_pair(LolaEventInstanceDeployment{10U, 11U, 12U, true, true},
                                        LolaEventInstanceDeployment{10U, 11U, 13U, true, true}),
                         std::make_pair(LolaEventInstanceDeployment{10U, 11U, 12U, true, true},
                                        LolaEventInstanceDeployment{10U, 11U, 12U, false, true}),
                         std::make_pair(LolaEventInstanceDeployment{10U, 11U, 12U, true, true},
                                        LolaEventInstanceDeployment{10U, 11U, 12U, true, false})}));

TEST(LolaEventInstanceDeploymentGetSlotsTest, GetNumberOfSampleSlotsExcludingTracingSlotReturnOptionalByDefault)
{
    LolaEventInstanceDeployment unit{};

    EXPECT_FALSE(unit.GetNumberOfSampleSlots().has_value());
    EXPECT_FALSE(unit.GetNumberOfSampleSlotsExcludingTracingSlot().has_value());
}

TEST(LolaEventInstanceDeploymentGetSlotsTracingEnabledTest, GetNumberOfSampleSlotsReturnsSetValue)
{
    LolaEventInstanceDeployment unit{};
    unit.SetTracingEnabled(true);

    const std::uint16_t set_number_of_sample_slots{10U};
    unit.SetNumberOfSampleSlots(set_number_of_sample_slots);

    const auto number_of_sample_slots = unit.GetNumberOfSampleSlots();
    ASSERT_TRUE(number_of_sample_slots.has_value());
    EXPECT_EQ(number_of_sample_slots.value(), set_number_of_sample_slots + 1);

    const auto number_of_sample_slots_excluding_tracing = unit.GetNumberOfSampleSlotsExcludingTracingSlot();
    ASSERT_TRUE(number_of_sample_slots_excluding_tracing.has_value());
    EXPECT_EQ(number_of_sample_slots_excluding_tracing.value(), set_number_of_sample_slots);
}

TEST(LolaEventInstanceDeploymentGetSlotsTracingDisabledTest, GetNumberOfSampleSlotsReturnsSetValue)
{
    LolaEventInstanceDeployment unit{};
    unit.SetTracingEnabled(false);

    const std::uint16_t set_number_of_sample_slots{10U};
    unit.SetNumberOfSampleSlots(set_number_of_sample_slots);

    const auto number_of_sample_slots = unit.GetNumberOfSampleSlots();
    ASSERT_TRUE(number_of_sample_slots.has_value());
    EXPECT_EQ(number_of_sample_slots.value(), set_number_of_sample_slots);

    const auto number_of_sample_slots_excluding_tracing = unit.GetNumberOfSampleSlotsExcludingTracingSlot();
    ASSERT_TRUE(number_of_sample_slots_excluding_tracing.has_value());
    EXPECT_EQ(number_of_sample_slots_excluding_tracing.value(), set_number_of_sample_slots);
}

}  // namespace
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
