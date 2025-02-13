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


#include "platform/aas/mw/com/impl/tracing/configuration/tracing_filter_config.h"

#include "platform/aas/mw/com/impl/tracing/configuration/proxy_event_trace_point_type.h"
#include <gtest/gtest.h>

namespace bmw
{
namespace mw
{
namespace com
{
namespace impl
{
namespace tracing
{
namespace
{

const std::string kServiceType{"my_service_type"};
const std::string kEventName{"my_event_name"};
const ITracingFilterConfig::InstanceSpecifierView kInstanceSpecifierView{"my_instance_specifier"};
const amp::optional<ITracingFilterConfig::InstanceSpecifierView> kEnableAllInstanceSpecifiers{};

template <typename TracePointTypeIn>
class TracingFilterConfigFixture : public ::testing::Test
{
};

// Gtest will run all tests in the TracingFilterConfigFixture once for every type, t, in MyTypes, such that TypeParam
// == t for each run.
using MyTypes = ::testing::
    Types<SkeletonEventTracePointType, SkeletonFieldTracePointType, ProxyEventTracePointType, ProxyFieldTracePointType>;
TYPED_TEST_SUITE(TracingFilterConfigFixture, MyTypes, );

TYPED_TEST(TracingFilterConfigFixture, CallingIsTracePointEnabledWithoutCallingAddReturnsFalse)
{
    const auto trace_point_type{static_cast<TypeParam>(1U)};

    // Given an empty ipc tracing filter config
    TracingFilterConfig tracing_filter_config{};

    // When checking if a trace point with an instance id is enabled before adding it to the config
    const bool is_enabled =
        tracing_filter_config.IsTracePointEnabled(kServiceType, kEventName, kInstanceSpecifierView, trace_point_type);

    // Then the trace point should be disabled
    EXPECT_FALSE(is_enabled);
}

TYPED_TEST(TracingFilterConfigFixture, CallingIsTracePointEnabledAfterCallingAddWithDifferentInstanceIdReturnsFalse)
{
    const ITracingFilterConfig::InstanceSpecifierView added_instance_specifier_view{"added_instance_specifier"};
    const ITracingFilterConfig::InstanceSpecifierView searched_instance_specifier_view{"searched_instance_specifier"};
    const auto trace_point_type{static_cast<TypeParam>(1U)};

    // Given an empty ipc tracing filter config
    TracingFilterConfig tracing_filter_config{};

    // When adding a trace point with an instance id
    tracing_filter_config.AddTracePoint(kServiceType, kEventName, added_instance_specifier_view, trace_point_type);

    // and then when checking if a trace point with a different instance id is enabled
    const bool is_enabled = tracing_filter_config.IsTracePointEnabled(
        kServiceType, kEventName, searched_instance_specifier_view, trace_point_type);

    // Then the trace point should be disabled
    EXPECT_FALSE(is_enabled);
}

TYPED_TEST(TracingFilterConfigFixture, CallingIsTracePointEnabledAfterCallingAddReturnsTrue)
{
    const auto trace_point_type{static_cast<TypeParam>(1U)};

    // Given an empty ipc tracing filter config
    TracingFilterConfig tracing_filter_config{};

    // When adding a trace point with an instance id
    tracing_filter_config.AddTracePoint(kServiceType, kEventName, kInstanceSpecifierView, trace_point_type);

    // and then when checking if the trace point is enabled
    const bool is_enabled =
        tracing_filter_config.IsTracePointEnabled(kServiceType, kEventName, kInstanceSpecifierView, trace_point_type);

    // Then the trace point should be enabled
    EXPECT_TRUE(is_enabled);
}

TYPED_TEST(TracingFilterConfigFixture, AddingSameTracePointTwiceWillNotCrash)
{
    const auto trace_point_type{static_cast<TypeParam>(1U)};

    // Given an empty ipc tracing filter config
    TracingFilterConfig tracing_filter_config{};

    // When adding the same trace point with an instance id twice
    tracing_filter_config.AddTracePoint(kServiceType, kEventName, kInstanceSpecifierView, trace_point_type);
    tracing_filter_config.AddTracePoint(kServiceType, kEventName, kInstanceSpecifierView, trace_point_type);

    // Then we shouldn't crash

    // and then when checking if the trace point is enabled
    const bool is_enabled =
        tracing_filter_config.IsTracePointEnabled(kServiceType, kEventName, kInstanceSpecifierView, trace_point_type);

    // Then the trace point should be enabled
    EXPECT_TRUE(is_enabled);
}

TEST(TracingFilterConfigTest, CheckingTracePointTypesWithSameNumericalValueDoNotMatch)
{
    const auto trace_point_type_0{static_cast<SkeletonEventTracePointType>(1U)};
    const auto trace_point_type_1{static_cast<ProxyEventTracePointType>(1U)};

    // Given an empty ipc tracing filter config
    TracingFilterConfig tracing_filter_config{};

    // When adding a trace point with a trace point type
    tracing_filter_config.AddTracePoint(kServiceType, kEventName, kInstanceSpecifierView, trace_point_type_0);

    // and then when checking if a trace point with the same identifiers and a service type with different enum type but
    // the same value
    const bool is_enabled =
        tracing_filter_config.IsTracePointEnabled(kServiceType, kEventName, kInstanceSpecifierView, trace_point_type_1);

    // Then the trace point should be disabled
    EXPECT_FALSE(is_enabled);
}

TEST(TracingFilterConfigDeathTest, AddingInvalidTracePointTypeTerminates)
{
    const auto trace_point_type{SkeletonEventTracePointType::INVALID};

    // Given an empty ipc tracing filter config
    TracingFilterConfig tracing_filter_config{};

    // When adding a trace point with an invalid trace point type it terminates
    EXPECT_DEATH(
        tracing_filter_config.AddTracePoint(kServiceType, kEventName, kInstanceSpecifierView, trace_point_type), ".*");
}

TEST(TracingFilterConfigGetNumberOfServiceElementsTest, InsertingNoTracePointsWithTraceDoneCBReturnsZero)
{
    const auto trace_point_type_0 = ProxyEventTracePointType::SUBSCRIBE;
    const auto trace_point_type_1 = SkeletonFieldTracePointType::SET_CALL;
    const auto trace_point_type_2 = ProxyFieldTracePointType::GET_NEW_SAMPLES;

    const std::string service_type_0{"my_service_type_0"};
    const std::string service_type_1{"my_service_type_1"};
    const std::string service_type_2{"my_service_type_2"};

    const std::string service_element_0{"my_service_element_name_0"};
    const std::string service_element_1{"my_service_element_name_1"};
    const std::string service_element_2{"my_service_element_name_2"};

    const ITracingFilterConfig::InstanceSpecifierView instance_specifier_view_0{"my_instance_specifier_0"};
    const ITracingFilterConfig::InstanceSpecifierView instance_specifier_view_1{"my_instance_specifier_1"};
    const ITracingFilterConfig::InstanceSpecifierView instance_specifier_view_2{"my_instance_specifier_2"};

    // Given an empty ipc tracing filter config
    TracingFilterConfig tracing_filter_config{};

    // When adding multiple trace points from different service elements which don't require tracing with a Trace
    // done callback
    tracing_filter_config.AddTracePoint(
        service_type_0, service_element_0, instance_specifier_view_0, trace_point_type_0);
    tracing_filter_config.AddTracePoint(
        service_type_1, service_element_1, instance_specifier_view_1, trace_point_type_1);
    tracing_filter_config.AddTracePoint(
        service_type_2, service_element_2, instance_specifier_view_2, trace_point_type_2);

    // then the number of service elements requiring a trace done callback should equal 0
    const auto number_of_service_elements = tracing_filter_config.GetNumberOfServiceElementsWithTraceDoneCB();
    EXPECT_EQ(number_of_service_elements, 0);
}

TEST(TracingFilterConfigGetNumberOfServiceElementsTest, InsertingTracePointsWithTraceDoneCBReturnsCorrectNumber)
{
    const auto trace_point_type_0 = SkeletonEventTracePointType::SEND;
    const auto trace_point_type_1 = SkeletonEventTracePointType::SEND_WITH_ALLOCATE;
    const auto trace_point_type_2 = ProxyFieldTracePointType::GET_NEW_SAMPLES;
    const auto trace_point_type_3 = SkeletonFieldTracePointType::UPDATE;
    const auto trace_point_type_4 = SkeletonFieldTracePointType::UPDATE_WITH_ALLOCATE;

    const std::string service_type_0{"my_service_type_0"};
    const std::string service_type_1{"my_service_type_1"};
    const std::string service_type_2{"my_service_type_2"};
    const std::string service_type_3{"my_service_type_3"};
    const std::string service_type_4{"my_service_type_4"};

    const std::string service_element_0{"my_service_element_name_0"};
    const std::string service_element_1{"my_service_element_name_1"};
    const std::string service_element_2{"my_service_element_name_2"};
    const std::string service_element_3{"my_service_element_name_3"};
    const std::string service_element_4{"my_service_element_name_4"};

    const ITracingFilterConfig::InstanceSpecifierView instance_specifier_view_0{"my_instance_specifier_0"};
    const ITracingFilterConfig::InstanceSpecifierView instance_specifier_view_1{"my_instance_specifier_1"};
    const ITracingFilterConfig::InstanceSpecifierView instance_specifier_view_2{"my_instance_specifier_2"};
    const ITracingFilterConfig::InstanceSpecifierView instance_specifier_view_3{"my_instance_specifier_3"};
    const ITracingFilterConfig::InstanceSpecifierView instance_specifier_view_4{"my_instance_specifier_4"};

    // Given an empty ipc tracing filter config
    TracingFilterConfig tracing_filter_config{};

    // When adding multiple trace points from different service elements, some of which require tracing with a Trace
    // done callback
    tracing_filter_config.AddTracePoint(
        service_type_0, service_element_0, instance_specifier_view_0, trace_point_type_0);
    tracing_filter_config.AddTracePoint(
        service_type_1, service_element_1, instance_specifier_view_1, trace_point_type_1);
    tracing_filter_config.AddTracePoint(
        service_type_2, service_element_2, instance_specifier_view_2, trace_point_type_2);
    tracing_filter_config.AddTracePoint(
        service_type_3, service_element_3, instance_specifier_view_3, trace_point_type_3);
    tracing_filter_config.AddTracePoint(
        service_type_4, service_element_4, instance_specifier_view_4, trace_point_type_4);

    // then the number of service elements requiring a trace done callback should equal 4
    const auto number_of_service_elements = tracing_filter_config.GetNumberOfServiceElementsWithTraceDoneCB();
    EXPECT_EQ(number_of_service_elements, 4);
}

TEST(TracingFilterConfigGetNumberOfServiceElementsTest,
     InsertingMultipleTracePointsFromSameServiceElementWithTraceDoneCBDoesNotCountMultiple)
{
    const auto trace_point_type_0 = SkeletonEventTracePointType::SEND;
    const auto trace_point_type_1 = SkeletonEventTracePointType::SEND_WITH_ALLOCATE;
    const auto trace_point_type_2 = ProxyFieldTracePointType::GET_NEW_SAMPLES;
    const auto trace_point_type_3 = SkeletonFieldTracePointType::UPDATE;
    const auto trace_point_type_4 = SkeletonFieldTracePointType::UPDATE_WITH_ALLOCATE;

    // Given an empty ipc tracing filter config
    TracingFilterConfig tracing_filter_config{};

    // When adding multiple trace points from the same service element, some of which require tracing with a Trace
    // done callback
    tracing_filter_config.AddTracePoint(kServiceType, kEventName, kInstanceSpecifierView, trace_point_type_0);
    tracing_filter_config.AddTracePoint(kServiceType, kEventName, kInstanceSpecifierView, trace_point_type_1);
    tracing_filter_config.AddTracePoint(kServiceType, kEventName, kInstanceSpecifierView, trace_point_type_2);
    tracing_filter_config.AddTracePoint(kServiceType, kEventName, kInstanceSpecifierView, trace_point_type_3);
    tracing_filter_config.AddTracePoint(kServiceType, kEventName, kInstanceSpecifierView, trace_point_type_4);

    // then the number of service elements requiring a trace done callback should equal 2
    const auto number_of_service_elements = tracing_filter_config.GetNumberOfServiceElementsWithTraceDoneCB();
    EXPECT_EQ(number_of_service_elements, 2);
}

}  // namespace
}  // namespace tracing
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
