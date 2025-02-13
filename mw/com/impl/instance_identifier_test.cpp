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


#include "platform/aas/mw/com/impl/instance_identifier.h"

#include "platform/aas/mw/com/impl/com_error.h"
#include "platform/aas/mw/com/impl/configuration/test/configuration_test_resources.h"

#include "platform/aas/lib/json/json_writer.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <type_traits>

namespace bmw
{
namespace mw
{
namespace com
{
namespace impl
{

class InstanceIdentifierAttorney
{
  public:
    static void SetConfiguration(Configuration* const configuration) noexcept
    {
        InstanceIdentifier::configuration_ = configuration;
    }
};

class ConfigurationGuard
{
  public:
    ConfigurationGuard() { InstanceIdentifierAttorney::SetConfiguration(&configuration); }
    ~ConfigurationGuard() { InstanceIdentifierAttorney::SetConfiguration(nullptr); }

  private:
    Configuration configuration{Configuration::ServiceTypeDeployments{},
                                Configuration::ServiceInstanceDeployments{},
                                GlobalConfiguration{},
                                TracingConfiguration{}};
};

namespace
{

using ::testing::Return;
using ::testing::StrEq;

const auto kServiceTypeName1 = "/bla/blub/service1";
const auto kServiceTypeName2 = "/bla/blub/service2";
const auto kInstanceSpecifier1 = InstanceSpecifier::Create(kServiceTypeName1).value();
const auto kInstanceSpecifier2 = InstanceSpecifier::Create(kServiceTypeName2).value();
const auto kService1 = make_ServiceIdentifierType(kServiceTypeName1);
const auto kService2 = make_ServiceIdentifierType(kServiceTypeName2);
const LolaServiceInstanceDeployment kServiceInstanceDeployment1{LolaServiceInstanceId{16U}};
const LolaServiceInstanceDeployment kServiceInstanceDeployment2{LolaServiceInstanceId{17U}};
const ServiceTypeDeployment kTestTypeDeployment1{amp::blank{}};
const ServiceTypeDeployment kTestTypeDeployment2{LolaServiceTypeDeployment{18U}};

TEST(InstanceIdentifierTest, Copyable)
{
    RecordProperty("Verifies", "5");
    RecordProperty("Description", "Checks copy semantics for InstanceIdentifier");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("Priority", "1");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    static_assert(std::is_copy_constructible<InstanceIdentifier>::value, "Is not copy constructible");
    static_assert(std::is_copy_assignable<InstanceIdentifier>::value, "Is not copy assignable");
}

TEST(InstanceIdentifier, Moveable)
{
    RecordProperty("Verifies", "1");
    RecordProperty("Description", "Checks move semantics for InstanceIdentifier");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("Priority", "1");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    static_assert(std::is_move_constructible<InstanceIdentifier>::value, "Is not move constructible");
    static_assert(std::is_move_assignable<InstanceIdentifier>::value, "Is not move assignable");
}

TEST(InstanceIdentifierTest, constructable)
{
    make_InstanceIdentifier({kService1,
                             LolaServiceInstanceDeployment{LolaServiceInstanceId{16U}},
                             QualityType::kASIL_QM,
                             kInstanceSpecifier1},
                            kTestTypeDeployment1);
}

TEST(InstanceIdentifierTest, CanBeCopiedAndEqualCompared)
{
    RecordProperty("Verifies", "7");
    RecordProperty("Description", "Checks CopyAssignment operator and EqualComparableOperator");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("Priority", "1");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    const ServiceInstanceDeployment instance_deployment{kService1,
                                                        LolaServiceInstanceDeployment{LolaServiceInstanceId{16U}},
                                                        QualityType::kASIL_QM,
                                                        kInstanceSpecifier1};

    const auto unit = make_InstanceIdentifier(instance_deployment, kTestTypeDeployment1);
    const auto unitCopy = unit;

    ASSERT_EQ(unit, unitCopy);
}

TEST(InstanceIdentifierTest, LessCompareable)
{
    RecordProperty("Verifies", "7");
    RecordProperty("Description", "Checks LessComparableOperator");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("Priority", "1");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    const ServiceInstanceDeployment instance_deployment_1{kService1,
                                                          LolaServiceInstanceDeployment{LolaServiceInstanceId{16U}},
                                                          QualityType::kASIL_QM,
                                                          kInstanceSpecifier1};
    const ServiceInstanceDeployment instance_deployment_2{kService2,
                                                          LolaServiceInstanceDeployment{LolaServiceInstanceId{16U}},
                                                          QualityType::kASIL_QM,
                                                          kInstanceSpecifier1};

    const auto unit = make_InstanceIdentifier(instance_deployment_2, kTestTypeDeployment1);
    const auto less = make_InstanceIdentifier(instance_deployment_1, kTestTypeDeployment1);

    ASSERT_LT(less, unit);
}

TEST(InstanceIdentifierViewTest, ExposeDeploymentInformation)
{
    ServiceInstanceDeployment deployment{kService1,
                                         LolaServiceInstanceDeployment{LolaServiceInstanceId{1U}},
                                         QualityType::kASIL_QM,
                                         kInstanceSpecifier1};
    const auto identifier = make_InstanceIdentifier(deployment, kTestTypeDeployment1);
    const auto unit = InstanceIdentifierView{identifier};

    EXPECT_EQ(unit.GetServiceInstanceDeployment(), deployment);
}

TEST(InstanceIdentifierViewTest, SameInstanceIsCompatible)
{
    ServiceInstanceDeployment deployment{kService1,
                                         LolaServiceInstanceDeployment{LolaServiceInstanceId{1U}},
                                         QualityType::kASIL_QM,
                                         kInstanceSpecifier1};
    const auto identifier = make_InstanceIdentifier(deployment, kTestTypeDeployment1);
    const auto unit = InstanceIdentifierView{identifier};

    EXPECT_TRUE(unit.isCompatibleWith(identifier));
}

TEST(InstanceIdentifierViewTest, SameInstanceViewIsCompatible)
{
    ServiceInstanceDeployment deployment{kService1,
                                         LolaServiceInstanceDeployment{LolaServiceInstanceId{1U}},
                                         QualityType::kASIL_QM,
                                         kInstanceSpecifier1};
    const auto identifier = make_InstanceIdentifier(deployment, kTestTypeDeployment1);
    const auto unit = InstanceIdentifierView{identifier};

    EXPECT_TRUE(unit.isCompatibleWith(unit));
}

using InstanceIdentifierFixture = ConfigurationStructsFixture;
TEST_F(InstanceIdentifierFixture, CanCreateFromSerializedObject)
{
    RecordProperty("Verifies", "7, 2");
    RecordProperty("Description",
                   "Checks creating InstanceIdentifier from valid serialized InstanceIdentifier from "
                   "InstanceIdentifier::ToString");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("Priority", "1");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    ConfigurationGuard configuration_guard{};

    const ServiceIdentifierType dummy_service = make_ServiceIdentifierType("foo", 1U, 0U);

    const ServiceInstanceDeployment service_instance_deployment{
        dummy_service, MakeLolaServiceInstanceDeployment(), QualityType::kASIL_B, kInstanceSpecifier1};

    const ServiceTypeDeployment service_type_deployment{MakeLolaServiceTypeDeployment()};

    const auto identifier = make_InstanceIdentifier(service_instance_deployment, service_type_deployment);

    const auto serialized_identifier{identifier.ToString()};

    const auto reconstructed_identifier_result = InstanceIdentifier::Create(serialized_identifier);
    ASSERT_TRUE(reconstructed_identifier_result.has_value());
    const auto& reconstructed_identifier = reconstructed_identifier_result.value();

    ExpectServiceInstanceDeploymentObjectsEqual(
        InstanceIdentifierView{reconstructed_identifier}.GetServiceInstanceDeployment(),
        InstanceIdentifierView{identifier}.GetServiceInstanceDeployment());
    ExpectServiceTypeDeploymentObjectsEqual(InstanceIdentifierView{reconstructed_identifier}.GetServiceTypeDeployment(),
                                            InstanceIdentifierView{identifier}.GetServiceTypeDeployment());
}

TEST_F(InstanceIdentifierFixture, CreatingFromInvalidSerializedObjectReturnsError)
{
    RecordProperty("Verifies", "7");
    RecordProperty("Description", "Checks creating InstanceIdentifier from invalid serialized InstanceIdentifier");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("Priority", "1");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    ConfigurationGuard configuration_guard{};

    const std::string invalid_serialized_identifier{"invalid identifier"};
    const auto reconstructed_identifier_result = InstanceIdentifier::Create(invalid_serialized_identifier);
    ASSERT_FALSE(reconstructed_identifier_result.has_value());
    EXPECT_EQ(reconstructed_identifier_result.error(), ComErrc::kInvalidInstanceIdentifierString);
}

TEST_F(InstanceIdentifierFixture, CreatingWithoutInitializingConfigurationReturnsError)
{
    const std::string serialized_identifier{""};
    const auto reconstructed_identifier_result = InstanceIdentifier::Create(serialized_identifier);
    ASSERT_FALSE(reconstructed_identifier_result.has_value());
    EXPECT_EQ(reconstructed_identifier_result.error(), ComErrc::kInvalidConfiguration);
}

TEST(InstanceIdentifierDeathTest, CreatingFromSerializedObjectWithMismatchedSerializationVersionTerminates)
{
    ConfigurationGuard configuration_guard{};

    const ServiceInstanceDeployment instance_deployment{
        kService1, MakeLolaServiceInstanceDeployment(), QualityType::kASIL_QM, kInstanceSpecifier1};

    const auto unit = make_InstanceIdentifier(instance_deployment, kTestTypeDeployment1);

    const auto serialization_version_key = "serializationVersion";
    const std::uint32_t invalid_serialization_version = InstanceIdentifierView::GetSerializationVersion() + 1;

    auto serialized_unit{InstanceIdentifierView{unit}.Serialize()};
    auto it = serialized_unit.find(serialization_version_key);
    ASSERT_NE(it, serialized_unit.end());
    it->second = json::Any{invalid_serialization_version};

    bmw::json::JsonWriter writer{};
    const std::string serialized_string_form = writer.ToBuffer(serialized_unit).value();

    EXPECT_DEATH(InstanceIdentifier::Create(serialized_string_form), ".*");
}

TEST_F(InstanceIdentifierFixture, HashInstanceIdentifierComparesEqual)
{
    ServiceInstanceDeployment deployment{kService1,
                                         LolaServiceInstanceDeployment{LolaServiceInstanceId{1U}},
                                         QualityType::kASIL_QM,
                                         kInstanceSpecifier1};
    const auto identifier_1 = make_InstanceIdentifier(deployment, kTestTypeDeployment1);
    const auto identifier_2 = identifier_1;
    const auto hash_1 = std::hash<InstanceIdentifier>{}.operator()(identifier_1);
    const auto hash_2 = std::hash<InstanceIdentifier>{}.operator()(identifier_2);
    EXPECT_EQ(hash_1, hash_2);
}

class InstanceIdentifierHashFixture : public ::testing::TestWithParam<std::array<InstanceIdentifier, 2>>
{
};

TEST_P(InstanceIdentifierHashFixture, HashesOfDifferentInstanceIdentifiersCompareInequal)
{
    const auto identifiers = GetParam();
    const auto identifier_1 = identifiers.at(0);
    const auto identifier_2 = identifiers.at(1);

    const auto hash_1 = std::hash<InstanceIdentifier>{}.operator()(identifier_1);
    const auto hash_2 = std::hash<InstanceIdentifier>{}.operator()(identifier_2);
    EXPECT_NE(hash_1, hash_2);
}

// Test that each element that should be used in the hashing algorithm is used by changing them one at a time.
INSTANTIATE_TEST_SUITE_P(
    InstanceIdentifierHashDifferentKeys,
    InstanceIdentifierHashFixture,
    ::testing::Values(
        std::array<InstanceIdentifier, 2>{make_InstanceIdentifier(ServiceInstanceDeployment{kService1,
                                                                                            kServiceInstanceDeployment1,
                                                                                            QualityType::kASIL_QM,
                                                                                            kInstanceSpecifier1},
                                                                  kTestTypeDeployment1),
                                          make_InstanceIdentifier(ServiceInstanceDeployment{kService2,
                                                                                            kServiceInstanceDeployment1,
                                                                                            QualityType::kASIL_QM,
                                                                                            kInstanceSpecifier1},
                                                                  kTestTypeDeployment1)},

        std::array<InstanceIdentifier, 2>{make_InstanceIdentifier(ServiceInstanceDeployment{kService1,
                                                                                            kServiceInstanceDeployment1,
                                                                                            QualityType::kASIL_QM,
                                                                                            kInstanceSpecifier1},
                                                                  kTestTypeDeployment1),
                                          make_InstanceIdentifier(ServiceInstanceDeployment{kService1,
                                                                                            kServiceInstanceDeployment2,
                                                                                            QualityType::kASIL_QM,
                                                                                            kInstanceSpecifier1},
                                                                  kTestTypeDeployment1)},

        std::array<InstanceIdentifier, 2>{make_InstanceIdentifier(ServiceInstanceDeployment{kService1,
                                                                                            kServiceInstanceDeployment1,
                                                                                            QualityType::kASIL_QM,
                                                                                            kInstanceSpecifier1},
                                                                  kTestTypeDeployment1),
                                          make_InstanceIdentifier(ServiceInstanceDeployment{kService1,
                                                                                            kServiceInstanceDeployment1,
                                                                                            QualityType::kASIL_B,
                                                                                            kInstanceSpecifier1},
                                                                  kTestTypeDeployment1)},

        std::array<InstanceIdentifier, 2>{make_InstanceIdentifier(ServiceInstanceDeployment{kService1,
                                                                                            kServiceInstanceDeployment1,
                                                                                            QualityType::kASIL_QM,
                                                                                            kInstanceSpecifier1},
                                                                  kTestTypeDeployment1),
                                          make_InstanceIdentifier(ServiceInstanceDeployment{kService1,
                                                                                            kServiceInstanceDeployment1,
                                                                                            QualityType::kASIL_QM,
                                                                                            kInstanceSpecifier2},
                                                                  kTestTypeDeployment1)},

        std::array<InstanceIdentifier, 2>{make_InstanceIdentifier(ServiceInstanceDeployment{kService1,
                                                                                            kServiceInstanceDeployment1,
                                                                                            QualityType::kASIL_QM,
                                                                                            kInstanceSpecifier1},
                                                                  kTestTypeDeployment1),
                                          make_InstanceIdentifier(ServiceInstanceDeployment{kService1,
                                                                                            kServiceInstanceDeployment1,
                                                                                            QualityType::kASIL_QM,
                                                                                            kInstanceSpecifier1},
                                                                  kTestTypeDeployment2)}));

}  // namespace
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
