// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#include "platform/aas/mw/com/impl/configuration/tracing_configuration.h"

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

const tracing::ServiceElementIdentifier kDummyServiceElementIdentifier{"my_service_type",
                                                                       "my_service_element",
                                                                       tracing::ServiceElementType::EVENT};
const tracing::ServiceElementIdentifierView kDummyServiceElementIdentifierView{
    kDummyServiceElementIdentifier.service_type_name.data(),
    kDummyServiceElementIdentifier.service_element_name.data(),
    kDummyServiceElementIdentifier.service_element_type};
const auto kDummyInstanceSpecifier = InstanceSpecifier::Create("my_dummy_instance_specifier").value();
const amp::string_view kDummyInstanceSpecifierView = kDummyInstanceSpecifier.ToString();

TEST(TracingConfigurationTest, GettingTracingEnabledReturnsSetValue)
{
    TracingConfiguration tracing_configuration{};
    {
        const auto set_tracing_enabled = true;
        tracing_configuration.SetTracingEnabled(set_tracing_enabled);
        const auto get_tracing_enabled = tracing_configuration.IsTracingEnabled();
        EXPECT_EQ(get_tracing_enabled, set_tracing_enabled);
    }
    {
        const auto set_tracing_enabled = false;
        tracing_configuration.SetTracingEnabled(set_tracing_enabled);
        const auto get_tracing_enabled = tracing_configuration.IsTracingEnabled();
        EXPECT_EQ(get_tracing_enabled, set_tracing_enabled);
    }
}

TEST(TracingConfigurationTest, GettingApplicationInstanceIDReturnsSetValue)
{
    TracingConfiguration tracing_configuration{};
    {
        const auto set_tracing_application_instance_id = "a";
        tracing_configuration.SetApplicationInstanceID(set_tracing_application_instance_id);
        const auto get_tracing_application_instance_id = tracing_configuration.GetApplicationInstanceID();
        EXPECT_EQ(get_tracing_application_instance_id, amp::string_view{set_tracing_application_instance_id});
    }
    {
        const auto set_tracing_application_instance_id =
            "this_other_really_long_application_id_that_is_probably_too_long_for_sso";
        tracing_configuration.SetApplicationInstanceID(set_tracing_application_instance_id);
        const auto get_tracing_application_instance_id = tracing_configuration.GetApplicationInstanceID();
        EXPECT_EQ(get_tracing_application_instance_id, amp::string_view{set_tracing_application_instance_id});
    }
}

TEST(TracingConfigurationTest, GettingTracingTraceFilterConfigPath)
{
    TracingConfiguration tracing_configuration{};
    {
        const auto set_tracing_filter_config_path = "b";
        tracing_configuration.SetTracingTraceFilterConfigPath(set_tracing_filter_config_path);
        const auto get_tracing_filter_config_path = tracing_configuration.GetTracingFilterConfigPath();
        EXPECT_EQ(get_tracing_filter_config_path, amp::string_view{set_tracing_filter_config_path});
    }
    {
        const auto set_tracing_application_instance_id =
            "this_other_really_long_configuration_path_that_is_probably_too_long_for_sso";
        tracing_configuration.SetTracingTraceFilterConfigPath(set_tracing_application_instance_id);
        const auto get_tracing_application_instance_id = tracing_configuration.GetTracingFilterConfigPath();
        EXPECT_EQ(get_tracing_application_instance_id, amp::string_view{set_tracing_application_instance_id});
    }
}

TEST(TracingConfigurationTest, CheckingIsServiceElementTracingEnabledBeforeSettingReturnsFalse)
{
    TracingConfiguration tracing_configuration{};
    const auto is_enabled = tracing_configuration.IsServiceElementTracingEnabled(kDummyServiceElementIdentifierView,
                                                                                 kDummyInstanceSpecifierView);
    EXPECT_FALSE(is_enabled);
}

TEST(TracingConfigurationTest, CheckingIsServiceElementTracingEnabledAfterSettingReturnsTrue)
{
    TracingConfiguration tracing_configuration{};
    tracing_configuration.SetServiceElementTracingEnabled(kDummyServiceElementIdentifier, kDummyInstanceSpecifier);
    const auto is_enabled = tracing_configuration.IsServiceElementTracingEnabled(kDummyServiceElementIdentifierView,
                                                                                 kDummyInstanceSpecifierView);
    EXPECT_TRUE(is_enabled);
}

TEST(TracingConfigurationTest, CheckingIsServiceElementTracingEnabledAfterSettingMultipleSpecifiersReturnsTrue)
{
    auto other_instance_specifier = InstanceSpecifier::Create("a_completely_different_specifier").value();
    auto other_instance_specifier_view = other_instance_specifier.ToString();
    TracingConfiguration tracing_configuration{};
    tracing_configuration.SetServiceElementTracingEnabled(kDummyServiceElementIdentifier, kDummyInstanceSpecifier);
    tracing_configuration.SetServiceElementTracingEnabled(kDummyServiceElementIdentifier, other_instance_specifier);
    const auto is_enabled_1 = tracing_configuration.IsServiceElementTracingEnabled(kDummyServiceElementIdentifierView,
                                                                                   kDummyInstanceSpecifierView);
    EXPECT_TRUE(is_enabled_1);

    const auto is_enabled_2 = tracing_configuration.IsServiceElementTracingEnabled(kDummyServiceElementIdentifierView,
                                                                                   other_instance_specifier_view);
    EXPECT_TRUE(is_enabled_2);
}

}  // namespace
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
