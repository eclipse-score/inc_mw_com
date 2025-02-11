// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#include "platform/aas/mw/com/impl/tracing/tracing_runtime.h"

#include "platform/aas/analysis/tracing/library/generic_trace_api/error_code/error_code.h"
#include "platform/aas/analysis/tracing/library/generic_trace_api/mocks/trace_library_mock.h"
#include "platform/aas/lib/memory/shared/pointer_arithmetic_util.h"
#include "platform/aas/mw/com/impl/bindings/mock_binding/tracing/tracing_runtime.h"
#include "platform/aas/mw/com/impl/tracing/trace_error.h"

#include <gmock/gmock.h>

#include <gtest/gtest.h>
#include <memory>
#include <utility>

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

class TracingRuntimeAttorney
{
  public:
    TracingRuntimeAttorney(TracingRuntime& tracing_runtime) noexcept : tracing_runtime_{tracing_runtime} {};

    bool IsTracingEnabled() const noexcept { return tracing_runtime_.atomic_state_.is_tracing_enabled; }
    void SetTracingEnabled(bool enabled) noexcept { tracing_runtime_.atomic_state_.is_tracing_enabled = enabled; }

    std::uint32_t GetFailureCounter() const noexcept
    {
        return tracing_runtime_.atomic_state_.consecutive_failure_counter;
    }
    void SetFailureCounter(std::uint32_t counter) noexcept
    {
        tracing_runtime_.atomic_state_.consecutive_failure_counter = counter;
    }

    std::unordered_map<BindingType, ITracingRuntimeBinding*>& GetTracingRuntimeBindings()
    {
        return tracing_runtime_.tracing_runtime_bindings_;
    }

  private:
    TracingRuntime& tracing_runtime_;
};

namespace
{
constexpr amp::string_view kDummyServiceTypeName{"my_service_type"};
constexpr amp::string_view kDummyElementName{"my_event"};
constexpr amp::string_view kInstanceSpecifier{"/my_service_type_port"};

using testing::_;
using testing::ByRef;
using testing::Eq;
using testing::Invoke;
using testing::Return;
using testing::WithArg;

class MySamplePtrType
{
  public:
    MySamplePtrType() = default;
    MySamplePtrType(MySamplePtrType& other) = delete;
    MySamplePtrType(MySamplePtrType&& other) = default;
};

class TracingRuntimeFixture : public ::testing::Test
{
  public:
    void SetUp() override
    {
        generic_trace_api_mock_ = std::make_unique<bmw::analysis::tracing::TraceLibraryMock>();
        std::unordered_map<BindingType, ITracingRuntimeBinding*> tracing_runtime_binding_map;
        tracing_runtime_binding_map.emplace(BindingType::kLoLa, &tracing_runtime_binding_mock_);
        EXPECT_CALL(tracing_runtime_binding_mock_, RegisterWithGenericTraceApi()).WillOnce(Return(true));

        unit_under_test_ = std::make_unique<TracingRuntime>(std::move(tracing_runtime_binding_map));
        ASSERT_TRUE(unit_under_test_);
        TracingRuntimeAttorney attorney{*unit_under_test_};
        EXPECT_EQ(attorney.GetFailureCounter(), 0U);
        EXPECT_TRUE(attorney.IsTracingEnabled());
    };

    void SetupTracingRuntimeBindingMockForShmDataTraceCall()
    {
        // expect, that there is currently no tracing call active
        ON_CALL(tracing_runtime_binding_mock_, IsServiceElementTracingActive(trace_context_id_))
            .WillByDefault(Return(false));
        // expect that UuT gets a valid ShmObject from the binding tracing runtime for the given service element
        // instance
        ON_CALL(tracing_runtime_binding_mock_, GetShmObjectHandle(dummy_service_element_instance_identifier_view_))
            .WillByDefault(Return(dummy_shm_object_handle_));
        // expect that UuT gets a valid ShmRegionStartAddress from the binding tracing runtime for the given service
        // element instance
        ON_CALL(tracing_runtime_binding_mock_,
                GetShmRegionStartAddress(dummy_service_element_instance_identifier_view_))
            .WillByDefault(Return(dummy_shm_object_start_address_));
        ON_CALL(tracing_runtime_binding_mock_, GetDataLossFlag()).WillByDefault(Return(false));
        ON_CALL(tracing_runtime_binding_mock_, GetTraceClientId()).WillByDefault(Return(trace_client_id_));

        analysis::tracing::ServiceInstanceElement::EventIdType event_id = 42U;
        analysis::tracing::ServiceInstanceElement::VariantType variant{event_id};
        analysis::tracing::ServiceInstanceElement service_instance_element{25, 1, 0, 1, variant};
        ON_CALL(tracing_runtime_binding_mock_,
                ConvertToTracingServiceInstanceElement(dummy_service_element_instance_identifier_view_))
            .WillByDefault(Return(service_instance_element));
    }

    void SetupTracingRuntimeBindingMockForLocalDataTraceCall()
    {
        // expect, that there is currently no tracing call active
        ON_CALL(tracing_runtime_binding_mock_, IsServiceElementTracingActive(trace_context_id_))
            .WillByDefault(Return(false));
        ON_CALL(tracing_runtime_binding_mock_, GetDataLossFlag()).WillByDefault(Return(false));
        ON_CALL(tracing_runtime_binding_mock_, GetTraceClientId()).WillByDefault(Return(trace_client_id_));

        analysis::tracing::ServiceInstanceElement::EventIdType event_id = 42U;
        analysis::tracing::ServiceInstanceElement::VariantType variant{event_id};
        analysis::tracing::ServiceInstanceElement service_instance_element{25, 1, 0, 1, variant};
        ON_CALL(tracing_runtime_binding_mock_,
                ConvertToTracingServiceInstanceElement(dummy_service_element_instance_identifier_view_))
            .WillByDefault(Return(service_instance_element));
    }

    TypeErasedSamplePtr CreateDummySamplePtr() { return TypeErasedSamplePtr{MySamplePtrType()}; }

