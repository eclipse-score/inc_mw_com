// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



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

/// \brief TC verifies, that a 2nd call to Runtime::Initialize() succeeds and configuration is activated, if NOT already
///        Runtime::getInstance() has been called before the 2nd Initialize() call!
/// \details Implementation will just log a warning as in production code a re-initialization is most likely an error/
///          unwanted, but in unit-testing being able to re-initialize is needed.
TEST(RuntimeSingleProcessTest, initSecondTimeDoesUpdateRuntime)
{
    const auto instance_specifier_result_1 = InstanceSpecifier::Create("abc/abc/TirePressurePort");
    ASSERT_TRUE(instance_specifier_result_1.has_value());
    bmw::StringLiteral test_args1[] = {"dummyname",
                                       "-service_instance_manifest",
                                       "platform/aas/mw/com/impl/configuration/example/ara_com_config.json"};

    const auto instance_specifier_result_2 = InstanceSpecifier::Create("abc/abc/TirePressurePortOther");
    ASSERT_TRUE(instance_specifier_result_2.has_value());
    bmw::StringLiteral test_args2[] = {"dummyname",
                                       "-service_instance_manifest",
                                       "platform/aas/mw/com/impl/configuration/example/ara_com_config_other.json"};
    const amp::span<const bmw::StringLiteral> test_args_span1{test_args1};
    const amp::span<const bmw::StringLiteral> test_args_span2{test_args2};

    Runtime::Initialize(test_args_span1);
    Runtime::Initialize(test_args_span2);
    auto identifiers = Runtime::getInstance().resolve(instance_specifier_result_2.value());
    EXPECT_EQ(identifiers.size(), 1);
}

}  // namespace
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
