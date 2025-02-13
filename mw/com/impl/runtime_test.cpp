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

#include "platform/aas/analysis/tracing/library/generic_trace_api/mocks/trace_library_mock.h"
#include "platform/aas/mw/com/impl/configuration/config_parser.h"
#include "platform/aas/mw/com/impl/instance_specifier.h"
#include "platform/aas/mw/com/impl/tracing/configuration/proxy_event_trace_point_type.h"
#include "platform/aas/mw/com/impl/tracing/configuration/skeleton_event_trace_point_type.h"
#include "platform/aas/mw/com/impl/tracing/configuration/tracing_filter_config.h"

#include <gtest/gtest.h>

#include <amp_span.hpp>
#include <string>

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

using testing::_;
using testing::Return;

/**
 * @brief TC verifies, that Runtime::Initialize() fails with abort, when NO "-service_instance_manifest" option is
 * given.
 */
TEST(RuntimeDeathTest, initNoManifestOption)
{
    bmw::StringLiteral test_args[] = {"dummyname", "arg1", "arg2", "arg3"};
    const amp::span<const bmw::StringLiteral> test_args_span{test_args};

    // Initialize without mandatory option "-service_instance_manifest"
    EXPECT_DEATH(Runtime::Initialize(test_args_span), ".*");
}

/**
 * @brief TC verifies, that Runtime::Initialize() fails with abort, when "-service_instance_manifest" option is
 * given, but no additional path parameter.
 */
TEST(RuntimeDeathTest, initMissingManifestPath)
{
    bmw::StringLiteral test_args[] = {"dummyname", "-service_instance_manifest"};
    const amp::span<const bmw::StringLiteral> test_args_span{test_args};

    // Initialize without arg/path-value for mandatory option "-service_instance_manifest"
    EXPECT_DEATH(Runtime::Initialize(test_args_span), ".*");
}

TEST(RuntimeDeathTest, InvalidJSONTerminates)
{
    EXPECT_DEATH(Runtime::Initialize(std::string{"{"}), ".*");
}

TEST(RuntimeConstructionTest, CtorWillCreateBindingRuntimes)
{
    Configuration dummy_configuration{Configuration::ServiceTypeDeployments{},
                                      Configuration::ServiceInstanceDeployments{},
                                      GlobalConfiguration{},
                                      TracingConfiguration{}};

    LolaServiceTypeDeployment service_type_deployment{42};
    dummy_configuration.AddServiceTypeDeployment(make_ServiceIdentifierType("dummyTypeName", 0, 0),
                                                 ServiceTypeDeployment{service_type_deployment});
    Runtime runtime{std::make_pair(std::move(dummy_configuration), amp::optional<tracing::TracingFilterConfig>{})};
    EXPECT_NE(runtime.GetBindingRuntime(BindingType::kLoLa), nullptr);
}

TEST(RuntimeTracingConfigTest, GetTracingFilterConfigWillReturnEmptyOptionalIfNotSet)
{
    Configuration dummy_configuration{Configuration::ServiceTypeDeployments{},
                                      Configuration::ServiceInstanceDeployments{},
                                      GlobalConfiguration{},
                                      TracingConfiguration{}};
    amp::optional<tracing::TracingFilterConfig> empty_filter_configuration{};
    Runtime runtime{std::make_pair(std::move(dummy_configuration), std::move(empty_filter_configuration))};
    auto tracing_config = runtime.GetTracingFilterConfig();
    EXPECT_EQ(tracing_config, nullptr);
}

