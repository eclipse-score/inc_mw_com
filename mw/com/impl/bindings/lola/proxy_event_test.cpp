// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#include "platform/aas/mw/com/impl/bindings/lola/proxy_event.h"
#include "platform/aas/mw/com/impl/bindings/lola/proxy_event_common.h"
#include "platform/aas/mw/com/impl/bindings/lola/test/proxy_event_test_resources.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <tuple>
#include <type_traits>
#include <vector>

namespace bmw
{
namespace mw
{
namespace com
{
namespace impl
{
namespace lola
{
namespace
{

using TestSampleType = std::uint32_t;

/// \brief Function that returns the value pointed to by a pointer
template <typename T>
T GetSamplePtrValue(const T* const sample_ptr)
{
    return *sample_ptr;
}

/// \brief Function that casts and returns the value pointed to by a void pointer
///
/// Assumes that the object in memory being pointed to is of type TestSampleType.
TestSampleType GetSamplePtrValue(const void* const void_ptr)
{
    auto* const typed_ptr = static_cast<const TestSampleType*>(void_ptr);
    return *typed_ptr;
}

/// \brief Structs containing types for templated gtests.
///
/// We use a struct instead of a tuple since a tuple cannot contain a void type.
struct ProxyEventStruct
{
    using SampleType = TestSampleType;
    using ProxyEventType = ProxyEvent<TestSampleType>;
    using ProxyEventAttorneyType = ProxyEventAttorney<TestSampleType>;
};
struct GenericProxyEventStruct
{
    using SampleType = void;
    using ProxyEventType = GenericProxyEvent;
    using ProxyEventAttorneyType = GenericProxyEventAttorney;
};

/// \brief Templated test fixture for ProxyEvent functionality that works for both ProxyEvent and GenericProxyEvent
///
/// \tparam T A tuple containing:
///     SampleType either a type such as std::uint32_t or void
///     ProxyEventType either ProxyEvent or GenericProxyEvent
///     ProxyEventAttorneyType either ProxyEventAttorney or GenericProxyEventAttorney
template <typename T>
class LolaProxyEventFixture : public LolaProxyEventResources
{
  protected:
    using SampleType = typename T::SampleType;
    using ProxyEventType = typename T::ProxyEventType;
    using ProxyEventAttorneyType = typename T::ProxyEventAttorneyType;

    bool IsNumNewSamplesAvailableEqualTo(const std::size_t expected_num_samples)
    {
        const auto num_samples_available = proxy_event_attorney_.GetNumNewSamplesAvailableImpl();
        EXPECT_TRUE(num_samples_available.has_value());
        EXPECT_EQ(num_samples_available.value(), expected_num_samples);
        return (num_samples_available.has_value() && (num_samples_available.value() == expected_num_samples));
    }

