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


#include "platform/aas/mw/com/impl/bindings/lola/tracing/tracing_runtime.h"

#include "platform/aas/analysis/tracing/common/types.h"
#include "platform/aas/analysis/tracing/library/generic_trace_api/mocks/trace_library_mock.h"
#include "platform/aas/mw/com/impl/bindings/lola/test/skeleton_test_resources.h"
#include "platform/aas/mw/com/impl/bindings/mock_binding/sample_ptr.h"
#include "platform/aas/mw/com/impl/plumbing/sample_ptr.h"

#include <gtest/gtest.h>

#include <memory>

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
namespace tracing
{

using TestSampleType = std::uint16_t;

class TracingRuntimeAttorney
{
  public:
    TracingRuntimeAttorney(TracingRuntime& tracing_runtime) : tracing_runtime_{tracing_runtime} {}

    bmw::containers::DynamicArray<TracingRuntime::TypeErasedSamplePtrWithMutex>& GetTypeErasedSamplePtrs()
        const noexcept
    {
        return tracing_runtime_.type_erased_sample_ptrs_;
    }

  private:
    TracingRuntime& tracing_runtime_;
};

namespace
{

using ServiceInstanceElement = analysis::tracing::ServiceInstanceElement;
using testing::_;
using testing::Eq;
using testing::Invoke;
using testing::Return;
using testing::WithArg;

const Configuration kEmptyConfiguration{Configuration::ServiceTypeDeployments{},
                                        Configuration::ServiceInstanceDeployments{},
                                        GlobalConfiguration{},
                                        TracingConfiguration{}};

const impl::tracing::ServiceElementInstanceIdentifierView kServiceElementInstanceIdentifier0{
    {"service_type_0",
     TracingRuntime::kDummyElementNameForShmRegisterCallback,
     TracingRuntime::kDummyElementTypeForShmRegisterCallback},
    "instance_specifier_0"};
const impl::tracing::ServiceElementInstanceIdentifierView kServiceElementInstanceIdentifier1{
    {"service_type_1",
     TracingRuntime::kDummyElementNameForShmRegisterCallback,
     TracingRuntime::kDummyElementTypeForShmRegisterCallback},
    "instance_specifier_1"};

const analysis::tracing::ShmObjectHandle kShmObjectHandle0{5U};
const analysis::tracing::ShmObjectHandle kShmObjectHandle1{6U};

void* const kStartAddress0{reinterpret_cast<void*>(0x10)};
void* const kStartAddress1{reinterpret_cast<void*>(0x20)};

const memory::shared::ISharedMemoryResource::FileDescriptor kShmFileDescriptor0{100U};
const memory::shared::ISharedMemoryResource::FileDescriptor kShmFileDescriptor1{200U};

const std::size_t kNumberOfTracingServiceElements{5U};

mock_binding::SamplePtr<TestSampleType> kMockBindingSamplePointer = std::make_unique<TestSampleType>(42U);
impl::SamplePtr<TestSampleType> kDummySamplePtr{std::move(kMockBindingSamplePointer), SampleReferenceGuard{}};
impl::tracing::TypeErasedSamplePtr kDummyTypeErasedSamplePtr{std::move(kDummySamplePtr)};

TEST(TracingRuntimeDataLossFlagTest, DataLossFlagIsFalseByDefault)
{
    // Given a TracingRuntimeObject
    TracingRuntime tracing_runtime{kNumberOfTracingServiceElements, kEmptyConfiguration};

    // Then getting the data loss flag before setting returns false
    EXPECT_FALSE(tracing_runtime.GetDataLossFlag());
}

TEST(TracingRuntimeDataLossFlagTest, GettingDataLossFlagAfterSettingReturnsTrue)
{
    // Given a TracingRuntimeObject
    TracingRuntime tracing_runtime{kNumberOfTracingServiceElements, kEmptyConfiguration};

    // When setting the data loss flag to true
    tracing_runtime.SetDataLossFlag(true);

    // Then getting the data loss flag returns true
    EXPECT_TRUE(tracing_runtime.GetDataLossFlag());
}

TEST(TracingRuntimeDataLossFlagTest, GettingDataLossFlagAfterClearingReturnsFalse)
{
    // Given a TracingRuntimeObject
    TracingRuntime tracing_runtime{kNumberOfTracingServiceElements, kEmptyConfiguration};

    // When setting the data loss flag to true
    tracing_runtime.SetDataLossFlag(true);

    // and then setting the data loss flag to false
    tracing_runtime.SetDataLossFlag(false);

    // Then getting the data loss flag returns false
    EXPECT_FALSE(tracing_runtime.GetDataLossFlag());
}

TEST(TracingRuntimeRegisterServiceElementTest, SamplePtrArrayShouldBeNeverBeResized)
{
    // Given a TracingRuntimeObject
    TracingRuntime tracing_runtime{kNumberOfTracingServiceElements, kEmptyConfiguration};
    const auto& sample_ptr_array = TracingRuntimeAttorney{tracing_runtime}.GetTypeErasedSamplePtrs();

    // Then the sample ptr array should initially be of size kNumberOfTracingServiceElements
    const auto sample_ptr_array_size = sample_ptr_array.size();
    EXPECT_EQ(sample_ptr_array_size, kNumberOfTracingServiceElements);

    // When registering multiple sample ptrs
    for (TestSampleType i = 0; i < kNumberOfTracingServiceElements; ++i)
    {
        const auto service_element_idx = tracing_runtime.RegisterServiceElement();
        EXPECT_EQ(service_element_idx, i);

        // Then the size of the array should never change
        const auto new_sample_ptr_array_size = sample_ptr_array.size();
        EXPECT_EQ(new_sample_ptr_array_size, kNumberOfTracingServiceElements);
    }
}

TEST(TracingRuntimeRegisterServiceElementTest, RegisteringMultipleServiceElementsWillSetConsecutiveElementsInArray)
{
    // Given a TracingRuntimeObject
    TracingRuntime tracing_runtime{kNumberOfTracingServiceElements, kEmptyConfiguration};

    // and when registering 2 service elements
    const auto idx_0 = tracing_runtime.RegisterServiceElement();
    const auto idx_1 = tracing_runtime.RegisterServiceElement();

    // Then the indices should be the 0 and 1
    ASSERT_EQ(idx_0, 0U);
    ASSERT_EQ(idx_1, 1U);
}

TEST(TracingRuntimeRegisterServiceElementDeathTest, RegisteringTooManyServiceElementsWillTerminate)
{
    // Given a TracingRuntimeObject
    TracingRuntime tracing_runtime{kNumberOfTracingServiceElements, kEmptyConfiguration};

    // When registering the maximum number of allowed service elements
    for (std::size_t i = 0; i < kNumberOfTracingServiceElements; ++i)
    {
        tracing_runtime.RegisterServiceElement();
    }
    // Then we don't crash
    // But then when we register another service element we terminate
    EXPECT_DEATH(tracing_runtime.RegisterServiceElement(), ".*");
}

TEST(TracingRuntimeTypeErasedSamplePtrTest, SettingTypeErasedSamplePtrDoesNotDestroySamplePtrUntilItIsCleared)
{
    class DestructorTracer
    {
      public:
        DestructorTracer(bool& was_destructed) : is_owner_{true}, was_destructed_{was_destructed} {}
        ~DestructorTracer()
        {
            if (is_owner_)
            {
                was_destructed_ = true;
            }
        }

        DestructorTracer(const DestructorTracer&) = delete;
        DestructorTracer& operator=(const DestructorTracer&) = delete;
        DestructorTracer& operator=(DestructorTracer&&) = delete;

        DestructorTracer(DestructorTracer&& other) : is_owner_{true}, was_destructed_{other.was_destructed_}
        {
            other.is_owner_ = false;
        }

      private:
        bool is_owner_;
        bool& was_destructed_;
    };

    bool was_destructed{false};
    mock_binding::SamplePtr<DestructorTracer> pointer = std::make_unique<DestructorTracer>(was_destructed);
    impl::SamplePtr<DestructorTracer> sample_ptr{std::move(pointer), SampleReferenceGuard{}};
    impl::tracing::TypeErasedSamplePtr type_erased_sample_ptr{std::move(sample_ptr)};

    // Given a TracingRuntimeObject
    TracingRuntime tracing_runtime{kNumberOfTracingServiceElements, kEmptyConfiguration};

    // When registering a service element
    const auto service_element_idx = tracing_runtime.RegisterServiceElement();
    EXPECT_EQ(service_element_idx, 0);

    // and setting the type erased sample ptr
    tracing_runtime.SetTypeErasedSamplePtr(std::move(type_erased_sample_ptr), service_element_idx);

    // Then the sample ptr will not be destroyed
    EXPECT_FALSE(was_destructed);

    // Until after the type erased sample ptr is cleared
    tracing_runtime.ClearTypeErasedSamplePtr(service_element_idx);
    EXPECT_TRUE(was_destructed);
}

TEST(TracingRuntimeTypeErasedSamplePtrTest, ServiceElementTracingIsActiveAfterSettingTypeErasedSamplePtr)
{
    RecordProperty("Verifies", "5");
    RecordProperty("Description",
                   "Calling SetTypeErasedSamplePtr will store that data in shared memory is currently being traced.");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    // Given a TracingRuntimeObject
    TracingRuntime tracing_runtime{kNumberOfTracingServiceElements, kEmptyConfiguration};

    // When registering a service element
    const auto service_element_idx = tracing_runtime.RegisterServiceElement();
    EXPECT_EQ(service_element_idx, 0);

    // Then the service element is initially inactive
    EXPECT_FALSE(tracing_runtime.IsServiceElementTracingActive(service_element_idx));

    // and when setting the type erased sample ptr
    tracing_runtime.SetTypeErasedSamplePtr(std::move(kDummyTypeErasedSamplePtr), service_element_idx);

    // Then the service element is active
    EXPECT_TRUE(tracing_runtime.IsServiceElementTracingActive(service_element_idx));
}

TEST(TracingRuntimeTypeErasedSamplePtrTest, ServiceElementTracingIsInActiveAfterClearingTypeErasedSamplePtr)
{
    // Given a TracingRuntimeObject
    TracingRuntime tracing_runtime{kNumberOfTracingServiceElements, kEmptyConfiguration};

    // When registering a service element
    const auto service_element_idx = tracing_runtime.RegisterServiceElement();
    EXPECT_EQ(service_element_idx, 0);

    // and when setting the type erased sample ptr
    tracing_runtime.SetTypeErasedSamplePtr(std::move(kDummyTypeErasedSamplePtr), service_element_idx);

    // Then the service element is active
    EXPECT_TRUE(tracing_runtime.IsServiceElementTracingActive(service_element_idx));

    // and when clearing the type erased sample ptr
    tracing_runtime.ClearTypeErasedSamplePtr(service_element_idx);

    // then the service element is inactive
    EXPECT_FALSE(tracing_runtime.IsServiceElementTracingActive(service_element_idx));
}

TEST(TracingRuntimeTypeErasedSamplePtrDeathTest, SettingTypeErasedSamplePtrBeforeRegisteringServiceElementTerminates)
{
    impl::tracing::ITracingRuntimeBinding::TraceContextId invalid_service_element_idx{0U};

    // Given a TracingRuntimeObject
    TracingRuntime tracing_runtime{kNumberOfTracingServiceElements, kEmptyConfiguration};

    // When a service element has not yet been registered

    // Then setting the type erased sample ptr will terminate
    EXPECT_DEATH(
        tracing_runtime.SetTypeErasedSamplePtr(std::move(kDummyTypeErasedSamplePtr), invalid_service_element_idx),
        ".*");
}

TEST(TracingRuntimeShmObjectHandleTestDeathTest, RegisterShmObjectWithNotTheExpectedDummyElementNameTerminates)
{
    // Given a TracingRuntimeObject
    TracingRuntime tracing_runtime{kNumberOfTracingServiceElements, kEmptyConfiguration};

    // and a ServiceElementInstanceIdentifier with a wrong/unexpected element name
    const impl::tracing::ServiceElementInstanceIdentifierView kServiceElementInstanceIdentifierInvalidName{
        {"service_type_0", "some_name", TracingRuntime::kDummyElementTypeForShmRegisterCallback},
        "instance_specifier_0"};

    // Using the invalid service element instance identifier leads to termination.
    EXPECT_DEATH(tracing_runtime.RegisterShmObject(
                     kServiceElementInstanceIdentifierInvalidName, kShmObjectHandle0, kStartAddress0),
                 ".*");
}

TEST(TracingRuntimeShmObjectHandleTestDeathTest, RegisterShmObjectWithNotTheExpectedDummyElementTypeTerminates)
{
    // Given a TracingRuntimeObject
    TracingRuntime tracing_runtime{kNumberOfTracingServiceElements, kEmptyConfiguration};

    // and a ServiceElementInstanceIdentifier with a wrong/unexpected element type
    const impl::tracing::ServiceElementInstanceIdentifierView kServiceElementInstanceIdentifierInvalidType{
        {"service_type_0",
         TracingRuntime::kDummyElementNameForShmRegisterCallback,
         impl::tracing::ServiceElementType::FIELD},
        "instance_specifier_0"};

    // Using the invalid service element instance identifier leads to termination.
    EXPECT_DEATH(tracing_runtime.RegisterShmObject(
                     kServiceElementInstanceIdentifierInvalidType, kShmObjectHandle0, kStartAddress0),
                 ".*");
}

TEST(TracingRuntimeShmObjectHandleTest, GettingShmObjectHandleAndStartAddressAfterRegisteringReturnsHandle)
{
    // Given a TracingRuntimeObject
    TracingRuntime tracing_runtime{kNumberOfTracingServiceElements, kEmptyConfiguration};

    // When registering 2 object handles and start addresses
    tracing_runtime.RegisterShmObject(kServiceElementInstanceIdentifier0, kShmObjectHandle0, kStartAddress0);
    tracing_runtime.RegisterShmObject(kServiceElementInstanceIdentifier1, kShmObjectHandle1, kStartAddress1);

    // Then getting the shm object handles returns the correct handles
    const auto returned_handle_0 = tracing_runtime.GetShmObjectHandle(kServiceElementInstanceIdentifier0);
    ASSERT_TRUE(returned_handle_0.has_value());
    EXPECT_EQ(returned_handle_0.value(), kShmObjectHandle0);

    const auto returned_handle_1 = tracing_runtime.GetShmObjectHandle(kServiceElementInstanceIdentifier1);
    ASSERT_TRUE(returned_handle_1.has_value());
    EXPECT_EQ(returned_handle_1.value(), kShmObjectHandle1);

    // And getting the start addresses returns the correct addresses
    const auto returned_start_address_0 = tracing_runtime.GetShmRegionStartAddress(kServiceElementInstanceIdentifier0);
    ASSERT_TRUE(returned_start_address_0.has_value());
    EXPECT_EQ(returned_start_address_0.value(), kStartAddress0);

    const auto returned_start_address_1 = tracing_runtime.GetShmRegionStartAddress(kServiceElementInstanceIdentifier1);
    ASSERT_TRUE(returned_start_address_1.has_value());
    EXPECT_EQ(returned_start_address_1.value(), kStartAddress1);
}

TEST(TracingRuntimeShmObjectHandleTest, GettingShmObjectHandleAndStartAddressWithoutRegistrationReturnsEmpty)
{
    // Given a TracingRuntimeObject
    TracingRuntime tracing_runtime{kNumberOfTracingServiceElements, kEmptyConfiguration};

    // When getting shm object handles before registering them
    const auto returned_handle_0 = tracing_runtime.GetShmObjectHandle(kServiceElementInstanceIdentifier0);
    const auto returned_handle_1 = tracing_runtime.GetShmObjectHandle(kServiceElementInstanceIdentifier1);

    // Then the results are empty optionals
    EXPECT_FALSE(returned_handle_0.has_value());
    EXPECT_FALSE(returned_handle_1.has_value());

    // When getting start addresses before registering them
    const auto returned_start_address_0 = tracing_runtime.GetShmRegionStartAddress(kServiceElementInstanceIdentifier0);
    const auto returned_start_address_1 = tracing_runtime.GetShmRegionStartAddress(kServiceElementInstanceIdentifier1);

    // Then the results are empty optionals
    EXPECT_FALSE(returned_start_address_0);
    EXPECT_FALSE(returned_start_address_1);
}

TEST(TracingRuntimeShmObjectHandleTest, GettingShmObjectHandleAndStartAddressAfterUnregisteringReturnsEmpty)
{
    // Given a TracingRuntimeObject
    TracingRuntime tracing_runtime{kNumberOfTracingServiceElements, kEmptyConfiguration};

    // When registering 2 object handles and start addresses
    tracing_runtime.RegisterShmObject(kServiceElementInstanceIdentifier0, kShmObjectHandle0, kStartAddress0);
    tracing_runtime.RegisterShmObject(kServiceElementInstanceIdentifier1, kShmObjectHandle1, kStartAddress1);

    // And then unregistering the first
    tracing_runtime.UnregisterShmObject(kServiceElementInstanceIdentifier0);

    // Then getting the shm object handles returns the correct handles only for the second
    const auto returned_handle_0 = tracing_runtime.GetShmObjectHandle(kServiceElementInstanceIdentifier0);
    EXPECT_FALSE(returned_handle_0.has_value());

    const auto returned_handle_1 = tracing_runtime.GetShmObjectHandle(kServiceElementInstanceIdentifier1);
    ASSERT_TRUE(returned_handle_1.has_value());
    EXPECT_EQ(returned_handle_1.value(), kShmObjectHandle1);

    // And getting the start addresses returns the correct addresses only for the second
    const auto returned_start_address_0 = tracing_runtime.GetShmRegionStartAddress(kServiceElementInstanceIdentifier0);
    EXPECT_FALSE(returned_start_address_0.has_value());

    const auto returned_start_address_1 = tracing_runtime.GetShmRegionStartAddress(kServiceElementInstanceIdentifier1);
    ASSERT_TRUE(returned_start_address_1.has_value());
    EXPECT_EQ(returned_start_address_1.value(), kStartAddress1);
}

TEST(TracingRuntimeCachedFileDescriptorTest, GettingFileDescriptorAfterCachingReturnsFileDescriptor)
{
    // Given a TracingRuntimeObject
    TracingRuntime tracing_runtime{kNumberOfTracingServiceElements, kEmptyConfiguration};

    // When caching 2 file descriptors
    tracing_runtime.CacheFileDescriptorForReregisteringShmObject(
        kServiceElementInstanceIdentifier0, kShmFileDescriptor0, kStartAddress0);
    tracing_runtime.CacheFileDescriptorForReregisteringShmObject(
        kServiceElementInstanceIdentifier1, kShmFileDescriptor1, kStartAddress1);

    // Then getting the file descriptors returns the correct descriptors
    const auto returned_file_descriptor_0 =
        tracing_runtime.GetCachedFileDescriptorForReregisteringShmObject(kServiceElementInstanceIdentifier0);
    ASSERT_TRUE(returned_file_descriptor_0.has_value());
    EXPECT_EQ(returned_file_descriptor_0.value().first, kShmFileDescriptor0);
    EXPECT_EQ(returned_file_descriptor_0.value().second, kStartAddress0);

    const auto returned_file_descriptor_1 =
        tracing_runtime.GetCachedFileDescriptorForReregisteringShmObject(kServiceElementInstanceIdentifier1);
    ASSERT_TRUE(returned_file_descriptor_1.has_value());
    EXPECT_EQ(returned_file_descriptor_1.value().first, kShmFileDescriptor1);
    EXPECT_EQ(returned_file_descriptor_1.value().second, kStartAddress1);
}

TEST(TracingRuntimeCachedFileDescriptorTest, GettingFileDescriptorWithoutCachingReturnsEmpty)
{
    // Given a TracingRuntimeObject
    TracingRuntime tracing_runtime{kNumberOfTracingServiceElements, kEmptyConfiguration};

    // When getting the file descriptors before caching them
    const auto returned_file_descriptor_0 =
        tracing_runtime.GetCachedFileDescriptorForReregisteringShmObject(kServiceElementInstanceIdentifier0);
    const auto returned_file_descriptor_1 =
        tracing_runtime.GetCachedFileDescriptorForReregisteringShmObject(kServiceElementInstanceIdentifier1);

    // Then the results are empty optionals
    EXPECT_FALSE(returned_file_descriptor_0.has_value());
    EXPECT_FALSE(returned_file_descriptor_1.has_value());
}

TEST(TracingRuntimeCachedFileDescriptorTest, GettingFileDescriptorAfterClearingReturnsEmpty)
{
    // Given a TracingRuntimeObject
    TracingRuntime tracing_runtime{kNumberOfTracingServiceElements, kEmptyConfiguration};

    // When caching 2 file descriptors
    tracing_runtime.CacheFileDescriptorForReregisteringShmObject(
        kServiceElementInstanceIdentifier0, kShmFileDescriptor0, kStartAddress0);
    tracing_runtime.CacheFileDescriptorForReregisteringShmObject(
        kServiceElementInstanceIdentifier1, kShmFileDescriptor1, kStartAddress1);

    // And then clearing the first
    tracing_runtime.ClearCachedFileDescriptorForReregisteringShmObject(kServiceElementInstanceIdentifier0);

    // Then getting the file descriptors returns the correct file descriptor only for the second
    const auto returned_file_descriptor_0 =
        tracing_runtime.GetCachedFileDescriptorForReregisteringShmObject(kServiceElementInstanceIdentifier0);
    EXPECT_FALSE(returned_file_descriptor_0.has_value());

    const auto returned_file_descriptor_1 =
        tracing_runtime.GetCachedFileDescriptorForReregisteringShmObject(kServiceElementInstanceIdentifier1);
    ASSERT_TRUE(returned_file_descriptor_1.has_value());
    EXPECT_EQ(returned_file_descriptor_1.value().first, kShmFileDescriptor1);
    EXPECT_EQ(returned_file_descriptor_1.value().second, kStartAddress1);
}

class RegisterWithGenericTraceApiFixture : public testing::Test
{
  public:
    RegisterWithGenericTraceApiFixture()
    {
        generic_trace_api_mock_ = std::make_unique<bmw::analysis::tracing::TraceLibraryMock>();
        TracingConfiguration dummy_tracing_configuration{};
        dummy_tracing_configuration.SetApplicationInstanceID(application_instance_id_);
        dummy_configuration_ = std::make_unique<Configuration>(Configuration::ServiceTypeDeployments{},
                                                               Configuration::ServiceInstanceDeployments{},
                                                               GlobalConfiguration{},
                                                               std::move(dummy_tracing_configuration));
    }