TEST(RuntimeTracingConfigTest, CtorWillCreateTracingRuntimeIfTracingEnabledAndFilterConfigExists)
{
    // The created impl::tracing::TracingRuntime will create binding specific TracingRuntimes, which will register
    // itself with the GenericTraceAPI within their ctor. Therefore we need to setup a mock for the GenericTraceAPI.
    auto generic_trace_api_mock = std::make_unique<bmw::analysis::tracing::TraceLibraryMock>();

    // Given a configuration, where tracing is enabled
    TracingConfiguration dummy_tracing_configuration{};
    dummy_tracing_configuration.SetTracingEnabled(true);

    Configuration dummy_configuration{Configuration::ServiceTypeDeployments{},
                                      Configuration::ServiceInstanceDeployments{},
                                      GlobalConfiguration{},
                                      std::move(dummy_tracing_configuration)};
    // and given a minimal but valid tracing filter configuration
    tracing::TracingFilterConfig dummy_filter_configuration{};
    // and a LoLa service type deployment within the configuration
    LolaServiceTypeDeployment service_type_deployment{42};
    dummy_configuration.AddServiceTypeDeployment(make_ServiceIdentifierType("dummyTypeName", 0, 0),
                                                 ServiceTypeDeployment{service_type_deployment});
    const analysis::tracing::TraceClientId trace_client_id{42};

    // then expect, that the LoLa specific tracing runtime in its ctor will call RegisterClient() at the GenericTraceAPI
    EXPECT_CALL(*generic_trace_api_mock.get(), RegisterClient(analysis::tracing::BindingType::kLoLa, _))
        .WillOnce(Return(trace_client_id));
    // and will register a trace-done-callback at the GenericTraceAPI
    EXPECT_CALL(*generic_trace_api_mock.get(), RegisterTraceDoneCB(trace_client_id, _))
        .WillOnce(Return(bmw::Result<Blank>{}));

    // when we create a Runtime with the configuration and the trace filter configuration.
    Runtime runtime{std::make_pair(std::move(dummy_configuration),
                                   amp::optional<tracing::TracingFilterConfig>{std::move(dummy_filter_configuration)})};

    // and if we request the binding runtime for the LoLa binding from the Runtime, we get a valid lola::Runtime
    ASSERT_NE(runtime.GetBindingRuntime(BindingType::kLoLa), nullptr);
    // and this LoLa runtime also has a valid LoLa specific tracing runtime.
    EXPECT_NE(runtime.GetBindingRuntime(BindingType::kLoLa)->GetTracingRuntime(), nullptr);
}

TEST(RuntimeTracingConfigTest, CtorWillNotCreateTracingRuntimeIfTracingDisabled)
{
    // Given a configuration, where tracing is disabled
    TracingConfiguration dummy_tracing_configuration{};
    dummy_tracing_configuration.SetTracingEnabled(false);

    Configuration dummy_configuration{Configuration::ServiceTypeDeployments{},
                                      Configuration::ServiceInstanceDeployments{},
                                      GlobalConfiguration{},
                                      std::move(dummy_tracing_configuration)};
    // and given a minimal but valid tracing filter configuration
    tracing::TracingFilterConfig dummy_filter_configuration{};
    // and a LoLa service type deployment within the configuration
    LolaServiceTypeDeployment service_type_deployment{42};
    dummy_configuration.AddServiceTypeDeployment(make_ServiceIdentifierType("dummyTypeName", 0, 0),
                                                 ServiceTypeDeployment{service_type_deployment});

    // when we create a Runtime with the configuration and the trace filter configuration.
    Runtime runtime{std::make_pair(std::move(dummy_configuration),
                                   amp::optional<tracing::TracingFilterConfig>{std::move(dummy_filter_configuration)})};

    // and if we request the binding runtime for the LoLa binding from the Runtime, we get a valid lola::Runtime
    ASSERT_NE(runtime.GetBindingRuntime(BindingType::kLoLa), nullptr);
    // and this LoLa runtime has no valid LoLa specific tracing runtime.
    EXPECT_EQ(runtime.GetBindingRuntime(BindingType::kLoLa)->GetTracingRuntime(), nullptr);
}