    ProxyEventType test_proxy_event_{*parent_, element_fq_id_, event_name_};
    ProxyEventAttorneyType proxy_event_attorney_{test_proxy_event_};
    ProxyEventCommon& proxy_event_common_{proxy_event_attorney_.GetProxyEventCommon()};
    ProxyEventCommonAttorney proxy_event_common_attorney_{proxy_event_common_};
    SampleReferenceTracker sample_reference_tracker_{100U};
};

// Gtest will RegisterEventHandlerBeforeSubscriptionrun all tests in the LolaProxyEventFixture once for every type, t,
// in MyTypes, such that TypeParam == t for each run.
using MyTypes = ::testing::Types<ProxyEventStruct, GenericProxyEventStruct>;
TYPED_TEST_SUITE(LolaProxyEventFixture, MyTypes, );

TYPED_TEST(LolaProxyEventFixture, TestGetNewSamples)
{
    using Base = LolaProxyEventFixture<TypeParam>;

    Base::RecordProperty("Verifies", "8, 3, 7, 3");
    Base::RecordProperty("Description",
                         "Checks that GetNewSamples will get new samples from provider and GetNumNewSamplesAvailable "
                         "reflects the number of new samples available. The value of the TracePointDataId will be the "
                         "timestamp of the event slot.");
    Base::RecordProperty("TestType", "Requirements-based test");
    Base::RecordProperty("Priority", "1");
    Base::RecordProperty("DerivationTechnique", "Analysis of requirements");

    const EventSlotStatus::EventTimeStamp input_timestamp{10U};
    const std::uint32_t input_value{42U};
    const auto slot = Base::PutData(input_value, input_timestamp);

    const std::size_t max_slots{1U};
    Base::test_proxy_event_.Subscribe(max_slots);

    EXPECT_TRUE(Base::IsNumNewSamplesAvailableEqualTo(1));

    std::uint8_t num_callbacks_called{0U};
    TrackerGuardFactory guard_factory{Base::sample_reference_tracker_.Allocate(1U)};
    const auto num_callbacks = Base::proxy_event_attorney_.GetNewSamplesImpl(
        [this, slot, &num_callbacks_called, input_value, input_timestamp](
            impl::SamplePtr<typename Base::SampleType> sample,
            const tracing::ITracingRuntime::TracePointDataId timestamp) {
            ASSERT_TRUE(sample);
            EXPECT_FALSE((Base::event_control_->data_control)[slot].IsInvalid());

            const auto value = GetSamplePtrValue(sample.get());
            EXPECT_EQ(value, input_value);
            num_callbacks_called++;

            EXPECT_EQ(timestamp, input_timestamp);
        },
        guard_factory);
    ASSERT_TRUE(num_callbacks.has_value());
    ASSERT_EQ(num_callbacks.value(), 1);
    ASSERT_EQ(num_callbacks.value(), num_callbacks_called);

    EXPECT_TRUE(Base::IsNumNewSamplesAvailableEqualTo(0));
}

TYPED_TEST(LolaProxyEventFixture, ReceiveEventsInOrder)
{
    using Base = LolaProxyEventFixture<TypeParam>;

    Base::RecordProperty("Verifies", "8, 3, 7");
    Base::RecordProperty("Description",
                         "Sends multiple events and checks that reported number of new samples is correct and they are "
                         "received in order.");
    Base::RecordProperty("TestType", "Requirements-based test");
    Base::RecordProperty("Priority", "1");
    Base::RecordProperty("DerivationTechnique", "Analysis of requirements");

    EventSlotStatus::EventTimeStamp send_time{1};
    const std::vector<TestSampleType> values_to_send{1, 2, 3};
    for (const auto value_to_send : values_to_send)
    {
        Base::PutData(value_to_send, send_time);
        send_time++;
    }

    const std::size_t max_slots{3U};
    Base::test_proxy_event_.Subscribe(max_slots);

    EXPECT_TRUE(Base::IsNumNewSamplesAvailableEqualTo(3));

    std::uint8_t num_callbacks_called{0U};
    std::vector<TestSampleType> results{};
    TrackerGuardFactory guard_factory{Base::sample_reference_tracker_.Allocate(3U)};
    EventSlotStatus::EventTimeStamp received_send_time = 1;
    const auto num_callbacks = Base::proxy_event_attorney_.GetNewSamplesImpl(
        [&results, &num_callbacks_called, &received_send_time](impl::SamplePtr<typename Base::SampleType> sample,
                                                               tracing::ITracingRuntime::TracePointDataId timestamp) {
            ASSERT_TRUE(sample);

            const auto value = GetSamplePtrValue(sample.get());
            EXPECT_GE(value, 0);
            EXPECT_LE(value, 3);
            results.push_back(value);
            num_callbacks_called++;

            EXPECT_EQ(timestamp, received_send_time);
            received_send_time++;
        },
        guard_factory);
    ASSERT_TRUE(num_callbacks.has_value());
    EXPECT_EQ(num_callbacks.value(), 3);
    EXPECT_EQ(num_callbacks.value(), num_callbacks_called);
    EXPECT_EQ(results.size(), 3);
    EXPECT_EQ(values_to_send, results);

    EXPECT_TRUE(Base::IsNumNewSamplesAvailableEqualTo(0));

    TrackerGuardFactory guard_factory_{Base::sample_reference_tracker_.Allocate(15U)};
    const auto no_new_sample =
        Base::proxy_event_attorney_.GetNewSamplesImpl([](auto, auto) noexcept {}, guard_factory_);
    ASSERT_TRUE(no_new_sample.has_value());
    EXPECT_EQ(no_new_sample.value(), 0);
}

TYPED_TEST(LolaProxyEventFixture, DoNotReceiveEventsFromThePast)
{
    using Base = LolaProxyEventFixture<TypeParam>;

    Base::RecordProperty("Verifies", "8, 3, 7");
    Base::RecordProperty("Description",
                         "Sends multiple events and checks that reported number of new samples is correct and no "
                         "samples of the past are reported/received.");
    Base::RecordProperty("TestType", "Requirements-based test");
    Base::RecordProperty("Priority", "1");
    Base::RecordProperty("DerivationTechnique", "Analysis of requirements");

    const std::size_t max_slots{2U};
    Base::test_proxy_event_.Subscribe(max_slots);

    const EventSlotStatus::EventTimeStamp input_timestamp{17U};
    const std::uint32_t input_value{42U};
    Base::PutData(input_value, input_timestamp);

    EXPECT_TRUE(Base::IsNumNewSamplesAvailableEqualTo(1));

    std::uint8_t num_callbacks_called{0U};
    TrackerGuardFactory guard_factory{Base::sample_reference_tracker_.Allocate(37U)};
    const auto num_samples = Base::proxy_event_attorney_.GetNewSamplesImpl(
        [&num_callbacks_called, input_value, input_timestamp](impl::SamplePtr<typename Base::SampleType> sample,
                                                              tracing::ITracingRuntime::TracePointDataId timestamp) {
            ASSERT_TRUE(sample);

            const auto value = GetSamplePtrValue(sample.get());
            EXPECT_EQ(value, input_value);
            num_callbacks_called++;

            EXPECT_EQ(timestamp, input_timestamp);
        },
        guard_factory);
    ASSERT_TRUE(num_samples.has_value());
    EXPECT_EQ(num_samples.value(), 1);
    EXPECT_EQ(num_samples.value(), num_callbacks_called);

    const EventSlotStatus::EventTimeStamp input_timestamp_2{1U};
    const std::uint32_t input_value_2{42U};
    Base::PutData(input_value_2, input_timestamp_2);

    EXPECT_TRUE(Base::IsNumNewSamplesAvailableEqualTo(0));
    TrackerGuardFactory new_guard_factory{Base::sample_reference_tracker_.Allocate(37U)};
    const auto new_num_samples = Base::proxy_event_attorney_.GetNewSamplesImpl(
        [](auto, auto) { FAIL() << "Callback was called although no sample was expected."; }, new_guard_factory);
    ASSERT_TRUE(num_samples.has_value());
    EXPECT_EQ(new_num_samples.value(), 0);
}

TYPED_TEST(LolaProxyEventFixture, GetNewSamplesFailsWhenNotSubscribed)
{
    using Base = LolaProxyEventFixture<TypeParam>;

    const std::uint32_t input_value{42U};
    Base::PutData(input_value);

    TrackerGuardFactory guard_factory{Base::sample_reference_tracker_.Allocate(1U)};
    const auto num_samples = Base::test_proxy_event_.GetNewSamples(
        [](impl::SamplePtr<typename Base::SampleType>, auto) noexcept {}, guard_factory);
    ASSERT_FALSE(num_samples.has_value());
}

TYPED_TEST(LolaProxyEventFixture, GetNumNewSamplesFailsWhenNotSubscribed)
{
    using Base = LolaProxyEventFixture<TypeParam>;

    const auto num_new_samples = Base::test_proxy_event_.GetNumNewSamplesAvailable();
    ASSERT_FALSE(num_new_samples.has_value());
    EXPECT_EQ(num_new_samples.error(), ComErrc::kNotSubscribed);
}

TYPED_TEST(LolaProxyEventFixture, ProxyEventHoldsConstSlotData)
{
    using Base = LolaProxyEventFixture<TypeParam>;

    Base::RecordProperty("Verifies", "");
    Base::RecordProperty("Description", "Proxy shall interpret slot data as const");
    Base::RecordProperty("TestType", "Requirements-based test");
    Base::RecordProperty("Priority", "1");
    Base::RecordProperty("DerivationTechnique", "Analysis of requirements");

    const std::uint32_t input_value{42U};
    Base::PutData(input_value);

    const auto& proxy_event_common = Base::proxy_event_attorney_.GetProxyEventCommon();
    using SlotType = typename std::remove_pointer<decltype(proxy_event_common.GetRawEventDataStorage())>::type;
    static_assert(std::is_const<SlotType>::value, "Proxy should hold const slot data.");
}

TYPED_TEST(LolaProxyEventFixture, TestProperEventAcquisition)
{
    using Base = LolaProxyEventFixture<TypeParam>;

    Base::RecordProperty("Verifies", ", SSR-6225206");
    Base::RecordProperty("Description",
                         "Checks whether a proxy is acquiring data from shared memory (req. ) and slot "
                         "referencing works (req. SSR-6225206).");
    Base::RecordProperty("TestType", "Requirements-based test");
    Base::RecordProperty("Priority", "1");
    Base::RecordProperty("DerivationTechnique", "Analysis of requirements");

    const std::size_t max_sample_count{2U};
    const EventSlotStatus::EventTimeStamp input_timestamp{10U};
    const std::uint32_t input_value{42U};
    Base::PutData(input_value, input_timestamp);

    SampleReferenceTracker sample_reference_tracker{2U};
    TrackerGuardFactory guard_factory{sample_reference_tracker.Allocate(1U)};
    Base::test_proxy_event_.Subscribe(max_sample_count);
    EXPECT_EQ(Base::test_proxy_event_.GetSubscriptionState(), SubscriptionState::kSubscribed);
    auto num_new_samples_avail = Base::test_proxy_event_.GetNumNewSamplesAvailable();
    EXPECT_TRUE(num_new_samples_avail.has_value());
    EXPECT_EQ(num_new_samples_avail.value(), 1);
    const Result<std::size_t> count = Base::test_proxy_event_.GetNewSamples(
        [&sample_reference_tracker, input_value, input_timestamp](
            impl::SamplePtr<typename Base::SampleType> sample,
            const tracing::ITracingRuntime::TracePointDataId timestamp) {
            EXPECT_EQ(sample_reference_tracker.GetNumAvailableSamples(), 1U);

            const auto value = GetSamplePtrValue(sample.get());
            EXPECT_EQ(value, input_value);

            EXPECT_EQ(timestamp, input_timestamp);
        },
        guard_factory);
    ASSERT_TRUE(count.has_value());
    EXPECT_EQ(*count, 1);
    EXPECT_EQ(sample_reference_tracker.GetNumAvailableSamples(), 2U);
    Base::test_proxy_event_.Unsubscribe();
}

TYPED_TEST(LolaProxyEventFixture, FailOnUnsubscribed)
{
    using Base = LolaProxyEventFixture<TypeParam>;

    Base::PutData();

    auto num_new_samples_avail = Base::test_proxy_event_.GetNumNewSamplesAvailable();
    EXPECT_FALSE(num_new_samples_avail.has_value());
    EXPECT_EQ(static_cast<ComErrc>(*(num_new_samples_avail.error())), ComErrc::kNotSubscribed);

    SampleReferenceTracker sample_reference_tracker{2U};
    {
        TrackerGuardFactory guard_factory{sample_reference_tracker.Allocate(1U)};
        const Result<std::size_t> count = Base::test_proxy_event_.GetNewSamples(
            [](impl::SamplePtr<typename Base::SampleType>, auto) {
                FAIL() << "Callback called despite not having a valid subscription to the event.";
            },
            guard_factory);
        ASSERT_FALSE(count.has_value());
    }
    EXPECT_EQ(sample_reference_tracker.GetNumAvailableSamples(), 2U);
}

TYPED_TEST(LolaProxyEventFixture, TransmitEventInShmArea)
{
    using Base = LolaProxyEventFixture<TypeParam>;

    Base::RecordProperty("Verifies", "");
    Base::RecordProperty("Description",
                         "A valid SampleAllocateePtr and SamplePtr shall reference a valid and correct slot.");
    Base::RecordProperty("TestType ", "Requirements-based test");
    Base::RecordProperty("DerivationTechnique", "Analysis of requirements");

    const EventSlotStatus::EventTimeStamp input_timestamp{1U};
    const std::uint32_t input_value{42U};
    const auto slot = Base::PutData(input_value, input_timestamp);

    const std::size_t max_sample_count{1U};

    SampleReferenceTracker sample_reference_tracker{2U};
    TrackerGuardFactory guard_factory{sample_reference_tracker.Allocate(1U)};
    Base::test_proxy_event_.Subscribe(max_sample_count);
    EXPECT_EQ(Base::test_proxy_event_.GetSubscriptionState(), SubscriptionState::kSubscribed);
    auto num_new_samples_avail = Base::test_proxy_event_.GetNumNewSamplesAvailable();
    EXPECT_TRUE(num_new_samples_avail.has_value());
    EXPECT_EQ(num_new_samples_avail.value(), 1);

    const auto num_samples = Base::proxy_event_attorney_.GetNewSamplesImpl(
        [this, slot, input_value, input_timestamp](impl::SamplePtr<typename Base::SampleType> sample,
                                                   const tracing::ITracingRuntime::TracePointDataId timestamp) {
            EXPECT_FALSE((Base::event_control_->data_control)[slot].IsInvalid());

            const auto value = GetSamplePtrValue(sample.get());
            EXPECT_EQ(value, input_value);

            EXPECT_EQ(timestamp, input_timestamp);
        },
        guard_factory);
    ASSERT_TRUE(num_samples.has_value());
    EXPECT_EQ(num_samples.value(), 1);

    TrackerGuardFactory guard_factory2{sample_reference_tracker.Allocate(1U)};
    const auto no_samples = Base::proxy_event_attorney_.GetNewSamplesImpl(
        [](impl::SamplePtr<typename Base::SampleType>, auto) noexcept {}, guard_factory2);
    ASSERT_TRUE(no_samples.has_value());
    EXPECT_EQ(no_samples.value(), 0);
}

TYPED_TEST(LolaProxyEventFixture, GetBindingType)
{
    using Base = LolaProxyEventFixture<TypeParam>;

    EXPECT_EQ(Base::test_proxy_event_.GetBindingType(), BindingType::kLoLa);
}

using LolaProxyEventDeathFixture = LolaProxyEventResources;
TEST_F(LolaProxyEventDeathFixture, FailOnEventNotFound)
{
    const ElementFqId bad_element_fq_id{0xcdef, 0x6, 0x10, ElementType::EVENT};
    const std::string bad_event_name{"BadEventName"};

    EXPECT_DEATH(ProxyEvent<std::uint8_t>(*parent_, bad_element_fq_id, bad_event_name), ".*");
}

using LolaGenericProxyEventDeathFixture = LolaProxyEventResources;
TEST_F(LolaGenericProxyEventDeathFixture, FailOnEventNotFound)
{
    const ElementFqId bad_element_fq_id{0xcdef, 0x6, 0x10, ElementType::EVENT};

    const std::string bad_event_name{"BadEventName"};

    EXPECT_DEATH(GenericProxyEvent(*parent_, bad_element_fq_id, bad_event_name), ".*");
}

using LolaGenericProxyEventFixture = LolaProxyEventResources;
TEST_F(LolaGenericProxyEventFixture, GetSampleSize)
{
    RecordProperty("Verifies", "4");
    RecordProperty("Description",
                   "Checks that GetSampleSize will return the sample size of the underlying event data type.");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("Priority", "1");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    // Given a valid GenericProxyEvent
    GenericProxyEvent proxy_event{*parent_, element_fq_id_, event_name_};
    // Expect, that asking about the Sample size, we get the sizeof the underlying event data type (which is
    // std::uint32_t in case of LolaProxyEventResources)
    EXPECT_EQ(proxy_event.GetSampleSize(), sizeof(SampleType));
}

TEST_F(LolaGenericProxyEventFixture, HasSerializedFormat)
{
    RecordProperty("Verifies", "9");
    RecordProperty("Description", "Checks that HasSerializedFormat will always return false for the Lola binding.");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("Priority", "1");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    // Given a valid GenericProxyEvent
    GenericProxyEvent proxy_event{*parent_, element_fq_id_, event_name_};
    // Expect, that asking about the serialized format, we get "FALSE"
    EXPECT_EQ(proxy_event.HasSerializedFormat(), false);
}

}  // namespace
}  // namespace lola
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
