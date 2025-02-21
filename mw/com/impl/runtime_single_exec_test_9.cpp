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

TEST(RuntimeSingleProcessTest, CannotRetrieveBinding)
{
    // Given a configuration at the proper location, which does NOT contain Fake bindings
    chdir("platform/aas/mw/com/impl");

    // expect, that Runtime gets initialized with the manifest/config from default location within the getInstance()
    // call from the default location.
    const auto* unit = Runtime::getInstance().GetBindingRuntime(BindingType::kFake);
    // and expect that NO fake binding Runtime can be resolved
    EXPECT_EQ(unit, nullptr);
}

}  // namespace
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