TEST(RuntimeTracingConfigTest, CtorWillNotCreateTracingRuntimeIfNoTraceFilterConfigExists)
{
    // Given a configuration, where tracing is enabled
    TracingConfiguration dummy_tracing_configuration{};
    dummy_tracing_configuration.SetTracingEnabled(true);

    Configuration dummy_configuration{Configuration::ServiceTypeDeployments{},
                                      Configuration::ServiceInstanceDeployments{},
                                      GlobalConfiguration{},
                                      std::move(dummy_tracing_configuration)};
    // and a LoLa service type deployment within the configuration
    LolaServiceTypeDeployment service_type_deployment{42};
    dummy_configuration.AddServiceTypeDeployment(make_ServiceIdentifierType("dummyTypeName", 0, 0),
                                                 ServiceTypeDeployment{service_type_deployment});

    // when we create a Runtime with the configuration and NO trace filter configuration.
    Runtime runtime{std::make_pair(std::move(dummy_configuration), amp::optional<tracing::TracingFilterConfig>{})};

    // and if we request the binding runtime for the LoLa binding from the Runtime, we get a valid lola::Runtime
    ASSERT_NE(runtime.GetBindingRuntime(BindingType::kLoLa), nullptr);
    // and this LoLa runtime has no valid LoLa specific tracing runtime.
    EXPECT_EQ(runtime.GetBindingRuntime(BindingType::kLoLa)->GetTracingRuntime(), nullptr);
}

TEST(RuntimeTracingConfigTest, GetTracingFilterConfigWillReturnConfigPassedToConstructor)
{
    Configuration dummy_configuration{Configuration::ServiceTypeDeployments{},
                                      Configuration::ServiceInstanceDeployments{},
                                      GlobalConfiguration{},
                                      TracingConfiguration{}};

    const amp::string_view service_type_0{"service_type_0"};
    const amp::string_view service_type_1{"service_type_1"};
    const amp::string_view event_name_0{"event_name_0"};
    const amp::string_view event_name_1{"event_name_1"};
    const amp::string_view instance_specifier_view_0{"instance_specifier_view_0"};
    const amp::string_view instance_specifier_view_1{"instance_specifier_view_1"};
    const auto trace_point_0 = tracing::SkeletonEventTracePointType::SEND_WITH_ALLOCATE;
    const auto trace_point_1 = tracing::ProxyEventTracePointType::GET_NEW_SAMPLES;

    tracing::TracingFilterConfig input_tracing_filter_config{};
    input_tracing_filter_config.AddTracePoint(service_type_0, event_name_0, instance_specifier_view_0, trace_point_0);
    input_tracing_filter_config.AddTracePoint(service_type_1, event_name_1, instance_specifier_view_1, trace_point_1);

    amp::optional<tracing::TracingFilterConfig> filter_configuration{input_tracing_filter_config};
    Runtime runtime{std::make_pair(std::move(dummy_configuration), std::move(filter_configuration))};
    auto output_tracing_filter_config = runtime.GetTracingFilterConfig();
    EXPECT_NE(output_tracing_filter_config, nullptr);
    const bool is_trace_point_enabled_0 = output_tracing_filter_config->IsTracePointEnabled(
        service_type_0, event_name_0, instance_specifier_view_0, trace_point_0);
    const bool is_trace_point_enabled_1 = output_tracing_filter_config->IsTracePointEnabled(
        service_type_1, event_name_1, instance_specifier_view_1, trace_point_1);

    EXPECT_TRUE(is_trace_point_enabled_0);
    EXPECT_TRUE(is_trace_point_enabled_1);
}

TEST(RuntimeTest, CanRetrieveServiceDiscovery)
{
    Configuration dummy_configuration{Configuration::ServiceTypeDeployments{},
                                      Configuration::ServiceInstanceDeployments{},
                                      GlobalConfiguration{},
                                      TracingConfiguration{}};
    amp::optional<tracing::TracingFilterConfig> empty_filter_configuration{};
    Runtime runtime{std::make_pair(std::move(dummy_configuration), std::move(empty_filter_configuration))};
    amp::ignore = runtime.GetServiceDiscovery();
}

}  // namespace
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
