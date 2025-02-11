// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#include "platform/aas/mw/com/impl/instance_specifier.h"

#include "platform/aas/mw/com/impl/com_error.h"

#include <amp_string_view.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <type_traits>
#include <unordered_map>

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

TEST(InstanceSpecifierTest, Copyable)
{
    RecordProperty("Verifies", "2");
    RecordProperty("Description", "Checks copy semantics for InstanceSpecifier");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("Priority", "1");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    static_assert(std::is_copy_constructible<InstanceSpecifier>::value, "Is not copyable");
    static_assert(std::is_copy_assignable<InstanceSpecifier>::value, "Is not copyable");
}

TEST(InstanceSpecifierTest, Moveable)
{
    RecordProperty("Verifies", "8");
    RecordProperty("Description", "Checks move semantics for InstanceSpecifier");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("Priority", "1");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    static_assert(std::is_move_constructible<InstanceSpecifier>::value, "Is not copyable");
    static_assert(std::is_move_assignable<InstanceSpecifier>::value, "Is not copyable");
}

TEST(InstanceSpecifierComparisonOperatorTest, EqualityOperatorForTwoInstanceSpecifiers)
{
    RecordProperty("Verifies", "4");
    RecordProperty("Description", "Checks equality operator for two InstanceSpecifiers");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("Priority", "1");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    // Given 2 valid instance specifier strings with the same value
    const auto valid_instance_specifier_string = "/good/instance/specifier";
    const auto valid_instance_specifier_string_2 = "/good/instance/specifier";

    // When creating 2 instance specifiers from each string
    const auto instance_specifier_result = InstanceSpecifier::Create(valid_instance_specifier_string);
    ASSERT_TRUE(instance_specifier_result.has_value());
    const InstanceSpecifier instance_specifier = std::move(instance_specifier_result).value();

    const auto instance_specifier_result_2 = InstanceSpecifier::Create(valid_instance_specifier_string_2);
    ASSERT_TRUE(instance_specifier_result_2.has_value());
    const InstanceSpecifier instance_specifier_2 = std::move(instance_specifier_result_2).value();

    // Then the 2 instance specifiers should be equal
    EXPECT_EQ(instance_specifier, instance_specifier_2);
}

TEST(InstanceSpecifierComparisonOperatorTest, EqualityOperatorForInstanceSpecifierAndStringView)
{
    RecordProperty("Verifies", "4");
    RecordProperty("Description", "Checks equality operator for an InstanceSpecifier and an amp::string_view");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("Priority", "1");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    // Given 2 valid instance specifier strings with the same value
    const auto valid_instance_specifier_string = "/good/instance/specifier";
    const auto valid_instance_specifier_string_2 = "/good/instance/specifier";

    // When creating an instance specifier from one string
    const auto instance_specifier_result = InstanceSpecifier::Create(valid_instance_specifier_string);
    ASSERT_TRUE(instance_specifier_result.has_value());
    const InstanceSpecifier instance_specifier = std::move(instance_specifier_result).value();

    // Then the instance specifier should be equal to the other string_view, regardless of the order
    EXPECT_EQ(instance_specifier, amp::string_view{valid_instance_specifier_string_2});
    EXPECT_EQ(amp::string_view{valid_instance_specifier_string_2}, instance_specifier);
}

TEST(InstanceSpecifierComparisonOperatorTest, InequalityOperatorForTwoInstanceSpecifiers)
{
    RecordProperty("Verifies", "4");
    RecordProperty("Description", "Checks inequality operator for two InstanceSpecifiers");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("Priority", "1");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    // Given 2 a valid instance specifier strings
    const auto valid_instance_specifier_string = "/good/instance/specifier";
    const auto valid_instance_specifier_string_2 = "/good/instance/specifier2";

    // When creating an instance specifier from each string
    const auto instance_specifier_result = InstanceSpecifier::Create(valid_instance_specifier_string);
    ASSERT_TRUE(instance_specifier_result.has_value());
    const InstanceSpecifier instance_specifier = std::move(instance_specifier_result).value();

    const auto instance_specifier_result_2 = InstanceSpecifier::Create(valid_instance_specifier_string_2);
    ASSERT_TRUE(instance_specifier_result_2.has_value());
    const InstanceSpecifier instance_specifier_2 = std::move(instance_specifier_result_2).value();

    // Then the 2 instance specifiers should not be equal
    EXPECT_NE(instance_specifier, instance_specifier_2);
}