    std::unique_ptr<TracingRuntime> unit_under_test_{nullptr};
    mock_binding::TracingRuntime tracing_runtime_binding_mock_{};
    std::unique_ptr<bmw::analysis::tracing::TraceLibraryMock> generic_trace_api_mock_{};
    ServiceElementIdentifierView dummy_service_element_identifier_view_{kDummyServiceTypeName,
                                                                        kDummyElementName,
                                                                        ServiceElementType::EVENT};
    ServiceElementInstanceIdentifierView dummy_service_element_instance_identifier_view_{
        dummy_service_element_identifier_view_,
        kInstanceSpecifier};
    TracingRuntime::TracePointDataId dummy_data_id_{42};
    void* dummy_shm_data_ptr_{reinterpret_cast<void*>(static_cast<intptr_t>(1111))};
    void* dummy_shm_object_start_address_ = reinterpret_cast<void*>(static_cast<uintptr_t>(1000));
    std::size_t dummy_shm_data_size_{23};
    analysis::tracing::ShmObjectHandle dummy_shm_object_handle_{77};
    impl::tracing::ITracingRuntimeBinding::TraceContextId trace_context_id_{1U};
    analysis::tracing::TraceClientId trace_client_id_{1};
};

TEST(TracingRuntimeMove, MoveAssign)
{
    mock_binding::TracingRuntime tracing_runtime_binding_mock_1;
    mock_binding::TracingRuntime tracing_runtime_binding_mock_2;
    mock_binding::TracingRuntime tracing_runtime_binding_mock_3;
    std::unordered_map<BindingType, ITracingRuntimeBinding*> tracing_runtime_binding_map_1;
    tracing_runtime_binding_map_1.emplace(BindingType::kLoLa, &tracing_runtime_binding_mock_1);
    tracing_runtime_binding_map_1.emplace(BindingType::kFake, &tracing_runtime_binding_mock_2);
    std::unordered_map<BindingType, ITracingRuntimeBinding*> tracing_runtime_binding_map_2;
    tracing_runtime_binding_map_2.emplace(BindingType::kLoLa, &tracing_runtime_binding_mock_3);

    TracingRuntime runtime_1(std::move(tracing_runtime_binding_map_1));
    TracingRuntime runtime_2(std::move(tracing_runtime_binding_map_2));

    TracingRuntimeAttorney attorney_1{runtime_1};
    TracingRuntimeAttorney attorney_2{runtime_2};
    attorney_2.SetTracingEnabled(false);
    attorney_2.SetFailureCounter(42);

    runtime_1 = std::move(runtime_2);
    EXPECT_FALSE(attorney_1.IsTracingEnabled());
    EXPECT_EQ(attorney_1.GetFailureCounter(), 42);
    EXPECT_EQ(attorney_1.GetTracingRuntimeBindings().size(), 1);
}

TEST(TracingRuntimeMove, MoveConstruct)
{
    mock_binding::TracingRuntime tracing_runtime_binding_mock_1;
    mock_binding::TracingRuntime tracing_runtime_binding_mock_2;

    std::unordered_map<BindingType, ITracingRuntimeBinding*> tracing_runtime_binding_map_1;
    tracing_runtime_binding_map_1.emplace(BindingType::kLoLa, &tracing_runtime_binding_mock_1);
    tracing_runtime_binding_map_1.emplace(BindingType::kFake, &tracing_runtime_binding_mock_2);

    TracingRuntime runtime_1(std::move(tracing_runtime_binding_map_1));
    TracingRuntimeAttorney attorney_1{runtime_1};
    attorney_1.SetTracingEnabled(false);
    attorney_1.SetFailureCounter(42);

    TracingRuntime runtime_2(std::move(runtime_1));
    TracingRuntimeAttorney attorney_2{runtime_2};

    EXPECT_FALSE(attorney_2.IsTracingEnabled());
    EXPECT_EQ(attorney_2.GetFailureCounter(), 42);
}

TEST(TracingRuntime, TracingRuntimeTraceWillReceivePointerToConstShmData)
{
    RecordProperty("Verifies", "7");
    RecordProperty(
        "Description",
        "Checks that the pointer to the shared memory data to be traced is passed to the TracingRuntime::Trace as "
        "a pointer to const.");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    using ShmPointerType = const void*;

    // Check that ShmPointerType is the type that is passed to TracingRuntime::Trace
    constexpr auto trace_shm_signature = static_cast<bmw::ResultBlank (bmw::mw::com::impl::tracing::TracingRuntime::*)(
        BindingType,
        ServiceElementInstanceIdentifierView,
        ITracingRuntime::TracePointType,
        amp::optional<ITracingRuntime::TracePointDataId>,
        ShmPointerType,
        std::size_t)>(&TracingRuntime::Trace);
    static_assert(std::is_member_function_pointer_v<decltype(trace_shm_signature)>,
                  "shm_trace_signature is not a method of TracingRuntime.");

    // Check that ShmPointerType is a pointer to const
    static_assert(std::is_const_v<std::remove_pointer_t<ShmPointerType>>,
                  "Pointer to shared memory is not a pointer to const.");
}

TEST(TracingRuntimeDisable, TraceClientRegistrationFails)
{
    RecordProperty("Verifies", "2");
    RecordProperty("Description",
                   "Checks whether the binding specific tracing runtimes are triggered to register themselves as "
                   "clients and that a failure even with one client leads to global disabling of tracing.");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    // given two binding specific tracing runtimes
    mock_binding::TracingRuntime tracing_runtime_binding_mock_1;
    mock_binding::TracingRuntime tracing_runtime_binding_mock_2;
    std::unordered_map<BindingType, ITracingRuntimeBinding*> tracing_runtime_binding_map;
    tracing_runtime_binding_map.emplace(BindingType::kLoLa, &tracing_runtime_binding_mock_1);
    tracing_runtime_binding_map.emplace(BindingType::kFake, &tracing_runtime_binding_mock_2);

    // expect, that one of those binding specific tracing runtimes registers successfully with the GenericTraceAPI
    EXPECT_CALL(tracing_runtime_binding_mock_1, RegisterWithGenericTraceApi()).WillOnce(Return(true));
    // expect, that the other of those binding specific tracing runtimes registers NOT successfully with the
    // GenericTraceAPI
    EXPECT_CALL(tracing_runtime_binding_mock_2, RegisterWithGenericTraceApi()).WillOnce(Return(false));

    // when the tracing runtime is created with those two binding specific runtimes
    TracingRuntime runtime(std::move(tracing_runtime_binding_map));

    TracingRuntimeAttorney attorney{runtime};
    // then expect, that tracing is globally disabled.
    EXPECT_FALSE(attorney.IsTracingEnabled());
}

TEST_F(TracingRuntimeFixture, SetDataLossFlag)
{
    // given a UuT which delegates to a mock ITracingRuntimeBinding in case of BindingType::kLoLa
    ASSERT_TRUE(unit_under_test_);

    // expect, that SetDataLossFlag(true) is called on the ITracingRuntimeBinding
    EXPECT_CALL(tracing_runtime_binding_mock_, SetDataLossFlag(true)).Times(1);

    // when we call SetDataLossFlag(BindingType::kLoLa) on the UuT
    unit_under_test_->SetDataLossFlag(BindingType::kLoLa);
}

using TracingRuntimeTraceShmFixture = TracingRuntimeFixture;
TEST_F(TracingRuntimeTraceShmFixture, CallingTraceDispatchesToBinding)
{
    RecordProperty("Verifies", "5, 1");
    RecordProperty(
        "Description",
        "Checks whether the right Trace call is done for data residing in shared-mem (5) and the right "
        "usage of ShmDataChunkList (1).");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    // given a UuT which delegates to a mock ITracingRuntimeBinding in case of BindingType::kLoLa

    SetupTracingRuntimeBindingMockForShmDataTraceCall();

    analysis::tracing::SharedMemoryLocation root_chunk_memory_location{
        dummy_shm_object_handle_,
        static_cast<size_t>(memory::shared::SubtractPointers(dummy_shm_data_ptr_, dummy_shm_object_start_address_))};
    analysis::tracing::SharedMemoryChunk root_chunk{root_chunk_memory_location, dummy_shm_data_size_};
    analysis::tracing::ShmDataChunkList expected_shm_chunk_list{root_chunk};

    EXPECT_CALL(*generic_trace_api_mock_.get(),
                Trace(trace_client_id_, _, Eq(ByRef(expected_shm_chunk_list)), trace_context_id_))
        .WillOnce(Return(analysis::tracing::TraceResult{}));

    // when we call Trace on the UuT
    auto result = unit_under_test_->Trace(BindingType::kLoLa,
                                          trace_context_id_,
                                          dummy_service_element_instance_identifier_view_,
                                          SkeletonEventTracePointType::SEND,
                                          dummy_data_id_,
                                          CreateDummySamplePtr(),
                                          dummy_shm_data_ptr_,
                                          dummy_shm_data_size_);
    EXPECT_TRUE(result.has_value());
}

TEST_F(TracingRuntimeTraceShmFixture, CallingTraceWillClearDataLossFlagOnSuccess)
{
    RecordProperty("Verifies", "3");
    RecordProperty("Description", "Checks reset of the data loss flag after successful Trace call.");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    // given a UuT which delegates to a mock ITracingRuntimeBinding in case of BindingType::kLoLa

    SetupTracingRuntimeBindingMockForShmDataTraceCall();

    EXPECT_CALL(tracing_runtime_binding_mock_, SetDataLossFlag(false)).Times(1);

    // when we call Trace on the UuT
    auto result = unit_under_test_->Trace(BindingType::kLoLa,
                                          trace_context_id_,
                                          dummy_service_element_instance_identifier_view_,
                                          SkeletonEventTracePointType::SEND,
                                          dummy_data_id_,
                                          CreateDummySamplePtr(),
                                          dummy_shm_data_ptr_,
                                          dummy_shm_data_size_);

    EXPECT_TRUE(result.has_value());
}

TEST_F(TracingRuntimeTraceShmFixture, CallingTraceWillIndicateThatShmIsCurrentlyBeingTraced)
{
    RecordProperty("Verifies", "5");
    RecordProperty("Description",
                   "Calling Trace will notify the binding that data in shared memory is currently being traced.");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    // given a UuT which delegates to a mock ITracingRuntimeBinding in case of BindingType::kLoLa
    SetupTracingRuntimeBindingMockForShmDataTraceCall();

    // then SetTypeErasedSamplePtr will be called on the binding, which indicates to the binding that tracing of shared
    // memory data is currently active.
    EXPECT_CALL(tracing_runtime_binding_mock_, SetTypeErasedSamplePtr(_, trace_context_id_)).Times(1);

    // when we call Trace on the UuT
    auto result = unit_under_test_->Trace(BindingType::kLoLa,
                                          trace_context_id_,
                                          dummy_service_element_instance_identifier_view_,
                                          SkeletonEventTracePointType::SEND,
                                          dummy_data_id_,
                                          CreateDummySamplePtr(),
                                          dummy_shm_data_ptr_,
                                          dummy_shm_data_size_);

    EXPECT_TRUE(result.has_value());
}

TEST_F(TracingRuntimeTraceShmFixture, CallingTraceWhileShmIsCurrentlyBeingTracedWillNotTraceAndWillSetDataLossFlag)
{
    RecordProperty("Verifies", "3");
    RecordProperty("Description",
                   "Calling Trace when the binding indicates that shared memory is currently being traced will not "
                   "Trace and will set the data loss flag.");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    // given a UuT which delegates to a mock ITracingRuntimeBinding in case of BindingType::kLoLa
    SetupTracingRuntimeBindingMockForShmDataTraceCall();

    // expect, that there is currently a tracing call active
    EXPECT_CALL(tracing_runtime_binding_mock_, IsServiceElementTracingActive(trace_context_id_)).WillOnce(Return(true));

    // and that the data loss flag will be set
    EXPECT_CALL(tracing_runtime_binding_mock_, SetDataLossFlag(true));

    // and Trace will not be called on the binding
    EXPECT_CALL(*generic_trace_api_mock_.get(), Trace(trace_client_id_, _, _, trace_context_id_)).Times(0);

    // when we call Trace on the UuT
    auto result = unit_under_test_->Trace(BindingType::kLoLa,
                                          trace_context_id_,
                                          dummy_service_element_instance_identifier_view_,
                                          SkeletonEventTracePointType::SEND,
                                          dummy_data_id_,
                                          CreateDummySamplePtr(),
                                          dummy_shm_data_ptr_,
                                          dummy_shm_data_size_);

    EXPECT_TRUE(result.has_value());
}

TEST_F(TracingRuntimeTraceShmFixture, TraceShmDataOK_RetryShmObjectRegistration)
{
    RecordProperty("Verifies", "5, 1, 7, 2");
    RecordProperty(
        "Description",
        "Checks whether the right Trace call is done for data residing in shared-mem (5) and the right "
        "usage of ShmDataChunkList (1). Also checks the transmission data loss flag (7). "
        "Additionally it tests, that re-registration of a previous/once failed ShmObject registration is done "
        "(2)");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    // given a UuT which delegates to a mock ITracingRuntimeBinding in case of BindingType::kLoLa
    ASSERT_TRUE(unit_under_test_);

    analysis::tracing::ShmObjectHandle shm_object_handle{77};
    memory::shared::ISharedMemoryResource::FileDescriptor shm_file_descriptor{1};

    // expect, that there is currently no tracing call active
    EXPECT_CALL(tracing_runtime_binding_mock_, IsServiceElementTracingActive(trace_context_id_))
        .WillOnce(Return(false));
    // expect, that the binding specific tracing runtime doesn't have a ShmObjectHandle for the given identifier
    EXPECT_CALL(tracing_runtime_binding_mock_, GetShmObjectHandle(dummy_service_element_instance_identifier_view_))
        .WillOnce(Return(amp::optional<analysis::tracing::ShmObjectHandle>{}));
    // then expect, that UuT calls GetCachedFileDescriptorForReregisteringShmObject() on binding specific tracing
    // runtime
    EXPECT_CALL(tracing_runtime_binding_mock_,
                GetCachedFileDescriptorForReregisteringShmObject(dummy_service_element_instance_identifier_view_))
        .WillOnce(Return(amp::optional<std::pair<memory::shared::ISharedMemoryResource::FileDescriptor, void*>>{
            {shm_file_descriptor, dummy_shm_object_start_address_}}));
    // and expect, that it then retries the RegisterShmObject() call on the GenericTraceAPI, which is successful and
    // returns a ShmObjectHandle
    EXPECT_CALL(*generic_trace_api_mock_.get(), RegisterShmObject(trace_client_id_, shm_file_descriptor))
        .WillOnce(Return(analysis::tracing::RegisterSharedMemoryObjectResult{shm_object_handle}));
    EXPECT_CALL(
        tracing_runtime_binding_mock_,
        RegisterShmObject(
            dummy_service_element_instance_identifier_view_, shm_object_handle, dummy_shm_object_start_address_))
        .Times(1);
    EXPECT_CALL(tracing_runtime_binding_mock_,
                GetShmRegionStartAddress(dummy_service_element_instance_identifier_view_))
        .WillOnce(Return(dummy_shm_object_start_address_));
    EXPECT_CALL(tracing_runtime_binding_mock_, GetDataLossFlag()).WillOnce(Return(false));
    EXPECT_CALL(tracing_runtime_binding_mock_, SetDataLossFlag(false)).Times(1);
    EXPECT_CALL(tracing_runtime_binding_mock_, GetTraceClientId()).WillRepeatedly(Return(trace_client_id_));

    analysis::tracing::ServiceInstanceElement::VariantType variant{};
    analysis::tracing::ServiceInstanceElement service_instance_element{25, 1, 0, 1, variant};
    EXPECT_CALL(tracing_runtime_binding_mock_,
                ConvertToTracingServiceInstanceElement(dummy_service_element_instance_identifier_view_))
        .WillOnce(Return(service_instance_element));
    EXPECT_CALL(tracing_runtime_binding_mock_, SetTypeErasedSamplePtr(_, trace_context_id_)).Times(1);

    EXPECT_CALL(*generic_trace_api_mock_.get(), Trace(trace_client_id_, _, _, trace_context_id_))
        .WillOnce(Return(analysis::tracing::TraceResult{}));

    // when we call Trace on the UuT
    auto result = unit_under_test_->Trace(BindingType::kLoLa,
                                          trace_context_id_,
                                          dummy_service_element_instance_identifier_view_,
                                          SkeletonEventTracePointType::SEND,
                                          dummy_data_id_,
                                          CreateDummySamplePtr(),
                                          dummy_shm_data_ptr_,
                                          dummy_shm_data_size_);

    EXPECT_TRUE(result.has_value());
    TracingRuntimeAttorney attorney{*unit_under_test_.get()};
    EXPECT_TRUE(attorney.IsTracingEnabled());
    EXPECT_EQ(attorney.GetFailureCounter(), 0);
}

TEST_F(TracingRuntimeTraceShmFixture, TraceShmDataNOK_RetryShmObjectRegistrationFailed)
{
    RecordProperty("Verifies", "5, 1, 7, 2");
    RecordProperty(
        "Description",
        "Checks whether the right Trace call is done for data residing in shared-mem (5) and the right "
        "usage of ShmDataChunkList (1). Also checks the transmission data loss flag (7). "
        "Additionally it tests, that re-registration of a previous/once failed ShmObject registration is tried."
        "(2)");

    // given a UuT which delegates to a mock ITracingRuntimeBinding in case of BindingType::kLoLa
    ASSERT_TRUE(unit_under_test_);
    memory::shared::ISharedMemoryResource::FileDescriptor shm_file_descriptor{1};

    // expect, that there is currently no tracing call active
    EXPECT_CALL(tracing_runtime_binding_mock_, IsServiceElementTracingActive(trace_context_id_))
        .WillOnce(Return(false));
    // expect, that the binding specific tracing runtime doesn't have a ShmObjectHandle for the given identifier
    EXPECT_CALL(tracing_runtime_binding_mock_, GetShmObjectHandle(dummy_service_element_instance_identifier_view_))
        .WillOnce(Return(amp::optional<analysis::tracing::ShmObjectHandle>{}));
    // then expect, that UuT calls GetCachedFileDescriptorForReregisteringShmObject() on binding specific tracing
    // runtime
    EXPECT_CALL(tracing_runtime_binding_mock_,
                GetCachedFileDescriptorForReregisteringShmObject(dummy_service_element_instance_identifier_view_))
        .WillOnce(Return(amp::optional<std::pair<memory::shared::ISharedMemoryResource::FileDescriptor, void*>>{
            {shm_file_descriptor, dummy_shm_object_start_address_}}));
    // expect, that UuT calls GetTraceClientId() on the binding specific tracing runtime
    EXPECT_CALL(tracing_runtime_binding_mock_, GetTraceClientId()).WillOnce(Return(trace_client_id_));
    // and expect, that it then retries the RegisterShmObject() call on the GenericTraceAPI, which fails with some error
    EXPECT_CALL(*generic_trace_api_mock_.get(), RegisterShmObject(trace_client_id_, shm_file_descriptor))
        .WillOnce(Return(bmw::MakeUnexpected(analysis::tracing::ErrorCode::kNotEnoughMemoryRecoverable)));
    // and expect, that UuT clears the cached file descriptor to avoid further retries for the failed shm-object.
    EXPECT_CALL(tracing_runtime_binding_mock_,
                ClearCachedFileDescriptorForReregisteringShmObject(dummy_service_element_instance_identifier_view_))
        .Times(1);

    // when we call Trace on the UuT
    auto result = unit_under_test_->Trace(BindingType::kLoLa,
                                          trace_context_id_,
                                          dummy_service_element_instance_identifier_view_,
                                          SkeletonEventTracePointType::SEND,
                                          dummy_data_id_,
                                          CreateDummySamplePtr(),
                                          dummy_shm_data_ptr_,
                                          dummy_shm_data_size_);

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), TraceErrorCode::TraceErrorDisableTracePointInstance);
    TracingRuntimeAttorney attorney{*unit_under_test_.get()};
    EXPECT_TRUE(attorney.IsTracingEnabled());
    EXPECT_EQ(attorney.GetFailureCounter(), 0);
}