  protected:
    const std::uint8_t trace_client_id_{42};
    std::unique_ptr<bmw::analysis::tracing::TraceLibraryMock> generic_trace_api_mock_{};
    static constexpr char application_instance_id_[] = "MyApp";
    std::unique_ptr<Configuration> dummy_configuration_{nullptr};
};

TEST_F(RegisterWithGenericTraceApiFixture, Registration_OK)
{
    // Given a TracingRuntimeObject
    TracingRuntime tracing_runtime{kNumberOfTracingServiceElements, *dummy_configuration_};

    // expect, that it calls RegisterClient with the configured application_instance_id_ at the GenericTraceAPI
    EXPECT_CALL(*generic_trace_api_mock_.get(),
                RegisterClient(analysis::tracing::BindingType::kLoLa, application_instance_id_))
        .WillOnce(Return(trace_client_id_));
    // and expect, that it calls then RegisterTraceDoneCB at the GenericTraceAPI with the trace client id, which has
    // been returned by the RegisterClient call
    EXPECT_CALL(*generic_trace_api_mock_.get(), RegisterTraceDoneCB(trace_client_id_, _))
        .WillOnce(Return(bmw::Result<Blank>{}));

    // expect the UuT to return true, when we call RegisterWithGenericTraceApi on it
    EXPECT_TRUE(tracing_runtime.RegisterWithGenericTraceApi());
}

TEST_F(RegisterWithGenericTraceApiFixture, Registration_Error)
{
    // Given a TracingRuntimeObject
    TracingRuntime tracing_runtime{kNumberOfTracingServiceElements, *dummy_configuration_};

    // expect, that it calls RegisterClient with the configured application_instance_id_ at the GenericTraceAPI
    EXPECT_CALL(*generic_trace_api_mock_.get(),
                RegisterClient(analysis::tracing::BindingType::kLoLa, application_instance_id_))
        .WillOnce(Return(bmw::MakeUnexpected(analysis::tracing::ErrorCode::kNotEnoughMemoryRecoverable)));

    // expect the UuT to return false, when we call RegisterWithGenericTraceApi on it
    EXPECT_FALSE(tracing_runtime.RegisterWithGenericTraceApi());
}

TEST_F(RegisterWithGenericTraceApiFixture, RegistrationTraceDoneCB_Error)
{
    // Given a TracingRuntimeObject
    TracingRuntime tracing_runtime{kNumberOfTracingServiceElements, *dummy_configuration_};

    // expect, that it calls RegisterClient with the configured application_instance_id_ at the GenericTraceAPI
    EXPECT_CALL(*generic_trace_api_mock_.get(),
                RegisterClient(analysis::tracing::BindingType::kLoLa, application_instance_id_))
        .WillOnce(Return(trace_client_id_));
    // and expect, that it calls then RegisterTraceDoneCB at the GenericTraceAPI with the trace client id, which has
    // been returned by the RegisterClient call, but it returns a failure
    EXPECT_CALL(*generic_trace_api_mock_.get(), RegisterTraceDoneCB(trace_client_id_, _))
        .WillOnce(Return(bmw::MakeUnexpected(analysis::tracing::ErrorCode::kDaemonConnectionFailedFatal)));

    // expect the UuT to return false, when we call RegisterWithGenericTraceApi on it
    EXPECT_FALSE(tracing_runtime.RegisterWithGenericTraceApi());
}

class TraceDoneCallbackFixture : public RegisterWithGenericTraceApiFixture
{
  public:
    TraceDoneCallbackFixture()
    {
        // expect, that it calls RegisterClient with the configured application_instance_id_ at the GenericTraceAPI
        ON_CALL(*generic_trace_api_mock_.get(),
                RegisterClient(analysis::tracing::BindingType::kLoLa, application_instance_id_))
            .WillByDefault(Return(trace_client_id_));

        // and expect, that it calls then RegisterTraceDoneCB at the GenericTraceAPI with the trace client id, which has
        // been returned by the RegisterClient call
        ON_CALL(*generic_trace_api_mock_.get(), RegisterTraceDoneCB(trace_client_id_, _))
            .WillByDefault(WithArg<1>(
                Invoke([this](auto trace_done_callback) -> analysis::tracing::RegisterTraceDoneCallBackResult {
                    trace_done_callback_ = std::move(trace_done_callback);
                    static_cast<void>(trace_done_callback);
                    return bmw::Result<Blank>{};
                })));
    }