TEST(InstanceSpecifierComparisonOperatorTest, InequalityOperatorForInstanceSpecifierAndStringView)
{
    RecordProperty("Verifies", "4");
    RecordProperty("Description", "Checks inequality operator for an InstanceSpecifier and an amp::string_view");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("Priority", "1");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    // Given 2 a valid instance specifier strings
    const auto valid_instance_specifier_string = "/good/instance/specifier";
    const auto valid_instance_specifier_string_2 = "/good/instance/specifier2";

    // When creating an instance specifier from the first string
    const auto instance_specifier_result = InstanceSpecifier::Create(valid_instance_specifier_string);
    ASSERT_TRUE(instance_specifier_result.has_value());
    const InstanceSpecifier instance_specifier = std::move(instance_specifier_result).value();

    // Then the instance specifier should not be equal to the string_view, regardless of the order
    EXPECT_NE(instance_specifier, amp::string_view{valid_instance_specifier_string_2});
    EXPECT_NE(amp::string_view{valid_instance_specifier_string_2}, instance_specifier);
}

TEST(InstanceSpecifierComparisonOperatorTest, LessThanOperatorForTwoInstanceSpecifiers)
{
    RecordProperty("Verifies", "4");
    RecordProperty("Description", "Checks less than operator for two InstanceSpecifiers");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("Priority", "1");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    // Given 2 a valid instance specifier strings
    const auto valid_instance_specifier_string = "/good/instance/specifier";
    const auto valid_instance_specifier_string_2 = "/good/instance/specifier2";

    // When creating an instance specifier from each string
    const auto instance_specifier_result = InstanceSpecifier::Create(valid_instance_specifier_string);
    ASSERT_TRUE(instance_specifier_result.has_value());
    const InstanceSpecifier instance_specifier = std::move(instance_specifier_result).value();

    const auto instance_specifier_result_2 = InstanceSpecifier::Create(valid_instance_specifier_string_2);
    ASSERT_TRUE(instance_specifier_result_2.has_value());
    const InstanceSpecifier instance_specifier_2 = std::move(instance_specifier_result_2).value();

    // Then the comparison operator should compare the underlying strings
    const bool string_comparison_result =
        amp::string_view{valid_instance_specifier_string} < amp::string_view{valid_instance_specifier_string_2};
    const bool instance_specifier_comparison_result = instance_specifier < instance_specifier_2;
    EXPECT_EQ(string_comparison_result, instance_specifier_comparison_result);
}

TEST(InstanceSpecifierComparisonOperatorTest, HashOperatorForDifferentUnderlyingStringsAreDifferent)
{
    RecordProperty("Verifies", "0");
    RecordProperty("Description",
                   "Checks the hash for InstanceSpecifiers with different underlying string are different");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("Priority", "1");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    // Given 2 a valid instance specifier strings
    const auto valid_instance_specifier_string = "/good/instance/specifier";
    const auto valid_instance_specifier_string_2 = "/good/instance/specifier2";

    // When creating an instance specifier from each string
    const auto instance_specifier_result = InstanceSpecifier::Create(valid_instance_specifier_string);
    ASSERT_TRUE(instance_specifier_result.has_value());
    const InstanceSpecifier instance_specifier = std::move(instance_specifier_result).value();

    const auto instance_specifier_result_2 = InstanceSpecifier::Create(valid_instance_specifier_string_2);
    ASSERT_TRUE(instance_specifier_result_2.has_value());
    const InstanceSpecifier instance_specifier_2 = std::move(instance_specifier_result_2).value();

    // Then the hash of each InstanceSpecifier should be different
    const auto hash_value = std::hash<InstanceSpecifier>{}(instance_specifier);
    const auto hash_value_2 = std::hash<InstanceSpecifier>{}(instance_specifier_2);
    EXPECT_NE(hash_value, hash_value_2);
}

TEST(InstanceSpecifierHashTest, HashOperatorForTheSameUnderlyingStringIsTheSame)
{
    RecordProperty("Verifies", "0");
    RecordProperty("Description",
                   "Checks the hash for InstanceSpecifiers with the same underlying string are the same");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("Priority", "1");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    // Given 2 valid instance specifier strings with the same value
    const auto valid_instance_specifier_string = "/good/instance/specifier";
    const auto valid_instance_specifier_string_2 = "/good/instance/specifier";

    // When creating 2 instance specifiers from each string
    const auto instance_specifier_result = InstanceSpecifier::Create(valid_instance_specifier_string);
    ASSERT_TRUE(instance_specifier_result.has_value());
    const InstanceSpecifier instance_specifier = std::move(instance_specifier_result).value();

    const auto instance_specifier_result_2 = InstanceSpecifier::Create(valid_instance_specifier_string_2);
    ASSERT_TRUE(instance_specifier_result_2.has_value());
    const InstanceSpecifier instance_specifier_2 = std::move(instance_specifier_result_2).value();

    // Then the hash of each InstanceSpecifier should be the same
    const auto hash_value = std::hash<InstanceSpecifier>{}(instance_specifier);
    const auto hash_value_2 = std::hash<InstanceSpecifier>{}(instance_specifier_2);
    EXPECT_EQ(hash_value, hash_value_2);
}

