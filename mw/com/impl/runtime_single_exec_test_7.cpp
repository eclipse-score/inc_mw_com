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

TEST(RuntimeSingleProcessTest, UseDefaultPathIfNotProvided)
{
    // Given a configuration at the proper location
    chdir("platform/aas/mw/com/impl");

    const auto instance_specifier_result = InstanceSpecifier::Create("abc/abc/TirePressurePort");
    ASSERT_TRUE(instance_specifier_result.has_value());
    const amp::span<const bmw::StringLiteral> test_args_span;

    // when initializing the Runtime with commandline props NOT containing a manifest path ...
    Runtime::Initialize(test_args_span);

    // expect, that it gets initialized with the manifest/config from default location within the getInstance() call
    // from the default location.
    auto identifiers = Runtime::getInstance().resolve(instance_specifier_result.value());
    // and expect that therefore the specifier can be resolved.
    EXPECT_EQ(identifiers.size(), 1);
}

}  // namespace
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