    analysis::tracing::TraceDoneCallBackType trace_done_callback_;
};

TEST_F(TraceDoneCallbackFixture, ServiceElementTracingIsInactiveAfterCallingTraceDoneCallbackWithCorrectContextId)
{
    RecordProperty("Verifies", "1, 8");
    RecordProperty("Description",
                   "Calling the trace done callback with the correct context id will clear that data in shared memory "
                   "is currently being traced.");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    mock_binding::SamplePtr<TestSampleType> mock_binding_sample_pointer_1 = std::make_unique<TestSampleType>(24U);
    impl::SamplePtr<TestSampleType> dummy_sample_pointer_1{std::move(mock_binding_sample_pointer_1),
                                                           SampleReferenceGuard{}};
    impl::tracing::TypeErasedSamplePtr dummy_type_erased_sample_pointer_1{std::move(dummy_sample_pointer_1)};

    // Given a TracingRuntimeObject
    TracingRuntime tracing_runtime{kNumberOfTracingServiceElements, *dummy_configuration_};

    // expect the UuT to return true, when we call RegisterWithGenericTraceApi on it
    ASSERT_TRUE(tracing_runtime.RegisterWithGenericTraceApi());

    // When registering a service element
    const auto trace_context_id_0 = tracing_runtime.RegisterServiceElement();
    const auto trace_context_id_1 = tracing_runtime.RegisterServiceElement();

    // and when setting the type erased sample ptr with the provided TraceContextIds
    tracing_runtime.SetTypeErasedSamplePtr(std::move(kDummyTypeErasedSamplePtr), trace_context_id_0);
    tracing_runtime.SetTypeErasedSamplePtr(std::move(dummy_type_erased_sample_pointer_1), trace_context_id_1);

    // Then tracing should be marked as active for both TraceContextIds
    EXPECT_TRUE(tracing_runtime.IsServiceElementTracingActive(trace_context_id_0));
    EXPECT_TRUE(tracing_runtime.IsServiceElementTracingActive(trace_context_id_1));

    // Then when calling the trace done callback with the second TraceContextId
    trace_done_callback_(trace_context_id_1);

    // Then tracing should no longer be active for the second TraceContextId
    EXPECT_TRUE(tracing_runtime.IsServiceElementTracingActive(trace_context_id_0));
    EXPECT_FALSE(tracing_runtime.IsServiceElementTracingActive(trace_context_id_1));

    // and when calling the trace done callback with the first TraceContextId
    trace_done_callback_(trace_context_id_0);

    // then tracing should no longer be active for either TraceContextId
    EXPECT_FALSE(tracing_runtime.IsServiceElementTracingActive(trace_context_id_0));
    EXPECT_FALSE(tracing_runtime.IsServiceElementTracingActive(trace_context_id_1));
}

TEST_F(TraceDoneCallbackFixture,
       ServiceElementTracingIsUnchangedAfterCallingTraceDoneCallbackWithCorrectContextIdASecondTime)
{
    RecordProperty("Verifies", "1");
    RecordProperty("Description",
                   "Calling the trace done callback with the correct context id a second time will print a warning.");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    // Given a TracingRuntimeObject
    TracingRuntime tracing_runtime{kNumberOfTracingServiceElements, *dummy_configuration_};

    // expect the UuT to return true, when we call RegisterWithGenericTraceApi on it
    ASSERT_TRUE(tracing_runtime.RegisterWithGenericTraceApi());

    // When registering a service element
    const auto trace_context_id = tracing_runtime.RegisterServiceElement();

    // and when setting the type erased sample ptr with the provided TraceContextId
    tracing_runtime.SetTypeErasedSamplePtr(std::move(kDummyTypeErasedSamplePtr), trace_context_id);

    // Then tracing should be marked as active
    EXPECT_TRUE(tracing_runtime.IsServiceElementTracingActive(trace_context_id));

    // Then when calling the trace done callback with the provided TraceContextId
    trace_done_callback_(trace_context_id);

    // and tracing should no longer be active
    EXPECT_FALSE(tracing_runtime.IsServiceElementTracingActive(trace_context_id));

    // capture stdout output during trace done callback call.
    testing::internal::CaptureStdout();

    // Then when calling the trace done callback with the provided TraceContextId again
    trace_done_callback_(trace_context_id);

    // stop capture and get captured data.
    std::string log_output = testing::internal::GetCapturedStdout();
    const char log_warn_snippet[] = "log warn";

    std::stringstream text_snippet{};
    text_snippet << "Lola TracingRuntime: TraceDoneCB with TraceContextId " << trace_context_id
                 << " was not pending but has been called anyway. Ignoring callback.";

    // Then a warning message should be logged (mw::log)
    auto first_offset = log_output.find(log_warn_snippet);
    EXPECT_TRUE(first_offset != log_output.npos);
    EXPECT_TRUE(log_output.find(text_snippet.str(), first_offset) != log_output.npos);
}

TEST_F(TraceDoneCallbackFixture, ServiceElementTracingIsUnchangedAfterCallingTraceDoneCallbackWithIncorrectContextId)
{
    RecordProperty("Verifies", "1");
    RecordProperty("Description", "Calling the trace done callback with an incorrect context id will print a warning.");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    // Given a TracingRuntimeObject
    TracingRuntime tracing_runtime{kNumberOfTracingServiceElements, *dummy_configuration_};

    // expect the UuT to return true, when we call RegisterWithGenericTraceApi on it
    ASSERT_TRUE(tracing_runtime.RegisterWithGenericTraceApi());

    // When registering a service element
    const auto trace_context_id = tracing_runtime.RegisterServiceElement();

    // and when setting the type erased sample ptr with the provided TraceContextId
    tracing_runtime.SetTypeErasedSamplePtr(std::move(kDummyTypeErasedSamplePtr), trace_context_id);

    // Then tracing should be marked as active
    EXPECT_TRUE(tracing_runtime.IsServiceElementTracingActive(trace_context_id));

    // capture stdout output during trace done callback call.
    testing::internal::CaptureStdout();

    // Then when calling the trace done callback with a different TraceContextId
    analysis::tracing::TraceContextId different_trace_context_id = trace_context_id + 1;
    trace_done_callback_(different_trace_context_id);

    // stop capture and get captured data.
    std::string log_output = testing::internal::GetCapturedStdout();
    const char log_warn_snippet[] = "log warn";

    std::stringstream text_snippet{};
    text_snippet << "Lola TracingRuntime: TraceDoneCB with TraceContextId " << different_trace_context_id
                 << " was not pending but has been called anyway. Ignoring callback.";

    // Then a warning message should be logged (mw::log)
    auto first_offset = log_output.find(log_warn_snippet);
    EXPECT_TRUE(first_offset != log_output.npos);
    EXPECT_TRUE(log_output.find(text_snippet.str(), first_offset) != log_output.npos);

    // and tracing should still be active
    EXPECT_TRUE(tracing_runtime.IsServiceElementTracingActive(trace_context_id));
}

class TracingRuntimeConvertToTracingServiceInstanceElementFixture : public testing::Test
{
  public:
    const std::string service_type_name_{"my_service_type"};
    const std::uint32_t major_version_number_{20U};
    const std::uint32_t minor_version_number_{30U};