TEST(InstanceSpecifierHashTest, InstanceSpecifierCanBeKeyForStlContainer)
{
    RecordProperty("Verifies", "0");
    RecordProperty("Description", "Checks the InstanceSpecifier can be used as key in STL container");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("Priority", "1");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    // Given a valid instance specifier string
    const auto valid_instance_specifier_string = "/good/instance/specifier";

    // When creating an instance specifier from the string
    const auto instance_specifier_result = InstanceSpecifier::Create(valid_instance_specifier_string);
    ASSERT_TRUE(instance_specifier_result.has_value());
    const InstanceSpecifier instance_specifier = std::move(instance_specifier_result).value();

    // When using the InstanceSpecifier as a key in an STL container
    std::unordered_map<InstanceSpecifier, int> test_map{{instance_specifier, 10}};

    // Then we should compile and shouldn't crash
}

TEST(InstanceSpecifierToStringTest, ToStringWillReturnTheUnderlyingString)
{
    RecordProperty("Verifies", "0");
    RecordProperty("Description", "Checks the ToString should return the underlying string");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("Priority", "1");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    // Given a valid instance specifier string
    const auto valid_instance_specifier_string = "/good/instance/specifier";

    // When creating an instance specifier from the string
    const auto instance_specifier_result = InstanceSpecifier::Create(valid_instance_specifier_string);
    ASSERT_TRUE(instance_specifier_result.has_value());
    const InstanceSpecifier instance_specifier = std::move(instance_specifier_result).value();

    // Then the value returned by ToString should be the same as the underlying string
    EXPECT_EQ(instance_specifier.ToString(), amp::string_view{valid_instance_specifier_string});
}

class InstanceSpecifierCanConstructFromValidStringFixture : public ::testing::TestWithParam<amp::string_view>
{
};

TEST_P(InstanceSpecifierCanConstructFromValidStringFixture, CanConstructFromValidString)
{
    RecordProperty("Verifies", "8");
    RecordProperty("Description", "Checks that an InstanceSpecifier can be created from a valid shortname path.");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("Priority", "1");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    // Given a valid instance specifier string
    const amp::string_view instance_specifier_string = GetParam();

    const auto instance_specifier_result = InstanceSpecifier::Create(instance_specifier_string);
    ASSERT_TRUE(instance_specifier_result.has_value());
}

INSTANTIATE_TEST_SUITE_P(InstanceSpecifierCanConstructFromValidStringTest,
                         InstanceSpecifierCanConstructFromValidStringFixture,
                         ::testing::Values("good/instance_specifier/123",
                                           "Good/Instance_specifier/with/caps/123",
                                           "_Good/Instance_specifier/123",
                                           "/Good/Instance_specifier/123",
                                           "g",
                                           "G",
                                           "Good"));

class InstanceSpecifierCannotConstructFromInvalidStringFixture : public ::testing::TestWithParam<amp::string_view>
{
};

TEST_P(InstanceSpecifierCannotConstructFromInvalidStringFixture, ConstructingFromInvalidStringReturnsError)
{
    RecordProperty("Verifies", "8");
    RecordProperty(
        "Description",
        "Checks that trying to create an InstanceSpecifier from an invalid shortname path returns an error.");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("Priority", "1");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    const amp::string_view instance_specifier_string = GetParam();
    const auto instance_specifier_result = InstanceSpecifier::Create(instance_specifier_string);
    ASSERT_FALSE(instance_specifier_result.has_value());
    EXPECT_EQ(instance_specifier_result.error(), ComErrc::kInvalidMetaModelShortname);
}

INSTANTIATE_TEST_SUITE_P(InstanceSpecifierCannotConstructFromInvalidStringTest,
                         InstanceSpecifierCannotConstructFromInvalidStringFixture,
                         ::testing::Values("1bad/instance_specifier/123",
                                           "bad/instance specifier/123",
                                           "bad/instance@specifier/123",
                                           "bad/instance!specifier/123",
                                           "bad/instance_specifier/123/",
                                           "//bad/instance_specifier//123",
                                           "bad/instance_specifier//123"));

}  // namespace
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