TEST_F(TracingRuntimeTraceShmFixture, TraceShmDataNOK_NoCachedFiledescriptor)
{
    RecordProperty("Verifies", "5, 1, 7, 2");
    RecordProperty(
        "Description",
        "Checks whether the right Trace call is done for data residing in shared-mem (5) and the right "
        "usage of ShmDataChunkList (1). Also checks the transmission data loss flag (7). "
        "Additionally it tests, that re-registration of a previous/once failed ShmObject registration is not tried."
        "(2), as there is no cached file descriptor, which means, that re-registering already failed. ");

    // given a UuT which delegates to a mock ITracingRuntimeBinding in case of BindingType::kLoLa
    ASSERT_TRUE(unit_under_test_);

    // expect, that there is currently no tracing call active
    EXPECT_CALL(tracing_runtime_binding_mock_, IsServiceElementTracingActive(trace_context_id_))
        .WillOnce(Return(false));
    // expect, that the binding specific tracing runtime doesn't have a ShmObjectHandle for the given identifier
    EXPECT_CALL(tracing_runtime_binding_mock_, GetShmObjectHandle(dummy_service_element_instance_identifier_view_))
        .WillOnce(Return(amp::optional<analysis::tracing::ShmObjectHandle>{}));
    // then expect, that UuT calls GetCachedFileDescriptorForReregisteringShmObject() on binding specific tracing
    // runtime, which doesn't return any
    EXPECT_CALL(tracing_runtime_binding_mock_,
                GetCachedFileDescriptorForReregisteringShmObject(dummy_service_element_instance_identifier_view_))
        .WillOnce(Return(amp::optional<std::pair<memory::shared::ISharedMemoryResource::FileDescriptor, void*>>{}));

    // when we call Trace on the UuT
    auto result = unit_under_test_->Trace(BindingType::kLoLa,
                                          trace_context_id_,
                                          dummy_service_element_instance_identifier_view_,
                                          SkeletonEventTracePointType::SEND,
                                          dummy_data_id_,
                                          CreateDummySamplePtr(),
                                          dummy_shm_data_ptr_,
                                          dummy_shm_data_size_);

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), TraceErrorCode::TraceErrorDisableTracePointInstance);
    TracingRuntimeAttorney attorney{*unit_under_test_.get()};
    EXPECT_TRUE(attorney.IsTracingEnabled());
    EXPECT_EQ(attorney.GetFailureCounter(), 0);
}