    const InstanceSpecifier instance_specifier_{InstanceSpecifier::Create("my_instance_specifier").value()};

    const LolaServiceInstanceId::InstanceId instance_id_{12U};
    const LolaServiceId service_id_{13U};
    const std::string event_name_{"my_event_name"};
    const LolaEventId event_id_{2U};
    const std::string field_name_{"my_field_name"};
    const LolaFieldId field_id_{3U};
};

TEST_F(TracingRuntimeConvertToTracingServiceInstanceElementFixture,
       ConvertFunctionGeneratesServiceInstanceElementFromConfig)
{
    const auto service_identifier_type =
        make_ServiceIdentifierType(service_type_name_, major_version_number_, minor_version_number_);
    const LolaServiceInstanceDeployment lola_service_instance_deployment{LolaServiceInstanceId{instance_id_}};
    const auto lola_service_type_deployment =
        CreateTypeDeployment(service_id_, {{event_name_, event_id_}}, {{field_name_, field_id_}});

    Configuration configuration{
        {{service_identifier_type, ServiceTypeDeployment{lola_service_type_deployment}}},
        {{instance_specifier_,
          ServiceInstanceDeployment{
              service_identifier_type, lola_service_instance_deployment, QualityType::kInvalid, instance_specifier_}}},
        GlobalConfiguration{},
        TracingConfiguration{}};

    const ServiceInstanceElement expected_service_instance_element_event{
        static_cast<ServiceInstanceElement::ServiceIdType>(service_id_),
        major_version_number_,
        minor_version_number_,
        static_cast<ServiceInstanceElement::InstanceIdType>(instance_id_),
        static_cast<ServiceInstanceElement::EventIdType>(event_id_)};
    const ServiceInstanceElement expected_service_instance_element_field{
        static_cast<ServiceInstanceElement::ServiceIdType>(service_id_),
        major_version_number_,
        minor_version_number_,
        static_cast<ServiceInstanceElement::InstanceIdType>(instance_id_),
        static_cast<ServiceInstanceElement::FieldIdType>(field_id_)};

    // Given a TracingRuntimeObject with a provided configuration object
    TracingRuntime tracing_runtime{kNumberOfTracingServiceElements, configuration};

    // When converting ServiceElementIdentifierViews to ServiceInstanceElements
    impl::tracing::ServiceElementInstanceIdentifierView service_element_instance_identifier_view_event{
        {service_type_name_, event_name_, impl::tracing::ServiceElementType::EVENT}, instance_specifier_.ToString()};
    const auto actual_service_instance_element_event =
        tracing_runtime.ConvertToTracingServiceInstanceElement(service_element_instance_identifier_view_event);

    impl::tracing::ServiceElementInstanceIdentifierView service_element_instance_identifier_view_field{
        {service_type_name_, field_name_, impl::tracing::ServiceElementType::FIELD}, instance_specifier_.ToString()};
    const auto actual_service_instance_element_field =
        tracing_runtime.ConvertToTracingServiceInstanceElement(service_element_instance_identifier_view_field);

    // Then the result should correspond to the provided configuration
    EXPECT_EQ(actual_service_instance_element_event, expected_service_instance_element_event);
    EXPECT_EQ(actual_service_instance_element_field, expected_service_instance_element_field);
}

using TracingRuntimeConvertToTracingServiceInstanceElementDeathFixture =
    TracingRuntimeConvertToTracingServiceInstanceElementFixture;
TEST_F(TracingRuntimeConvertToTracingServiceInstanceElementDeathFixture,
       CallingConvertFunctionOnElementWithoutInstanceIdTerminates)
{
    const auto service_identifier_type =
        make_ServiceIdentifierType(service_type_name_, major_version_number_, minor_version_number_);
    const LolaServiceInstanceDeployment lola_service_instance_deployment_without_instance_id{};
    const auto lola_service_type_deployment =
        CreateTypeDeployment(service_id_, {{event_name_, event_id_}}, {{field_name_, field_id_}});

    Configuration configuration{{{service_identifier_type, ServiceTypeDeployment{lola_service_type_deployment}}},
                                {{instance_specifier_,
                                  ServiceInstanceDeployment{service_identifier_type,
                                                            lola_service_instance_deployment_without_instance_id,
                                                            QualityType::kInvalid,
                                                            instance_specifier_}}},
                                GlobalConfiguration{},
                                TracingConfiguration{}};

    // Given a TracingRuntimeObject with a provided configuration object which does not contain an instance id
    TracingRuntime tracing_runtime{kNumberOfTracingServiceElements, configuration};

    // When converting a ServiceElementIdentifierView to a ServiceInstanceElement
    // Then we should terminate
    impl::tracing::ServiceElementInstanceIdentifierView service_element_instance_identifier_view_event{
        {service_type_name_, event_name_, impl::tracing::ServiceElementType::EVENT}, instance_specifier_.ToString()};
    EXPECT_DEATH(tracing_runtime.ConvertToTracingServiceInstanceElement(service_element_instance_identifier_view_event),
                 ".*");
}

TEST_F(TracingRuntimeConvertToTracingServiceInstanceElementDeathFixture,
       CallingConvertFunctionOnElementWithInvalidElementTypeTerminates)
{
    const auto service_identifier_type =
        make_ServiceIdentifierType(service_type_name_, major_version_number_, minor_version_number_);
    const LolaServiceInstanceDeployment lola_service_instance_deployment{LolaServiceInstanceId{instance_id_}};
    const auto lola_service_type_deployment =
        CreateTypeDeployment(service_id_, {{event_name_, event_id_}}, {{field_name_, field_id_}});

    Configuration configuration{
        {{service_identifier_type, ServiceTypeDeployment{lola_service_type_deployment}}},
        {{instance_specifier_,
          ServiceInstanceDeployment{
              service_identifier_type, lola_service_instance_deployment, QualityType::kInvalid, instance_specifier_}}},
        GlobalConfiguration{},
        TracingConfiguration{}};

    // Given a TracingRuntimeObject with a provided configuration object
    TracingRuntime tracing_runtime{kNumberOfTracingServiceElements, configuration};

    // When converting a ServiceElementIdentifierView with an invalid ServiceElementType to a ServiceInstanceElement
    // Then we should terminate
    impl::tracing::ServiceElementInstanceIdentifierView service_element_instance_identifier_view_event{
        {service_type_name_, event_name_, impl::tracing::ServiceElementType::INVALID}, instance_specifier_.ToString()};
    EXPECT_DEATH(tracing_runtime.ConvertToTracingServiceInstanceElement(service_element_instance_identifier_view_event),
                 ".*");
}

}  // namespace
}  // namespace tracing
}  // namespace lola
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
