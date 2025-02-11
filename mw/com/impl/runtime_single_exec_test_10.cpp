// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#include "platform/aas/mw/com/impl/configuration/lola_service_type_deployment.h"
#include "platform/aas/mw/com/impl/handle_type.h"
#include "platform/aas/mw/com/impl/instance_identifier.h"
#include "platform/aas/mw/com/impl/instance_specifier.h"
#include "platform/aas/mw/com/impl/runtime.h"

#include "platform/aas/lib/memory/string_literal.h"

#include <amp_blank.hpp>
#include <amp_overload.hpp>
#include <amp_span.hpp>
#include <amp_string_view.hpp>
#include <amp_variant.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <unistd.h>
#include <vector>

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

std::vector<amp::string_view> GetEventNameListFromHandle(const HandleType& handle_type) noexcept
{
    using ReturnType = std::vector<amp::string_view>;

    const auto& identifier = handle_type.GetInstanceIdentifier();
    const auto& service_type_deployment = InstanceIdentifierView{identifier}.GetServiceTypeDeployment();
    auto visitor = amp::overload(
        [](const LolaServiceTypeDeployment& deployment) -> ReturnType {
            ReturnType event_names;
            for (const auto& event : deployment.events_)
            {
                const auto event_name = amp::string_view{event.first.data(), event.first.size()};
                event_names.push_back(event_name);
            }
            return event_names;
        },
        [](const amp::blank&) -> ReturnType { return {}; });
    return amp::visit(visitor, service_type_deployment.binding_info_);
}

/// \brief TC verifies that a HandleType can be created from a Lola json configuration and will contain the events
/// specified in the configuration.
///
/// \note we are re-using the existing //platform/aas/mw/com/impl/configuration:example/ara_com_config.json manifest
///       in this test.
TEST(RuntimeSingleProcessTest, initValidManifestPathReturnsWithValidInstanceSpecifier)
{
    RecordProperty("Verifies", "6");
    RecordProperty("Description",
                   "A HandleType containing the events in the Lola configuration file can be created from the "
                   "configuration file.");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("Priority", "1");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    // Given an InstanceIdentifier which is extracted from a Json configuration file
    const auto instance_specifier_result = InstanceSpecifier::Create("abc/abc/TirePressurePort");
    ASSERT_TRUE(instance_specifier_result.has_value());
    bmw::StringLiteral test_args[] = {"dummyname",
                                      "-service_instance_manifest",
                                      "platform/aas/mw/com/impl/configuration/example/ara_com_config.json"};
    const amp::span<const bmw::StringLiteral> test_args_span{test_args};

    Runtime::Initialize(test_args_span);
    auto identifiers = Runtime::getInstance().resolve(instance_specifier_result.value());
    EXPECT_EQ(identifiers.size(), 1);

    // When creating a handle from the InstanceIdentifier
    auto handle_type = make_HandleType(identifiers[0]);
    const auto event_name_list = GetEventNameListFromHandle(handle_type);

    // Then the handle will contain the events specified in the configuration.
    ASSERT_EQ(event_name_list.size(), 1);
    EXPECT_THAT(event_name_list, ::testing::Contains("CurrentPressureFrontLeft"));
}

}  // namespace
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