TEST_F(TracingRuntimeTraceShmFixture, TraceShmDataNOK_GetShmRegionStartAddressFailedDeathTest)
{
    // given a UuT which delegates to a mock ITracingRuntimeBinding in case of BindingType::kLoLa
    ASSERT_TRUE(unit_under_test_);

    auto death_test_sequence = [this]() -> void {
        // expect, that there is currently no tracing call active
        EXPECT_CALL(tracing_runtime_binding_mock_, IsServiceElementTracingActive(trace_context_id_))
            .WillOnce(Return(false));
        analysis::tracing::ShmObjectHandle shm_object_handle{77};
        // and expect, that UuT gets successful a ShmObjectHandle for the service element instance from the binding
        // specific tracing runtime
        EXPECT_CALL(tracing_runtime_binding_mock_, GetShmObjectHandle(dummy_service_element_instance_identifier_view_))
            .WillOnce(Return(shm_object_handle));
        // but then the call by UuT to get the shm_start_address for the service element instance doesn't return an
        // address
        EXPECT_CALL(tracing_runtime_binding_mock_,
                    GetShmRegionStartAddress(dummy_service_element_instance_identifier_view_))
            .WillOnce(Return(amp::optional<void*>{}));
        // when we call Trace on the UuT
        unit_under_test_->Trace(BindingType::kLoLa,
                                trace_context_id_,
                                dummy_service_element_instance_identifier_view_,
                                SkeletonEventTracePointType::SEND,
                                dummy_data_id_,
                                CreateDummySamplePtr(),
                                dummy_shm_data_ptr_,
                                dummy_shm_data_size_);
    };

    // we expect to die!
    EXPECT_DEATH(death_test_sequence(), ".*");
}

TEST_F(TracingRuntimeTraceShmFixture, TraceShmDataNOK_NonRecoverableError)
{
    RecordProperty("Verifies", "9");
    RecordProperty(
        "Description",
        "Checks that after a non-recoverable error in Trace() call, the data-loss flag is set, the caller is notified, "
        "to abandon further trace-calls for the same trace-point-type and a log message mit severity warning is "
        "issued");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    // given a UuT which delegates to a mock ITracingRuntimeBinding in case of BindingType::kLoLa
    ASSERT_TRUE(unit_under_test_);

    // expect, that all preconditions for a Trace call to the GenericTraceAPI are met
    SetupTracingRuntimeBindingMockForShmDataTraceCall();

    // expect, that UuT calls Trace on the GenericTraceAPI, which returns a non-recoverable error
    EXPECT_CALL(*generic_trace_api_mock_.get(), Trace(trace_client_id_, _, _, trace_context_id_))
        .WillOnce(Return(bmw::MakeUnexpected(analysis::tracing::ErrorCode::kInvalidArgumentFatal)));

    // expect, that UuT frees sample_ptr on binding specific runtime as this trace call is lost and no trace-done
    // callback will happen, which frees the sample ptr.
    EXPECT_CALL(tracing_runtime_binding_mock_, ClearTypeErasedSamplePtr(trace_context_id_)).Times(1);

    // expect, that UuT sets data-loss-flag on binding specific runtime as this trace call is lost because of error
    EXPECT_CALL(tracing_runtime_binding_mock_, SetDataLossFlag(true)).Times(1);

    // capture stdout output during Trace() call.
    testing::internal::CaptureStdout();

    TracingRuntimeAttorney attorney{*unit_under_test_.get()};
    auto previous_error_counter = attorney.GetFailureCounter();

    // when we call Trace on the UuT
    auto result = unit_under_test_->Trace(BindingType::kLoLa,
                                          trace_context_id_,
                                          dummy_service_element_instance_identifier_view_,
                                          SkeletonEventTracePointType::SEND,
                                          dummy_data_id_,
                                          CreateDummySamplePtr(),
                                          dummy_shm_data_ptr_,
                                          dummy_shm_data_size_);

    // stop capture and get captured data.
    std::string log_output = testing::internal::GetCapturedStdout();
    const char log_warn_snippet[] = "log warn";
    const char text_snippet[] = "TracingRuntime: Disabling Tracing for ";
    // and expect, that the output contains a warning message (mw::log)
    auto first_offset = log_output.find(log_warn_snippet);
    EXPECT_TRUE(first_offset != log_output.npos);
    EXPECT_TRUE(log_output.find(text_snippet, first_offset) != log_output.npos);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), TraceErrorCode::TraceErrorDisableTracePointInstance);

    // expect that tracing is still enabled afterwards because non-recoverable error in Trace() just deactivates the
    // specific trace-point-instance
    EXPECT_TRUE(attorney.IsTracingEnabled());
    EXPECT_EQ(attorney.GetFailureCounter(), previous_error_counter + 1U);
}

TEST_F(TracingRuntimeTraceShmFixture, TraceShmDataNOK_TerminalFatalError)
{
    RecordProperty("Verifies", "4");
    RecordProperty("Description",
                   "Checks that after a terminal fatal error in Trace() call, tracing is "
                   "completely disabled and a log message mit severity warning is issued");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    // given a UuT which delegates to a mock ITracingRuntimeBinding in case of BindingType::kLoLa
    ASSERT_TRUE(unit_under_test_);

    // expect, that all preconditions for a Trace call to the GenericTraceAPI are met
    SetupTracingRuntimeBindingMockForShmDataTraceCall();

    // expect, that UuT calls Trace on the GenericTraceAPI, which returns a terminal fatal error
    EXPECT_CALL(*generic_trace_api_mock_.get(), Trace(trace_client_id_, _, _, trace_context_id_))
        .WillOnce(Return(bmw::MakeUnexpected(analysis::tracing::ErrorCode::kTerminalFatal)));

    // expect, that UuT frees sample_ptr on binding specific runtime as this trace call is lost and no trace-done
    // callback will happen, which frees the sample ptr.
    EXPECT_CALL(tracing_runtime_binding_mock_, ClearTypeErasedSamplePtr(trace_context_id_)).Times(1);

    // capture stdout output during Trace() call.
    testing::internal::CaptureStdout();

    // when we call Trace on the UuT
    auto result = unit_under_test_->Trace(BindingType::kLoLa,
                                          trace_context_id_,
                                          dummy_service_element_instance_identifier_view_,
                                          SkeletonEventTracePointType::SEND,
                                          dummy_data_id_,
                                          CreateDummySamplePtr(),
                                          dummy_shm_data_ptr_,
                                          dummy_shm_data_size_);

    // stop capture and get captured data.
    std::string log_output = testing::internal::GetCapturedStdout();
    const char log_warn_snippet[] = "log warn";
    const char text_snippet[] = "kTerminalFatal";
    // and expect, that the output contains a warning message (mw::log)
    auto first_offset = log_output.find(log_warn_snippet);
    EXPECT_TRUE(first_offset != log_output.npos);
    EXPECT_TRUE(log_output.find(text_snippet, first_offset) != log_output.npos);

    // expect, that there was an error
    EXPECT_FALSE(result.has_value());
    // and expect, that the error code is TraceErrorDisableAllTracePoints
    EXPECT_EQ(result.error(), TraceErrorCode::TraceErrorDisableAllTracePoints);
    // expect, that tracing is globally disabled
    TracingRuntimeAttorney attorney{*unit_under_test_.get()};
    // expect that tracing is disabled afterwards because kTerminalFatal error in Trace() deactivates the tracing
    // completely
    EXPECT_FALSE(attorney.IsTracingEnabled());
}

TEST_F(TracingRuntimeTraceShmFixture, TraceShmDataNOK_RecoverableError)
{
    RecordProperty("Verifies", "5, 1, 7, 3");
    RecordProperty(
        "Description",
        "Checks whether the right Trace call is done for data residing in shared-mem (5) and the right "
        "usage of ShmDataChunkList (1). Also checks the transmission data loss flag (7). "
        "Furthermore checks, that in case of recoverable error the consecutive error counter gets incremented "
        "(3)");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    // given a UuT which delegates to a mock ITracingRuntimeBinding in case of BindingType::kLoLa
    ASSERT_TRUE(unit_under_test_);
    TracingRuntimeAttorney attorney{*unit_under_test_.get()};
    auto previous_failure_counter = attorney.GetFailureCounter();

    // expect, that all preconditions for a Trace call to the GenericTraceAPI are met
    SetupTracingRuntimeBindingMockForShmDataTraceCall();

    // expect, that UuT calls Trace on the GenericTraceAPI, which returns a non-recoverable error
    EXPECT_CALL(*generic_trace_api_mock_.get(), Trace(trace_client_id_, _, _, trace_context_id_))
        .WillOnce(Return(bmw::MakeUnexpected(analysis::tracing::ErrorCode::kRingBufferFullRecoverable)));

    // expect, that UuT frees sample_ptr on binding specific runtime as this trace call is lost and no trace-done
    // callback will happen, which frees the sample ptr.
    EXPECT_CALL(tracing_runtime_binding_mock_, ClearTypeErasedSamplePtr(trace_context_id_)).Times(1);

    // expect, that UuT sets data-loss-flag on binding specific runtime as this trace call is lost because of error
    EXPECT_CALL(tracing_runtime_binding_mock_, SetDataLossFlag(true)).Times(1);

    // when we call Trace on the UuT
    auto result = unit_under_test_->Trace(BindingType::kLoLa,
                                          trace_context_id_,
                                          dummy_service_element_instance_identifier_view_,
                                          SkeletonEventTracePointType::SEND,
                                          dummy_data_id_,
                                          CreateDummySamplePtr(),
                                          dummy_shm_data_ptr_,
                                          dummy_shm_data_size_);

    EXPECT_TRUE(result.has_value());
    // expect that tracing is enabled
    EXPECT_TRUE(attorney.IsTracingEnabled());
    // expect, that failure counter is incremented by one
    EXPECT_EQ(attorney.GetFailureCounter(), previous_failure_counter + 1);
}

