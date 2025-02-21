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


#include "platform/aas/mw/com/impl/plumbing/skeleton_binding_factory.h"
#include "platform/aas/mw/com/impl/bindings/lola/skeleton.h"
#include "platform/aas/mw/com/impl/bindings/mock_binding/skeleton.h"
#include "platform/aas/mw/com/impl/plumbing/dummy_instance_identifier_builder.h"

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

class SkeletonBindingFactoryFixture : public ::testing::Test
{
  public:
    test::DummyInstanceIdentifierBuilder instance_identifier_builder_;
};

TEST_F(SkeletonBindingFactoryFixture, CanCreateLoLaBinding)
{
    RecordProperty("Verifies", "1, 2, , ");
    RecordProperty("Description", "Checks whether a skeleton lola binding can be created and set at runtime");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("Priority", "1");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    // Given an InstanceIdentifier with a LoLa binding
    auto instance_id = instance_identifier_builder_.CreateValidLolaInstanceIdentifier();

    // When creating the binding
    auto unit = SkeletonBindingFactory::Create(instance_id);

    // That it is not null and a LoLa Skeleton
    EXPECT_NE(unit, nullptr);
    EXPECT_NE(dynamic_cast<lola::Skeleton*>(unit.get()), nullptr);
}

TEST_F(SkeletonBindingFactoryFixture, CanNotCreateOtherBinding)
{
    // Given an InstanceIdentifier with a SomeIp binding
    auto instance_id = instance_identifier_builder_.CreateSomeIpBindingInstanceIdentifier();

    // When creating the binding
    auto unit = SkeletonBindingFactory::Create(instance_id);

    // That it is not null and a LoLa Skeleton
    EXPECT_EQ(unit, nullptr);
}

}  // namespace
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
