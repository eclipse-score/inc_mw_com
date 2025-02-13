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


#include "platform/aas/mw/com/impl/runtime.h"

#include "platform/aas/mw/com/impl/configuration/config_parser.h"
#include "platform/aas/mw/com/impl/instance_specifier.h"
#include <unistd.h>

#include <gtest/gtest.h>

#include <amp_span.hpp>

/// \attention There shall be only ONE test case per translation unit/unit test, which deals with the runtime Meyers
///            singleton instance. The reason is the singleton behavior of impl::Runtime! Once the singleton (Meyer
///            singleton) has been initialized with a certain config, it is fixed! Even if Runtime::Initialize is called
///            again with a different config/json, the singleton returned by Runtime::getInstance() remains unchanged!
///            And the Meyer singleton can't be removed/reset between tests!
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

/// \brief TC verifies, that Runtime::Initialize() succeeds, when called with a valid manifest path and that a
///        consecutive call to Runtime::resolve() returns the expected bmw::mw::com::InstanceIdentifiers.
///
/// \note we are re-using the existing //platform/aas/mw/com/impl/configuration:example/ara_com_config.json manifest
///       in this test.
TEST(RuntimeSingleProcessTest, initValidManifestPathReturnsWithValidInstanceSpecifier)
{
    RecordProperty("Verifies", ", 9");
    RecordProperty("Description", "InstanceSpecifier resolution can not retrieve wrong InstanceIdentifier.");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("Priority", "1");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    const auto instance_specifier_result = InstanceSpecifier::Create("abc/abc/TirePressurePort");
    ASSERT_TRUE(instance_specifier_result.has_value());
    bmw::StringLiteral test_args[] = {"dummyname",
                                      "-service_instance_manifest",
                                      "platform/aas/mw/com/impl/configuration/example/ara_com_config.json"};
    const amp::span<const bmw::StringLiteral> test_args_span{test_args};

    Runtime::Initialize(test_args_span);
    auto identifiers = Runtime::getInstance().resolve(instance_specifier_result.value());
    EXPECT_EQ(identifiers.size(), 1);
}

}  // namespace
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