TEST_F(TracingRuntimeTraceShmFixture, TraceShmDataNOK_ConsecutiveRecoverableErrors)
{
    RecordProperty("Verifies", "5, 1, 7, 3");
    RecordProperty(
        "Description",
        "Checks whether the right Trace call is done for data residing in shared-mem (5) and the right "
        "usage of ShmDataChunkList (1). Also checks the transmission data loss flag (7) and "
        "that "
        "after a configured amount of consecutive Trace() error, tracing gets completely disabled (3)");
    RecordProperty("Priority", "1");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    // given a UuT which delegates to a mock ITracingRuntimeBinding in case of BindingType::kLoLa
    ASSERT_TRUE(unit_under_test_);

    TracingRuntimeAttorney attorney{*unit_under_test_.get()};
    // expect that tracing is enabled
    EXPECT_TRUE(attorney.IsTracingEnabled());
    EXPECT_EQ(attorney.GetFailureCounter(), 0);

    const auto kRetries{10};
    static_assert(TracingRuntime::MAX_CONSECUTIVE_ACCEPTABLE_TRACE_FAILURES > kRetries);
    const auto kFailureCounterStart{TracingRuntime::MAX_CONSECUTIVE_ACCEPTABLE_TRACE_FAILURES - kRetries};
    attorney.SetFailureCounter(kFailureCounterStart);

    // expect, that all preconditions for a Trace call to the GenericTraceAPI are met
    SetupTracingRuntimeBindingMockForShmDataTraceCall();
    // expect, that UuT calls Trace on the GenericTraceAPI, which returns a recoverable error
    EXPECT_CALL(*generic_trace_api_mock_.get(), Trace(trace_client_id_, _, _, trace_context_id_))
        .Times(kRetries)
        .WillRepeatedly(Return(bmw::MakeUnexpected(analysis::tracing::ErrorCode::kRingBufferFullRecoverable)));
    // expect, that UuT frees sample_ptr on binding specific runtime as this trace call is lost and no trace-done
    // callback will happen, which frees the sample ptr.
    EXPECT_CALL(tracing_runtime_binding_mock_, ClearTypeErasedSamplePtr(trace_context_id_)).Times(kRetries);
    // expect, that UuT sets data-loss-flag on binding specific runtime as this trace call is lost because of
    // error
    EXPECT_CALL(tracing_runtime_binding_mock_, SetDataLossFlag(true)).Times(kRetries);

    for (unsigned i = 0; i < kRetries; i++)
    {
        EXPECT_EQ(attorney.GetFailureCounter(), kFailureCounterStart + i);
        // when we call Trace on the UuT
        auto result = unit_under_test_->Trace(BindingType::kLoLa,
                                              trace_context_id_,
                                              dummy_service_element_instance_identifier_view_,
                                              SkeletonEventTracePointType::SEND,
                                              dummy_data_id_,
                                              CreateDummySamplePtr(),
                                              dummy_shm_data_ptr_,
                                              dummy_shm_data_size_);

        if (i < kRetries - 1)
        {
            EXPECT_TRUE(result.has_value());
            // expect that tracing is enabled
            EXPECT_TRUE(attorney.IsTracingEnabled());
        }
    }
    // expect that tracing is disabled
    EXPECT_FALSE(attorney.IsTracingEnabled());
}

using TracingRuntimeRegisterShmObjectFixture = TracingRuntimeFixture;
TEST_F(TracingRuntimeRegisterShmObjectFixture, RegisterShmObjectOK)
{
    RecordProperty("Verifies", "4");
    RecordProperty("Description", "Verifies, that the correct API from GenericTraceAPI gets called.");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    // given a UuT which delegates to a mock ITracingRuntimeBinding in case of BindingType::kLoLa
    ASSERT_TRUE(unit_under_test_);
    memory::shared::ISharedMemoryResource::FileDescriptor shm_file_descriptor{1};
    void* shm_object_start_address = reinterpret_cast<void*>(static_cast<uintptr_t>(777));
    analysis::tracing::ShmObjectHandle generic_trace_api_shm_handle{5};

    // expect, that UuT calls GetTraceClientId() on the binding specific tracing runtime
    EXPECT_CALL(tracing_runtime_binding_mock_, GetTraceClientId()).WillOnce(Return(trace_client_id_));
    // and that it calls RegisterShmObject() on the GenericTraceAPI, which returns generic_trace_api_shm_handle as the
    // translated shm_file_descriptor
    EXPECT_CALL(*generic_trace_api_mock_.get(), RegisterShmObject(trace_client_id_, shm_file_descriptor))
        .WillOnce(Return(analysis::tracing::RegisterSharedMemoryObjectResult{generic_trace_api_shm_handle}));
    // expect NO call to RegisterShmObject on the binding specific tracing runtime!
    EXPECT_CALL(tracing_runtime_binding_mock_, RegisterShmObject(_, _, _)).Times(0);
    // and that UuT calls RegisterShmObject on the binding specific tracing runtime
    EXPECT_CALL(
        tracing_runtime_binding_mock_,
        RegisterShmObject(
            dummy_service_element_instance_identifier_view_, generic_trace_api_shm_handle, shm_object_start_address))
        .Times(1);

    // when calling RegisterShmObject on the UuT.
    unit_under_test_->RegisterShmObject(BindingType::kLoLa,
                                        dummy_service_element_instance_identifier_view_,
                                        shm_file_descriptor,
                                        shm_object_start_address);

    // expect, that afterwards tracing is still enabled and the failure counter is 0.
    TracingRuntimeAttorney attorney{*unit_under_test_.get()};
    EXPECT_TRUE(attorney.IsTracingEnabled());
    EXPECT_EQ(attorney.GetFailureCounter(), 0);
}

TEST_F(TracingRuntimeRegisterShmObjectFixture, RegisterShmObjectNOK_unrecoverable)
{
    RecordProperty("Verifies", "4, 6");
    RecordProperty("Description",
                   "Verifies, that the correct API from GenericTraceAPI gets called and that in case of an "
                   "unrecoverable error no registration-retry logic is set up.");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    // given a UuT which delegates to a mock ITracingRuntimeBinding in case of BindingType::kLoLa
    ASSERT_TRUE(unit_under_test_);
    memory::shared::ISharedMemoryResource::FileDescriptor shm_file_descriptor{1};
    void* shm_object_start_address = reinterpret_cast<void*>(static_cast<uintptr_t>(777));

    // expect, that UuT calls GetTraceClientId() on the binding specific tracing runtime
    EXPECT_CALL(tracing_runtime_binding_mock_, GetTraceClientId()).WillOnce(Return(trace_client_id_));
    // and that it calls RegisterShmObject() on the GenericTraceAPI, which returns an unrecoverable error
    EXPECT_CALL(*generic_trace_api_mock_.get(), RegisterShmObject(trace_client_id_, shm_file_descriptor))
        .WillOnce(Return(bmw::MakeUnexpected(analysis::tracing::ErrorCode::kInvalidArgumentFatal)));
    // and explicitly expect RegisterShmObject() on the binding specific trace runtime NOT being called.
    EXPECT_CALL(tracing_runtime_binding_mock_, RegisterShmObject(_, _, _)).Times(0);

    // when calling RegisterShmObject on the UuT.
    unit_under_test_->RegisterShmObject(BindingType::kLoLa,
                                        dummy_service_element_instance_identifier_view_,
                                        shm_file_descriptor,
                                        shm_object_start_address);

    // expect, that afterwards tracing is still enabled.
    TracingRuntimeAttorney attorney{*unit_under_test_.get()};
    EXPECT_TRUE(attorney.IsTracingEnabled());
}

