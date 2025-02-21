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

/// \brief TC verifies, that configuration gets loaded from default manifest path, when Runtime is implicitly default
///        initialized by call to Runtime::getInstance() and that a 2nd explicit call to Runtime::Initialize() does
///        not lead to an error.
TEST(RuntimeSingleProcessTest, DefaultInitTwice)
{
    // Given a configuration at the proper location
    chdir("platform/aas/mw/com/impl");
    auto& unit = Runtime::getInstance();

    // When using default initialization of the runtime
    Runtime::Initialize();

    // Then the config is read and the runtime can be used
    const auto instance_specifier_result = InstanceSpecifier::Create("abc/abc/TirePressurePort");
    ASSERT_TRUE(instance_specifier_result.has_value());
    ASSERT_FALSE(unit.resolve(instance_specifier_result.value()).empty());
}

}  // namespace
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
