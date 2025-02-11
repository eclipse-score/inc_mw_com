// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#include "platform/aas/mw/com/impl/plumbing/skeleton_event_binding_factory.h"
#include "platform/aas/mw/com/impl/bindings/mock_binding/skeleton_event.h"
#include "platform/aas/mw/com/impl/configuration/lola_service_instance_deployment.h"
#include "platform/aas/mw/com/impl/plumbing/dummy_instance_identifier_builder.h"
#include "platform/aas/mw/com/impl/plumbing/skeleton_binding_factory.h"

#include <amp_variant.hpp>

#include <gtest/gtest.h>
#include <string>
#include <utility>

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

const auto kEventName{"Event1"};

class FakeSkeleton : public SkeletonBase
{
  public:
    FakeSkeleton(InstanceIdentifier instance_id)
        : SkeletonBase{SkeletonBindingFactory::Create(instance_id), instance_id}
    {
    }
};

class SkeletonEventBindingFactoryFixture : public ::testing::Test
{
  protected:
    using SampleType = std::uint8_t;

    test::DummyInstanceIdentifierBuilder instance_identifier_builder_;
};

TEST(SkeletonEventBindingFactory, CanConstructEvent)
{
    RecordProperty("Verifies", "1, 2, ");
    RecordProperty("Description", "Checks whether a skeleton event lola binding can be created and set at runtime");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("Priority", "1");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    // Given a fake skeleton that uses LoLa
    const auto service = make_ServiceIdentifierType("foo", 1U, 0U);
    ServiceTypeDeployment type_deployment{LolaServiceTypeDeployment{1U}};
    auto* const lola_type_deployment = amp::get_if<LolaServiceTypeDeployment>(&type_deployment.binding_info_);
    ASSERT_NE(lola_type_deployment, nullptr);

    lola_type_deployment->events_[kEventName] = 1U;

    auto instance_specifier_result = InstanceSpecifier::Create("/my_dummy_instance_specifier");
    auto lola_service_instance = LolaServiceInstanceDeployment{LolaServiceInstanceId{16U}};
    lola_service_instance.events_[kEventName] = LolaEventInstanceDeployment{};
    lola_service_instance.events_[kEventName].SetNumberOfSampleSlots(1);
    lola_service_instance.events_[kEventName].SetMaxSubscribers(3);
    const ServiceInstanceDeployment instance_deployment{
        service, lola_service_instance, QualityType::kASIL_QM, std::move(instance_specifier_result).value()};

    auto identifier = make_InstanceIdentifier(instance_deployment, type_deployment);
    FakeSkeleton parent_skeleton{identifier};

    // When constructing a event for that
    const auto unit = SkeletonEventBindingFactory<std::uint8_t>::Create(identifier, parent_skeleton, kEventName);

    // Then it is possible to construct an event
    EXPECT_NE(unit, nullptr);
    EXPECT_NE(dynamic_cast<lola::SkeletonEvent<std::uint8_t>*>(unit.get()), nullptr);
}

TEST_F(SkeletonEventBindingFactoryFixture, CannotConstructEventFromSomeIpBinding)
{
    // Given a fake skeleton that uses a someip binding
    auto identifier = instance_identifier_builder_.CreateSomeIpBindingInstanceIdentifier();
    FakeSkeleton parent_skeleton{identifier};

    // When constructing a event for that
    const auto unit = SkeletonEventBindingFactory<std::uint8_t>::Create(identifier, parent_skeleton, kEventName);

    // Then it is not possible to construct an event
    EXPECT_EQ(unit, nullptr);
}

TEST_F(SkeletonEventBindingFactoryFixture, CannotConstructEventFromBlankBinding)
{
    // Given a fake skeleton that uses a blank binding
    auto identifier = instance_identifier_builder_.CreateBlankBindingInstanceIdentifier();
    FakeSkeleton parent_skeleton{identifier};

    // When constructing a event for that
    const auto unit = SkeletonEventBindingFactory<std::uint8_t>::Create(identifier, parent_skeleton, kEventName);

    // Then it is not possible to construct an event
    EXPECT_EQ(unit, nullptr);
}

}  // namespace
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