TEST_F(TracingRuntimeRegisterShmObjectFixture, RegisterShmObjectNOK_FatalError)
{
    RecordProperty("Verifies", "4");
    RecordProperty("Description",
                   "Checks that after a terminal fatal error in RegisterShmObject() call, tracing is "
                   "completely disabled and a log message mit severity warning is "
                   "issued");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    // given a UuT which delegates to a mock ITracingRuntimeBinding in case of BindingType::kLoLa
    ASSERT_TRUE(unit_under_test_);
    memory::shared::ISharedMemoryResource::FileDescriptor shm_file_descriptor{1};
    void* shm_object_start_address = reinterpret_cast<void*>(static_cast<uintptr_t>(777));

    // expect, that UuT calls GetTraceClientId() on the binding specific tracing runtime
    EXPECT_CALL(tracing_runtime_binding_mock_, GetTraceClientId()).WillOnce(Return(trace_client_id_));
    // and that it calls RegisterShmObject() on the GenericTraceAPI, which returns an unrecoverable error
    EXPECT_CALL(*generic_trace_api_mock_.get(), RegisterShmObject(trace_client_id_, shm_file_descriptor))
        .WillOnce(Return(bmw::MakeUnexpected(analysis::tracing::ErrorCode::kTerminalFatal)));

    // when calling RegisterShmObject on the UuT.
    unit_under_test_->RegisterShmObject(BindingType::kLoLa,
                                        dummy_service_element_instance_identifier_view_,
                                        shm_file_descriptor,
                                        shm_object_start_address);

    // expect, that afterwards tracing is disabled.
    TracingRuntimeAttorney attorney{*unit_under_test_.get()};
    EXPECT_FALSE(attorney.IsTracingEnabled());
}

TEST_F(TracingRuntimeRegisterShmObjectFixture, RegisterShmObjectNOK_recoverable)
{
    RecordProperty("Verifies", "4, 2");
    RecordProperty("Description",
                   "Verifies, that the correct API from GenericTraceAPI gets called and that in case of an "
                   "recoverable error registration-retry logic is set up.");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    // given a UuT which delegates to a mock ITracingRuntimeBinding in case of BindingType::kLoLa
    ASSERT_TRUE(unit_under_test_);
    memory::shared::ISharedMemoryResource::FileDescriptor shm_file_descriptor{1};
    void* shm_object_start_address = reinterpret_cast<void*>(static_cast<uintptr_t>(777));

    // expect, that UuT calls GetTraceClientId() on the binding specific tracing runtime
    EXPECT_CALL(tracing_runtime_binding_mock_, GetTraceClientId()).WillOnce(Return(trace_client_id_));
    // and that it calls RegisterShmObject() on the GenericTraceAPI, which returns a recoverable error
    EXPECT_CALL(*generic_trace_api_mock_.get(), RegisterShmObject(trace_client_id_, shm_file_descriptor))
        .WillOnce(Return(bmw::MakeUnexpected(analysis::tracing::ErrorCode::kMessageSendFailedRecoverable)));
    // and that UuT calls CacheFileDescriptorForReregisteringShmObject() on the binding specific tracing runtime
    EXPECT_CALL(tracing_runtime_binding_mock_,
                CacheFileDescriptorForReregisteringShmObject(
                    dummy_service_element_instance_identifier_view_, shm_file_descriptor, shm_object_start_address))
        .Times(1);

    // when calling RegisterShmObject on the UuT.
    unit_under_test_->RegisterShmObject(BindingType::kLoLa,
                                        dummy_service_element_instance_identifier_view_,
                                        shm_file_descriptor,
                                        shm_object_start_address);

    // expect, that afterwards tracing is still enabled.
    TracingRuntimeAttorney attorney{*unit_under_test_.get()};
    EXPECT_TRUE(attorney.IsTracingEnabled());
}

using TracingRuntimeUnregisterShmObjectFixture = TracingRuntimeFixture;
TEST_F(TracingRuntimeUnregisterShmObjectFixture, UnregisterShmObject)
{
    // given a UuT which delegates to a mock ITracingRuntimeBinding in case of BindingType::kLoLa
    ASSERT_TRUE(unit_under_test_);
    memory::shared::ISharedMemoryResource::FileDescriptor shm_file_descriptor{1};
    void* shm_object_start_address = reinterpret_cast<void*>(static_cast<uintptr_t>(777));
    analysis::tracing::ShmObjectHandle generic_trace_api_shm_handle{5};

    // expect, that UuT calls GetTraceClientId() on the binding specific tracing runtime
    EXPECT_CALL(tracing_runtime_binding_mock_, GetTraceClientId()).Times(2).WillRepeatedly(Return(trace_client_id_));
    // and that it calls RegisterShmObject() on the GenericTraceAPI, which returns generic_trace_api_shm_handle as the
    // translated shm_file_descriptor
    EXPECT_CALL(*generic_trace_api_mock_.get(), RegisterShmObject(trace_client_id_, shm_file_descriptor))
        .WillOnce(Return(analysis::tracing::RegisterSharedMemoryObjectResult{generic_trace_api_shm_handle}));
    // and that UuT calls RegisterShmObject on the binding specific tracing runtime
    EXPECT_CALL(
        tracing_runtime_binding_mock_,
        RegisterShmObject(
            dummy_service_element_instance_identifier_view_, generic_trace_api_shm_handle, shm_object_start_address));

    // and that UuT calls GetShmObjectHandle on the binding specific tracing runtime
    EXPECT_CALL(tracing_runtime_binding_mock_, GetShmObjectHandle(dummy_service_element_instance_identifier_view_))
        .WillOnce(Return(generic_trace_api_shm_handle));
    // and that UuT calls UnregisterShmObject on the binding specific tracing runtime
    EXPECT_CALL(tracing_runtime_binding_mock_, UnregisterShmObject(dummy_service_element_instance_identifier_view_));
    // and that it calls UnregisterShmObject() on the GenericTraceAPI, which does not return an error
    EXPECT_CALL(*generic_trace_api_mock_.get(), UnregisterShmObject(trace_client_id_, generic_trace_api_shm_handle))
        .WillOnce(Return(ResultBlank{}));

    // when calling RegisterShmObject on the UuT.
    unit_under_test_->RegisterShmObject(BindingType::kLoLa,
                                        dummy_service_element_instance_identifier_view_,
                                        shm_file_descriptor,
                                        shm_object_start_address);

    // and then calling UnregisterShmObject on the UuT.
    unit_under_test_->UnregisterShmObject(BindingType::kLoLa, dummy_service_element_instance_identifier_view_);

    // expect, that afterwards tracing is still enabled and the failure counter is 0.
    TracingRuntimeAttorney attorney{*unit_under_test_.get()};
    EXPECT_TRUE(attorney.IsTracingEnabled());
    EXPECT_EQ(attorney.GetFailureCounter(), 0);
}

TEST_F(TracingRuntimeUnregisterShmObjectFixture, UnregisterShmObject_Fails)
{
    // given a UuT which delegates to a mock ITracingRuntimeBinding in case of BindingType::kLoLa
    ASSERT_TRUE(unit_under_test_);
    memory::shared::ISharedMemoryResource::FileDescriptor shm_file_descriptor{1};
    void* shm_object_start_address = reinterpret_cast<void*>(static_cast<uintptr_t>(777));
    analysis::tracing::ShmObjectHandle generic_trace_api_shm_handle{5};

    // expect, that UuT calls GetTraceClientId() on the binding specific tracing runtime
    EXPECT_CALL(tracing_runtime_binding_mock_, GetTraceClientId()).Times(2).WillRepeatedly(Return(trace_client_id_));
    // and that it calls RegisterShmObject() on the GenericTraceAPI, which returns generic_trace_api_shm_handle as the
    // translated shm_file_descriptor
    EXPECT_CALL(*generic_trace_api_mock_.get(), RegisterShmObject(trace_client_id_, shm_file_descriptor))
        .WillOnce(Return(analysis::tracing::RegisterSharedMemoryObjectResult{generic_trace_api_shm_handle}));
    // and that UuT calls RegisterShmObject on the binding specific tracing runtime
    EXPECT_CALL(
        tracing_runtime_binding_mock_,
        RegisterShmObject(
            dummy_service_element_instance_identifier_view_, generic_trace_api_shm_handle, shm_object_start_address));

    // and that UuT calls GetShmObjectHandle on the binding specific tracing runtime
    EXPECT_CALL(tracing_runtime_binding_mock_, GetShmObjectHandle(dummy_service_element_instance_identifier_view_))
        .WillOnce(Return(generic_trace_api_shm_handle));
    // and that UuT calls UnregisterShmObject on the binding specific tracing runtime
    EXPECT_CALL(tracing_runtime_binding_mock_, UnregisterShmObject(dummy_service_element_instance_identifier_view_));
    // and that it calls UnregisterShmObject() on the GenericTraceAPI, which does return an error
    EXPECT_CALL(*generic_trace_api_mock_.get(), UnregisterShmObject(trace_client_id_, generic_trace_api_shm_handle))
        .WillOnce(Return(MakeUnexpected(analysis::tracing::ErrorCode::kNotEnoughMemoryRecoverable)));

    // when calling RegisterShmObject on the UuT.
    unit_under_test_->RegisterShmObject(BindingType::kLoLa,
                                        dummy_service_element_instance_identifier_view_,
                                        shm_file_descriptor,
                                        shm_object_start_address);

    // and then calling UnregisterShmObject on the UuT.
    unit_under_test_->UnregisterShmObject(BindingType::kLoLa, dummy_service_element_instance_identifier_view_);

    // expect, that afterwards tracing is still enabled and the failure counter is 0.
    TracingRuntimeAttorney attorney{*unit_under_test_.get()};
    EXPECT_TRUE(attorney.IsTracingEnabled());
    EXPECT_EQ(attorney.GetFailureCounter(), 0);
}

TEST_F(TracingRuntimeUnregisterShmObjectFixture, UnregisterShmObject_NotRegistered)
{
    // given a UuT which delegates to a mock ITracingRuntimeBinding in case of BindingType::kLoLa
    ASSERT_TRUE(unit_under_test_);

    // and that UuT calls GetShmObjectHandle on the binding specific tracing runtime, which returns an empty optional
    EXPECT_CALL(tracing_runtime_binding_mock_, GetShmObjectHandle(dummy_service_element_instance_identifier_view_))
        .WillOnce(Return(amp::optional<analysis::tracing::ShmObjectHandle>{}));
    // and that UuT calls ClearCachedFileDescriptorForReregisteringShmObject on the binding specific tracing runtime
    EXPECT_CALL(tracing_runtime_binding_mock_,
                ClearCachedFileDescriptorForReregisteringShmObject(dummy_service_element_instance_identifier_view_));

    // when calling UnregisterShmObject on the UuT.
    unit_under_test_->UnregisterShmObject(BindingType::kLoLa, dummy_service_element_instance_identifier_view_);

    // expect, that afterwards tracing is still enabled and the failure counter is 0.
    TracingRuntimeAttorney attorney{*unit_under_test_.get()};
    EXPECT_TRUE(attorney.IsTracingEnabled());
    EXPECT_EQ(attorney.GetFailureCounter(), 0);
}

using TracingRuntimeTraceLocalFixture = TracingRuntimeFixture;
TEST_F(TracingRuntimeTraceLocalFixture, CallingTraceDispatchesToBinding)
{
    RecordProperty("Verifies", "1, 6");
    RecordProperty("Description",
                   "Checks whether the right Trace call is done for local data (1). Also checks the "
                   "handling of LocalDataChunkLists (6)");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    // given a UuT which delegates to a mock ITracingRuntimeBinding in case of BindingType::kLoLa
    ASSERT_TRUE(unit_under_test_);
    TracingRuntimeAttorney attorney{*unit_under_test_.get()};
    auto previous_failure_counter = attorney.GetFailureCounter();

    void* local_data_ptr{reinterpret_cast<void*>(static_cast<intptr_t>(500))};
    std::size_t local_data_size{8};
    amp::optional<TracingRuntime::TracePointDataId> data_id{};

    analysis::tracing::LocalDataChunk root_chunk{local_data_ptr, local_data_size};
    analysis::tracing::LocalDataChunkList expected_chunk_list{root_chunk};

    SetupTracingRuntimeBindingMockForLocalDataTraceCall();

    // and then expect, that UuT calls the GenericTraceAPI::Trace() call with local chunk list
    EXPECT_CALL(*generic_trace_api_mock_.get(), Trace(trace_client_id_, _, Eq(ByRef(expected_chunk_list))))
        .WillOnce(Return(analysis::tracing::TraceResult{}));
    // when we call Trace on the UuT
    auto result = unit_under_test_->Trace(BindingType::kLoLa,
                                          dummy_service_element_instance_identifier_view_,
                                          SkeletonEventTracePointType::SEND,
                                          data_id,
                                          local_data_ptr,
                                          local_data_size);

    // expect, that there was no error
    EXPECT_TRUE(result.has_value());
    // expect, that tracing is still globally enabled
    EXPECT_TRUE(attorney.IsTracingEnabled());
    // expect, that failure counter is still the same
    EXPECT_EQ(attorney.GetFailureCounter(), previous_failure_counter);
}

TEST_F(TracingRuntimeTraceLocalFixture, TraceLocalData_RecoverableError)
{
    RecordProperty("Verifies", "1, 7, 6");
    RecordProperty(
        "Description",
        "Checks whether the right Trace call is done for local data (1). Also checks the transmission"
        "data loss flag (7) and handling of LocalDataChunkLists (6)");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    // given a UuT which delegates to a mock ITracingRuntimeBinding in case of BindingType::kLoLa

    TracingRuntimeAttorney attorney{*unit_under_test_.get()};
    auto previous_failure_counter = attorney.GetFailureCounter();

    void* local_data_ptr{reinterpret_cast<void*>(static_cast<intptr_t>(500))};
    std::size_t local_data_size{8};
    amp::optional<TracingRuntime::TracePointDataId> data_id{};

    SetupTracingRuntimeBindingMockForLocalDataTraceCall();

    // and then expect, that UuT calls the GenericTraceAPI::Trace() call with local chunk list
    EXPECT_CALL(*generic_trace_api_mock_.get(), Trace(trace_client_id_, _, _))
        .WillOnce(Return(MakeUnexpected(analysis::tracing::ErrorCode::kNotEnoughMemoryRecoverable)));
    // and expect, that it calls binding specific tracing runtime SetDataLoss(true) after a failed call to
    // GenericTraceAPI::Trace()
    EXPECT_CALL(tracing_runtime_binding_mock_, SetDataLossFlag(true)).Times(1);
    // when we call Trace on the UuT
    auto result = unit_under_test_->Trace(BindingType::kLoLa,
                                          dummy_service_element_instance_identifier_view_,
                                          SkeletonEventTracePointType::SEND,
                                          data_id,
                                          local_data_ptr,
                                          local_data_size);

    // expect, that there was no error (as non-recoverable error would just lead to a returned error in case of
    // threshold reached)
    EXPECT_TRUE(result.has_value());
    // expect, that tracing is still globally enabled
    EXPECT_TRUE(attorney.IsTracingEnabled());
    // expect, that failure counter is incremented by one
    EXPECT_EQ(attorney.GetFailureCounter(), previous_failure_counter + 1);
}

TEST_F(TracingRuntimeTraceLocalFixture, TraceLocalData_NonRecoverableError)
{
    RecordProperty("Verifies", "1, 7");
    RecordProperty(
        "Description",
        "Checks whether the right Trace call is done for local data (1). Also checks the transmission"
        "data loss flag (7)");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    // given a UuT which delegates to a mock ITracingRuntimeBinding in case of BindingType::kLoLa
    ASSERT_TRUE(unit_under_test_);
    TracingRuntimeAttorney attorney{*unit_under_test_.get()};
    auto previous_failure_counter = attorney.GetFailureCounter();

    void* local_data_ptr{reinterpret_cast<void*>(static_cast<intptr_t>(500))};
    std::size_t local_data_size{8};
    amp::optional<TracingRuntime::TracePointDataId> data_id{};

    analysis::tracing::LocalDataChunk root_chunk{local_data_ptr, local_data_size};
    analysis::tracing::LocalDataChunkList expected_chunk_list{root_chunk};

    analysis::tracing::ServiceInstanceElement::EventIdType event_id = 42U;
    analysis::tracing::ServiceInstanceElement::VariantType variant{event_id};
    analysis::tracing::ServiceInstanceElement service_instance_element{25, 1, 0, 1, variant};

    // expect, that UuT calls GetDataLossFlag() on the binding specific tracing runtime to setup meta-info property
    EXPECT_CALL(tracing_runtime_binding_mock_, GetDataLossFlag()).WillOnce(Return(false));
    // and expect, that it calls binding specific tracing runtime to convert binding specific service element instance
    // identification into a independent/tracing specific one
    EXPECT_CALL(tracing_runtime_binding_mock_,
                ConvertToTracingServiceInstanceElement(dummy_service_element_instance_identifier_view_))
        .WillOnce(Return(service_instance_element));
    // and expect, that it calls binding specific tracing runtime to get its client id for call to GenericTraceAPI
    EXPECT_CALL(tracing_runtime_binding_mock_, GetTraceClientId()).WillOnce(Return(trace_client_id_));
    // and then expect, that UuT calls the GenericTraceAPI::Trace() call with local chunk list
    EXPECT_CALL(*generic_trace_api_mock_.get(), Trace(trace_client_id_, _, Eq(ByRef(expected_chunk_list))))
        .WillOnce(Return(MakeUnexpected(analysis::tracing::ErrorCode::kChannelCreationFailedFatal)));
    // and expect, that it calls binding specific tracing runtime SetDataLoss(true) after a failed call to
    // GenericTraceAPI::Trace()
    EXPECT_CALL(tracing_runtime_binding_mock_, SetDataLossFlag(true)).Times(1);
    // when we call Trace on the UuT
    auto result = unit_under_test_->Trace(BindingType::kLoLa,
                                          dummy_service_element_instance_identifier_view_,
                                          SkeletonEventTracePointType::SEND,
                                          data_id,
                                          local_data_ptr,
                                          local_data_size);

    // expect, that there was an error
    EXPECT_FALSE(result.has_value());

    EXPECT_EQ(result.error(), TraceErrorCode::TraceErrorDisableTracePointInstance);
    // expect, that tracing is still globally enabled
    EXPECT_TRUE(attorney.IsTracingEnabled());
    // expect, that failure counter is incremented by one
    EXPECT_EQ(attorney.GetFailureCounter(), previous_failure_counter + 1);
}

TEST_F(TracingRuntimeTraceLocalFixture, DisabledTracing_EarlyReturns)
{
    // given a UuT which delegates to a mock ITracingRuntimeBinding in case of BindingType::kLoLa
    ASSERT_TRUE(unit_under_test_);
    // and which has tracing disabled
    TracingRuntimeAttorney attorney{*unit_under_test_.get()};
    attorney.SetTracingEnabled(false);

    // expect, that NO calls are done to the GenericTraceAPI
    EXPECT_CALL(*generic_trace_api_mock_.get(), Trace(_, _, _, _)).Times(0);
    EXPECT_CALL(*generic_trace_api_mock_.get(), Trace(_, _, _)).Times(0);
    EXPECT_CALL(*generic_trace_api_mock_.get(), RegisterShmObject(_, testing::Matcher<const std::string&>())).Times(0);
    EXPECT_CALL(*generic_trace_api_mock_.get(), RegisterShmObject(_, testing::Matcher<int>())).Times(0);
    EXPECT_CALL(*generic_trace_api_mock_.get(), UnregisterShmObject(_, _)).Times(0);

    // expect, that all calls to its public interface directly return (in case they have an error code return,
    // the code shall be TraceErrorDisableAllTracePoints)
    auto result1 = unit_under_test_->Trace(BindingType::kLoLa,
                                           dummy_service_element_instance_identifier_view_,
                                           SkeletonEventTracePointType::SEND,
                                           amp::optional<TracingRuntime::TracePointDataId>{},
                                           reinterpret_cast<void*>(static_cast<intptr_t>(500)),
                                           std::size_t{0});
    EXPECT_FALSE(result1.has_value());
    EXPECT_EQ(result1.error(), TraceErrorCode::TraceErrorDisableAllTracePoints);

    auto result2 = unit_under_test_->Trace(BindingType::kLoLa,
                                           trace_context_id_,
                                           dummy_service_element_instance_identifier_view_,
                                           SkeletonEventTracePointType::SEND,
                                           dummy_data_id_,
                                           CreateDummySamplePtr(),
                                           dummy_shm_data_ptr_,
                                           dummy_shm_data_size_);
    EXPECT_FALSE(result2.has_value());
    EXPECT_EQ(result2.error(), TraceErrorCode::TraceErrorDisableAllTracePoints);

    unit_under_test_->RegisterShmObject(BindingType::kLoLa,
                                        dummy_service_element_instance_identifier_view_,
                                        memory::shared::ISharedMemoryResource::FileDescriptor{1},
                                        reinterpret_cast<void*>(static_cast<uintptr_t>(777)));

    unit_under_test_->UnregisterShmObject(BindingType::kLoLa, dummy_service_element_instance_identifier_view_);
    unit_under_test_->SetDataLossFlag(BindingType::kLoLa);
}

TEST_F(TracingRuntimeTraceLocalFixture, TraceLocalData_FatalError)
{
    RecordProperty("Verifies", "4");
    RecordProperty("Description",
                   "Checks that after a terminal fatal error in Trace() call, tracing is "
                   "completely disabled and a log message mit severity warning is issued");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    // given a UuT which delegates to a mock ITracingRuntimeBinding in case of BindingType::kLoLa
    ASSERT_TRUE(unit_under_test_);
    TracingRuntimeAttorney attorney{*unit_under_test_.get()};

    void* local_data_ptr{reinterpret_cast<void*>(static_cast<intptr_t>(500))};
    std::size_t local_data_size{8};
    amp::optional<TracingRuntime::TracePointDataId> data_id{};

    analysis::tracing::LocalDataChunk root_chunk{local_data_ptr, local_data_size};
    analysis::tracing::LocalDataChunkList expected_chunk_list{root_chunk};

    analysis::tracing::ServiceInstanceElement::EventIdType event_id = 42U;
    analysis::tracing::ServiceInstanceElement::VariantType variant{event_id};
    analysis::tracing::ServiceInstanceElement service_instance_element{25, 1, 0, 1, variant};

    // expect, that UuT calls GetDataLossFlag() on the binding specific tracing runtime to setup meta-info property
    EXPECT_CALL(tracing_runtime_binding_mock_, GetDataLossFlag()).WillOnce(Return(false));
    // and expect, that it calls binding specific tracing runtime to convert binding specific service element instance
    // identification into a independent/tracing specific one
    EXPECT_CALL(tracing_runtime_binding_mock_,
                ConvertToTracingServiceInstanceElement(dummy_service_element_instance_identifier_view_))
        .WillOnce(Return(service_instance_element));
    // and expect, that it calls binding specific tracing runtime to get its client id for call to GenericTraceAPI
    EXPECT_CALL(tracing_runtime_binding_mock_, GetTraceClientId()).WillOnce(Return(trace_client_id_));
    // and then expect, that UuT calls the GenericTraceAPI::Trace() call with local chunk list (which returns fatal
    // error)
    EXPECT_CALL(*generic_trace_api_mock_.get(), Trace(trace_client_id_, _, Eq(ByRef(expected_chunk_list))))
        .WillOnce(Return(MakeUnexpected(analysis::tracing::ErrorCode::kTerminalFatal)));
    // when we call Trace on the UuT
    auto result = unit_under_test_->Trace(BindingType::kLoLa,
                                          dummy_service_element_instance_identifier_view_,
                                          SkeletonEventTracePointType::SEND,
                                          data_id,
                                          local_data_ptr,
                                          local_data_size);

    // expect, that there was an error
    EXPECT_FALSE(result.has_value());
    // and expect, that the error code is TraceErrorDisableAllTracePoints
    EXPECT_EQ(result.error(), TraceErrorCode::TraceErrorDisableAllTracePoints);
    // expect, that tracing is globally disabled
    EXPECT_FALSE(attorney.IsTracingEnabled());
}

class TracingRuntimeTraceDataLossFlagParameterisedFixture : public TracingRuntimeFixture,
                                                            public ::testing::WithParamInterface<bool>
{
};

TEST_P(TracingRuntimeTraceDataLossFlagParameterisedFixture, CallingShmTraceWillTransmitCurrentValueOfDataLossFlag)
{
    RecordProperty("Verifies", "7");
    RecordProperty("Description", "Checks the transmission data loss flag with Shm Trace call");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    const bool data_loss_flag = GetParam();

    // given a UuT which delegates to a mock ITracingRuntimeBinding in case of BindingType::kLoLa

    SetupTracingRuntimeBindingMockForShmDataTraceCall();

    EXPECT_CALL(tracing_runtime_binding_mock_, GetDataLossFlag()).WillOnce(Return(data_loss_flag));

    EXPECT_CALL(*generic_trace_api_mock_.get(), Trace(trace_client_id_, _, _, trace_context_id_))
        .WillOnce(WithArg<1>(Invoke([data_loss_flag](const auto& meta_info) -> analysis::tracing::TraceResult {
            const auto ara_com_meta_info = amp::get<analysis::tracing::AraComMetaInfo>(meta_info);
            const auto transmitted_data_loss_value_bitset = ara_com_meta_info.trace_status_;

            if (data_loss_flag)
            {
                EXPECT_TRUE(transmitted_data_loss_value_bitset.any());
            }
            else
            {
                EXPECT_FALSE(transmitted_data_loss_value_bitset.any());
            }
            return analysis::tracing::TraceResult{};
        })));

    // when we call Trace on the UuT
    auto result = unit_under_test_->Trace(BindingType::kLoLa,
                                          trace_context_id_,
                                          dummy_service_element_instance_identifier_view_,
                                          SkeletonEventTracePointType::SEND,
                                          dummy_data_id_,
                                          CreateDummySamplePtr(),
                                          dummy_shm_data_ptr_,
                                          dummy_shm_data_size_);

    EXPECT_TRUE(result.has_value());
}

TEST_P(TracingRuntimeTraceDataLossFlagParameterisedFixture, CallingLocalTraceWillTransmitCurrentValueOfDataLossFlag)
{
    RecordProperty("Verifies", "7");
    RecordProperty("Description", "Checks the transmission data loss flag with local Trace call");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    const bool data_loss_flag = GetParam();

    void* local_data_ptr{reinterpret_cast<void*>(static_cast<intptr_t>(500))};
    std::size_t local_data_size{8};
    amp::optional<TracingRuntime::TracePointDataId> data_id{};

    // given a UuT which delegates to a mock ITracingRuntimeBinding in case of BindingType::kLoLa

    SetupTracingRuntimeBindingMockForLocalDataTraceCall();

    EXPECT_CALL(tracing_runtime_binding_mock_, GetDataLossFlag()).WillOnce(Return(data_loss_flag));

    EXPECT_CALL(*generic_trace_api_mock_.get(), Trace(trace_client_id_, _, _))
        .WillOnce(WithArg<1>(Invoke([data_loss_flag](const auto& meta_info) -> analysis::tracing::TraceResult {
            const auto ara_com_meta_info = amp::get<analysis::tracing::AraComMetaInfo>(meta_info);
            const auto transmitted_data_loss_value_bitset = ara_com_meta_info.trace_status_;

            if (data_loss_flag)
            {
                EXPECT_TRUE(transmitted_data_loss_value_bitset.any());
            }
            else
            {
                EXPECT_FALSE(transmitted_data_loss_value_bitset.any());
            }
            return analysis::tracing::TraceResult{};
        })));

    // when we call Trace on the UuT
    auto result = unit_under_test_->Trace(BindingType::kLoLa,
                                          dummy_service_element_instance_identifier_view_,
                                          SkeletonEventTracePointType::SEND,
                                          data_id,
                                          local_data_ptr,
                                          local_data_size);

    // expect, that there was no error
    EXPECT_TRUE(result.has_value());
}

INSTANTIATE_TEST_SUITE_P(TracingRuntimeTraceDataLossFlagParameterisedFixture,
                         TracingRuntimeTraceDataLossFlagParameterisedFixture,
                         ::testing::Values(true, false));

}  // namespace
}  // namespace tracing
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
