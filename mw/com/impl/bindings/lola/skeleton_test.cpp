// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#include "platform/aas/mw/com/impl/bindings/lola/skeleton.h"
#include "platform/aas/mw/com/impl/bindings/lola/test/skeleton_test_resources.h"
#include "platform/aas/mw/com/impl/bindings/lola/test/transaction_log_test_resources.h"
#include "platform/aas/mw/com/impl/bindings/mock_binding/skeleton_event.h"
#include "platform/aas/mw/com/impl/tracing/tracing_runtime_mock.h"

#include <gtest/gtest.h>

#include <amp_variant.hpp>

#include <string>
#include <vector>

namespace bmw::mw::com::impl::lola
{
namespace
{

using ::testing::_;
using ::testing::InvokeWithoutArgs;
using ::testing::MockFunction;
using ::testing::Return;
using ::testing::StrEq;
using ::testing::StrictMock;

const std::string kServiceInstanceUsageFilePath{"/test_service_instance_usage_file_path"};
const std::int32_t kServiceInstanceUsageFileDescriptor{7890};

const os::Fcntl::Operation kNonBlockingExclusiveLockOperation =
    os::Fcntl::Operation::kLockExclusive | bmw::os::Fcntl::Operation::kLockNB;
const os::Fcntl::Operation kUnlockOperation = os::Fcntl::Operation::kUnLock;

/// \brief Test fixture which mocks the SharedMemoryFactory, allowing us to return a mocked SharedMemoryResource.
/// \details Tests which check the creation of the shared memory can use SharedMemoryResourceMocks. Tests which check
/// the allocated memory can use SharedMemoryResourceHeapAllocatorMock to allow checking the memory without using actual
/// shared memory.
class SkeletonTestMockedSharedMemoryFixture : public SkeletonMockedMemoryFixture
{
  protected:
    const LolaServiceTypeDeployment* GetLolaServiceTypeDeployment(const ServiceTypeDeployment& service_type_deployment)
    {
        const auto* const lola_service_type_deployment =
            amp::get_if<LolaServiceTypeDeployment>(&service_type_deployment.binding_info_);
        EXPECT_NE(lola_service_type_deployment, nullptr);
        return lola_service_type_deployment;
    }
};

TEST_F(SkeletonTestMockedSharedMemoryFixture, GetBindingType)
{
    // Given a deployment based on a default LolaServiceInstanceDeployment which has QM and ASIL B support
    const auto instance_identifier =
        make_InstanceIdentifier(test::kValidMinimalAsilInstanceDeployment, test::kValidMinimalTypeDeployment);

    // ... and a skeleton constructed from it
    InitialiseSkeleton(instance_identifier);

    // expect, that it returns BindingType::kLoLa, when asked about its binding type
    EXPECT_EQ(skeleton_->GetBindingType(), BindingType::kLoLa);
}

TEST_F(SkeletonTestMockedSharedMemoryFixture, StopOfferCallsUnregisterShmObjectTraceCallback)
{
    MockFunction<void(amp::string_view, impl::tracing::ServiceElementType)> unregister_shm_object_trace_callback{};
    amp::optional<SkeletonBinding::RegisterShmObjectTraceCallback> register_shm_object_trace_callback{};
    SkeletonBinding::SkeletonEventBindings events{};
    SkeletonBinding::SkeletonFieldBindings fields{};

    // Given a deployment based on an default LolaServiceInstanceDeployment which has QM and ASIL B support
    // ... and a skeleton constructed from it
    InitialiseSkeleton(GetValidASILInstanceIdentifier());

    // and that opening the service instance usage marker file succeeds
    ExpectServiceUsageMarkerFileCreatedOrOpenedAndClosed(kServiceInstanceUsageFilePath,
                                                         kServiceInstanceUsageFileDescriptor);

    // and that flocking the service instance usage marker file succeeds in PrepareOffer and in PrepareStopOffer
    EXPECT_CALL(*fcntl_mock_, flock(kServiceInstanceUsageFileDescriptor, kNonBlockingExclusiveLockOperation))
        .Times(2)
        .WillRepeatedly(Return(amp::blank{}));
    EXPECT_CALL(*fcntl_mock_, flock(kServiceInstanceUsageFileDescriptor, kUnlockOperation))
        .Times(2)
        .WillRepeatedly(Return(amp::blank{}));

    // When trying to create QM control and data segments succeed
    ExpectControlSegmentCreated(QualityType::kASIL_QM);
    ExpectControlSegmentCreated(QualityType::kASIL_B);
    ExpectDataSegmentCreated();

    // Then the shared memory will be cleaned up in PrepareStopOffer
    EXPECT_CALL(shared_memory_factory_mock_, Remove(test::kControlChannelPathQm));
    EXPECT_CALL(shared_memory_factory_mock_, Remove(test::kControlChannelPathAsilB));
    EXPECT_CALL(shared_memory_factory_mock_, Remove(test::kDataChannelPath));

    EXPECT_CALL(unregister_shm_object_trace_callback,
                Call(amp::string_view{tracing::TracingRuntime::kDummyElementNameForShmRegisterCallback},
                     tracing::TracingRuntime::kDummyElementTypeForShmRegisterCallback));

    // Then PrepareOffer will succeed
    EXPECT_TRUE(skeleton_->PrepareOffer(events, fields, std::move(register_shm_object_trace_callback)).has_value());

    // When a service is stopped with the optional UnregisterShmObjectTraceCallback set
    skeleton_->PrepareStopOffer(unregister_shm_object_trace_callback.AsStdFunction());
}

using SkeletonTestSharedMemoryCreationFixture = SkeletonTestMockedSharedMemoryFixture;
TEST_F(SkeletonTestSharedMemoryCreationFixture, PrepareServiceOfferFailsOnShmCreateFailureForQmControl)
{
    using namespace memory::shared;

    // Given a Skeleton constructed from a valid identifier referencing a QM deployment
    InitialiseSkeleton(GetValidInstanceIdentifier());

    // Expect that the usage marker file path is created and closed
    ExpectServiceUsageMarkerFileCreatedOrOpenedAndClosed();

    // But when trying to create a control qm segment fails by returning a nullptr
    EXPECT_CALL(shared_memory_factory_mock_,
                Create(test::kControlChannelPathQm, _, _, WritablePermissionsMatcher(), false))
        .WillOnce(Return(nullptr));

    // Then PrepareOffer will fail
    SkeletonBinding::SkeletonEventBindings events{};
    SkeletonBinding::SkeletonFieldBindings fields{};
    amp::optional<SkeletonBinding::RegisterShmObjectTraceCallback> register_shm_object_trace_callback{};
    EXPECT_FALSE(skeleton_->PrepareOffer(events, fields, std::move(register_shm_object_trace_callback)).has_value());
}

TEST_F(SkeletonTestSharedMemoryCreationFixture, PrepareServiceOfferFailsOnShmCreateFailureForAsilBControl)
{
    using namespace memory::shared;

    // Given a Skeleton constructed from a valid identifier referencing an ASIL B deployment
    InitialiseSkeleton(GetValidASILInstanceIdentifier());

    // Expect that the usage marker file path is created and closed
    ExpectServiceUsageMarkerFileCreatedOrOpenedAndClosed();

    // When trying to create a QM control segment succeeds
    ExpectControlSegmentCreated(QualityType::kASIL_QM);

    // But when trying to create an ASIL B control segment fails by returning a nullptr
    EXPECT_CALL(shared_memory_factory_mock_,
                Create(test::kControlChannelPathAsilB, _, _, WritablePermissionsMatcher(), false))
        .WillOnce(Return(nullptr));

    // Then PrepareOffer will fail
    SkeletonBinding::SkeletonEventBindings events{};
    SkeletonBinding::SkeletonFieldBindings fields{};
    amp::optional<SkeletonBinding::RegisterShmObjectTraceCallback> register_shm_object_trace_callback{};
    EXPECT_FALSE(skeleton_->PrepareOffer(events, fields, std::move(register_shm_object_trace_callback)).has_value());
}

TEST_F(SkeletonTestSharedMemoryCreationFixture, PrepareServiceOfferFailsOnShmCreateFailureForData)
{
    using namespace memory::shared;
    // Given a Skeleton constructed from a valid identifier referencing a QM deployment
    InitialiseSkeleton(GetValidInstanceIdentifier());

    // Expect that the usage marker file path is created and closed
    ExpectServiceUsageMarkerFileCreatedOrOpenedAndClosed();

    // When trying to create a QM control segment succeeds
    ExpectControlSegmentCreated(QualityType::kASIL_QM);

    // But when trying to create a data segment fails by returning a nullptr
    EXPECT_CALL(shared_memory_factory_mock_, Create(test::kDataChannelPath, _, _, ReadablePermissionsMatcher(), false))
        .WillOnce(Return(nullptr));

    // Then PrepareOffer will fail
    SkeletonBinding::SkeletonEventBindings events{};
    SkeletonBinding::SkeletonFieldBindings fields{};
    amp::optional<SkeletonBinding::RegisterShmObjectTraceCallback> register_shm_object_trace_callback{};
    EXPECT_FALSE(skeleton_->PrepareOffer(events, fields, std::move(register_shm_object_trace_callback)).has_value());
}

TEST_F(SkeletonTestSharedMemoryCreationFixture, PrepareServiceOfferWithTraceCallback)
{
    RecordProperty("Verifies", "1, 4");
    RecordProperty("Description",
                   "Checks whether, typed-memory is preferred (1) for creation of shm-object for DATA in "
                   "case it is tracing relevant. Checks, that registration is only tried, when shm-object has been "
                   "successfully created in typed-mem (4).");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    using namespace memory::shared;
    auto memory_resource_mock = std::static_pointer_cast<::testing::StrictMock<SharedMemoryResourceHeapAllocatorMock>>(
        data_shared_memory_resource_mock_);
    void* const shm_object_base_address = reinterpret_cast<void*>(static_cast<uintptr_t>(1000));
    const ISharedMemoryResource::FileDescriptor shm_object_file_descriptor{55};

    MockFunction<void(
        amp::string_view, impl::tracing::ServiceElementType, ISharedMemoryResource::FileDescriptor, void*)>
        register_shm_object_trace_callback{};

    // Given a Skeleton constructed from a valid identifier referencing a QM deployment
    InitialiseSkeleton(GetValidInstanceIdentifier());

    // Expect that the usage marker file path is created and closed
    ExpectServiceUsageMarkerFileCreatedOrOpenedAndClosed();

    // When trying to create a QM control segment succeeds
    ExpectControlSegmentCreated(QualityType::kASIL_QM);

    // and trying to create a data segment in typed-mem succeeds
    ExpectDataSegmentCreated(true);

    EXPECT_CALL(*memory_resource_mock, IsShmInTypedMemory()).WillOnce(::testing::Return(true));

    EXPECT_CALL(*memory_resource_mock, GetFileDescriptor()).WillOnce(::testing::Return(shm_object_file_descriptor));
    EXPECT_CALL(*memory_resource_mock, getBaseAddress()).WillOnce(::testing::Return(shm_object_base_address));

    // and that the callback will be called once
    EXPECT_CALL(register_shm_object_trace_callback,
                Call(amp::string_view{tracing::TracingRuntime::kDummyElementNameForShmRegisterCallback},
                     tracing::TracingRuntime::kDummyElementTypeForShmRegisterCallback,
                     shm_object_file_descriptor,
                     shm_object_base_address));

    // Then PrepareOffer will succeed
    SkeletonBinding::SkeletonEventBindings events{};
    SkeletonBinding::SkeletonFieldBindings fields{};
    EXPECT_TRUE(
        skeleton_->PrepareOffer(events, fields, register_shm_object_trace_callback.AsStdFunction()).has_value());
}

TEST_F(SkeletonTestSharedMemoryCreationFixture, PrepareServiceOfferWithTraceCallbackNeverCalledIfNotInTypedMemory)
{
    RecordProperty("Verifies", "1, 4");
    RecordProperty("Description",
                   "Checks whether, typed-memory is preferred (1) for creation of shm-object for DATA in "
                   "case it is tracing relevant. Checks, that registration is only tried, when shm-object has been "
                   "successfully created in typed-mem (4).");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    using namespace memory::shared;
    auto memory_resource_mock = std::static_pointer_cast<::testing::StrictMock<SharedMemoryResourceHeapAllocatorMock>>(
        data_shared_memory_resource_mock_);

    MockFunction<void(
        amp::string_view, impl::tracing::ServiceElementType, ISharedMemoryResource::FileDescriptor, void*)>
        register_shm_object_trace_callback{};

    // Given a Skeleton constructed from a valid identifier referencing a QM deployment
    InitialiseSkeleton(GetValidInstanceIdentifier());

    // Expect that the usage marker file path is created and closed
    ExpectServiceUsageMarkerFileCreatedOrOpenedAndClosed();

    // When trying to create a QM control segment succeeds
    ExpectControlSegmentCreated(QualityType::kASIL_QM);

    // and trying to create a data segment in typed-mem succeeds
    ExpectDataSegmentCreated(true);

    // and that the shared memory resource cannot be created in typed memory
    EXPECT_CALL(*memory_resource_mock, IsShmInTypedMemory()).WillOnce(::testing::Return(false));

    EXPECT_CALL(*memory_resource_mock, GetFileDescriptor()).Times(0);
    EXPECT_CALL(*memory_resource_mock, getBaseAddress()).Times(0);

    // and that the callback will never be called
    EXPECT_CALL(register_shm_object_trace_callback, Call(_, _, _, _)).Times(0);

    // Then PrepareOffer will succeed
    SkeletonBinding::SkeletonEventBindings events{};
    SkeletonBinding::SkeletonFieldBindings fields{};
    EXPECT_TRUE(
        skeleton_->PrepareOffer(events, fields, register_shm_object_trace_callback.AsStdFunction()).has_value());
}

using SkeletonPrepareOfferFixture = SkeletonTestMockedSharedMemoryFixture;
TEST_F(SkeletonPrepareOfferFixture, PrepareOfferCreatesSharedMemoryIfOpeningAndFLockingServiceUsageMarkerFileSucceeds)
{
    SkeletonBinding::SkeletonEventBindings events{};
    SkeletonBinding::SkeletonFieldBindings fields{};
    amp::optional<SkeletonBinding::RegisterShmObjectTraceCallback> register_shm_object_trace_callback{};

    // Given a Skeleton constructed from a valid identifier referencing a QM deployment
    InitialiseSkeleton(GetValidInstanceIdentifier());

    // and that opening the service instance usage marker file succeeds
    ExpectServiceUsageMarkerFileCreatedOrOpenedAndClosed(kServiceInstanceUsageFilePath,
                                                         kServiceInstanceUsageFileDescriptor);

    // and that flocking the service instance usage marker file succeeds
    ExpectServiceUsageMarkerFileFlockAcquired(kServiceInstanceUsageFileDescriptor);

    // When trying to create QM control and data segments succeed
    ExpectControlSegmentCreated(QualityType::kASIL_QM);
    ExpectDataSegmentCreated();

    // Then PrepareOffer will succeed
    EXPECT_TRUE(skeleton_->PrepareOffer(events, fields, std::move(register_shm_object_trace_callback)).has_value());
}

TEST_F(SkeletonPrepareOfferFixture,
       PrepareOfferRemovesOldSharedMemoryArtefactsIfOpeningAndFLockingServiceUsageMarkerFileSucceeds)
{
    SkeletonBinding::SkeletonEventBindings events{};
    SkeletonBinding::SkeletonFieldBindings fields{};
    amp::optional<SkeletonBinding::RegisterShmObjectTraceCallback> register_shm_object_trace_callback{};

    // Given a Skeleton constructed from a valid identifier referencing a QM deployment
    InitialiseSkeleton(GetValidInstanceIdentifier());

    // and that opening the service instance usage marker file succeeds
    ExpectServiceUsageMarkerFileCreatedOrOpenedAndClosed(kServiceInstanceUsageFilePath,
                                                         kServiceInstanceUsageFileDescriptor);

    // and that flocking the service instance usage marker file succeeds
    ExpectServiceUsageMarkerFileFlockAcquired(kServiceInstanceUsageFileDescriptor);

    EXPECT_CALL(shared_memory_factory_mock_, RemoveStaleArtefacts(test::kControlChannelPathQm));
    EXPECT_CALL(shared_memory_factory_mock_, RemoveStaleArtefacts(test::kControlChannelPathAsilB));
    EXPECT_CALL(shared_memory_factory_mock_, RemoveStaleArtefacts(test::kDataChannelPath));

    // When trying to create QM control and data segments succeed
    ExpectControlSegmentCreated(QualityType::kASIL_QM);
    ExpectDataSegmentCreated();

    // Then PrepareOffer will succeed
    EXPECT_TRUE(skeleton_->PrepareOffer(events, fields, std::move(register_shm_object_trace_callback)).has_value());
}

TEST_F(SkeletonPrepareOfferFixture, PrepareOfferFailsIfOpeningServiceUsageMarkerFileFails)
{
    SkeletonBinding::SkeletonEventBindings events{};
    SkeletonBinding::SkeletonFieldBindings fields{};
    amp::optional<SkeletonBinding::RegisterShmObjectTraceCallback> register_shm_object_trace_callback{};

    // Given a Skeleton constructed from a valid identifier referencing a QM deployment
    InitialiseSkeleton(GetValidInstanceIdentifier());

    // and that opening the service instance usage marker file fails
    EXPECT_CALL(*partial_restart_path_builder_mock_, GetServiceInstanceUsageMarkerFilePath(_))
        .WillOnce(Return(kServiceInstanceUsageFilePath));
    EXPECT_CALL(*fcntl_mock_, open(StrEq(kServiceInstanceUsageFilePath.data()), _, _))
        .WillOnce(Return(amp::make_unexpected(os::Error::createFromErrno(EPERM))));

    // Then PrepareOffer will fail
    EXPECT_FALSE(skeleton_->PrepareOffer(events, fields, std::move(register_shm_object_trace_callback)).has_value());
}

TEST_F(SkeletonPrepareOfferFixture, PrepareOfferOpensAndCleansExistingSharedMemoryIfFLockingServiceUsageMarkerFilefails)
{
    const ElementFqId element_fq_id{1U, 2U, 3U, ElementType::EVENT};

    ServiceDataControl service_data_control_qm{CreateServiceDataControlWithEvent(element_fq_id, QualityType::kASIL_QM)};
    ServiceDataControl service_data_control_asil_b{
        CreateServiceDataControlWithEvent(element_fq_id, QualityType::kASIL_B)};

    auto& event_control_qm = GetEventControlFromServiceDataControl(element_fq_id, service_data_control_qm);
    auto& event_control_asil_b = GetEventControlFromServiceDataControl(element_fq_id, service_data_control_asil_b);

    SkeletonBinding::SkeletonEventBindings events{};
    SkeletonBinding::SkeletonFieldBindings fields{};
    amp::optional<SkeletonBinding::RegisterShmObjectTraceCallback> register_shm_object_trace_callback{};

    // Given a Skeleton constructed from a valid identifier referencing a QM deployment
    InitialiseSkeleton(GetValidASILInstanceIdentifier());

    // and that opening the service instance usage marker file succeeds
    ExpectServiceUsageMarkerFileCreatedOrOpenedAndClosed(kServiceInstanceUsageFilePath,
                                                         kServiceInstanceUsageFileDescriptor);

    // and that flocking the service instance usage marker file fails
    ExpectServiceUsageMarkerFileAlreadyFlocked(kServiceInstanceUsageFileDescriptor);

    // When trying to open QM and ASIL B control segments which had previously allocated slots that were in writing
    auto first_allocation_qm = event_control_qm.data_control.AllocateNextSlot();
    ASSERT_TRUE(first_allocation_qm.has_value());

    auto first_allocation_asil_b = event_control_asil_b.data_control.AllocateNextSlot();
    ASSERT_TRUE(first_allocation_asil_b.has_value());

    ExpectControlSegmentOpened(QualityType::kASIL_QM, service_data_control_qm);
    ExpectControlSegmentOpened(QualityType::kASIL_B, service_data_control_asil_b);

    // and when opening the data segment
    ServiceDataStorage service_data_storage{data_shared_memory_resource_mock_->getMemoryResourceProxy()};
    ExpectDataSegmentOpened(service_data_storage);

    // Then PrepareOffer will succeed and clean up the service data controls
    EXPECT_TRUE(skeleton_->PrepareOffer(events, fields, std::move(register_shm_object_trace_callback)).has_value());

    // And a new allocation will return the same slot as before, as it was cleaned up
    auto second_allocation_qm = event_control_qm.data_control.AllocateNextSlot();
    ASSERT_TRUE(second_allocation_qm.has_value());
    EXPECT_EQ(first_allocation_qm.value(), second_allocation_qm.value());

    auto second_allocation_asil_b = event_control_asil_b.data_control.AllocateNextSlot();
    ASSERT_TRUE(second_allocation_asil_b.has_value());
    EXPECT_EQ(first_allocation_asil_b.value(), second_allocation_asil_b.value());
}

TEST_F(SkeletonPrepareOfferFixture, PrepareOfferFailsIfOpeningExistingSharedMemoryDataFails)
{
    SkeletonBinding::SkeletonEventBindings events{};
    SkeletonBinding::SkeletonFieldBindings fields{};
    amp::optional<SkeletonBinding::RegisterShmObjectTraceCallback> register_shm_object_trace_callback{};

    // Given a Skeleton constructed from a valid identifier referencing a QM deployment
    InitialiseSkeleton(GetValidInstanceIdentifier());

    // and that opening the service instance usage marker file succeeds
    ExpectServiceUsageMarkerFileCreatedOrOpenedAndClosed(kServiceInstanceUsageFilePath,
                                                         kServiceInstanceUsageFileDescriptor);

    // and that flocking the service instance usage marker file fails
    ExpectServiceUsageMarkerFileAlreadyFlocked(kServiceInstanceUsageFileDescriptor);

    // When trying to open QM control segment succeeds
    ServiceDataControl service_data_control_qm{control_qm_shared_memory_resource_mock_->getMemoryResourceProxy()};
    ExpectControlSegmentOpened(QualityType::kASIL_QM, service_data_control_qm);

    // and the path builder returns a valid path for the data shared memory
    EXPECT_CALL(*shm_path_builder_mock_, GetDataChannelShmName(test::kDefaultLolaInstanceId))
        .WillOnce(Return("dummy_data_path"));

    // But when trying to open the data segment fails by returning a nullptr
    EXPECT_CALL(shared_memory_factory_mock_, Open("dummy_data_path", true, _)).WillOnce(Return(nullptr));

    // Then PrepareOffer will fail
    EXPECT_FALSE(skeleton_->PrepareOffer(events, fields, std::move(register_shm_object_trace_callback)).has_value());
}

TEST_F(SkeletonPrepareOfferFixture, PrepareOfferFailsIfOpeningExistingSharedMemoryControlQmFails)
{
    SkeletonBinding::SkeletonEventBindings events{};
    SkeletonBinding::SkeletonFieldBindings fields{};
    amp::optional<SkeletonBinding::RegisterShmObjectTraceCallback> register_shm_object_trace_callback{};

    // Given a Skeleton constructed from a valid identifier referencing a QM deployment
    InitialiseSkeleton(GetValidInstanceIdentifier());

    // and that opening the service instance usage marker file succeeds
    ExpectServiceUsageMarkerFileCreatedOrOpenedAndClosed(kServiceInstanceUsageFilePath,
                                                         kServiceInstanceUsageFileDescriptor);

    // and that flocking the service instance usage marker file fails
    ExpectServiceUsageMarkerFileAlreadyFlocked(kServiceInstanceUsageFileDescriptor);

    // and the path builder returns a valid path for the control qm shared memory
    EXPECT_CALL(*shm_path_builder_mock_, GetControlChannelShmName(test::kDefaultLolaInstanceId, QualityType::kASIL_QM))
        .WillOnce(Return("dummy_control_path_qm"));

    // But when trying to create a control qm segment fails by returning a nullptr
    EXPECT_CALL(shared_memory_factory_mock_, Open("dummy_control_path_qm", true, _)).WillOnce(Return(nullptr));

    // Then PrepareOffer will fail
    EXPECT_FALSE(skeleton_->PrepareOffer(events, fields, std::move(register_shm_object_trace_callback)).has_value());
}

TEST_F(SkeletonPrepareOfferFixture, PrepareOfferFailsIfOpeningExistingSharedMemoryControlAsilBFails)
{
    SkeletonBinding::SkeletonEventBindings events{};
    SkeletonBinding::SkeletonFieldBindings fields{};
    amp::optional<SkeletonBinding::RegisterShmObjectTraceCallback> register_shm_object_trace_callback{};

    // Given a Skeleton constructed from a valid identifier referencing a QM deployment
    InitialiseSkeleton(GetValidASILInstanceIdentifier());

    // and that opening the service instance usage marker file succeeds
    ExpectServiceUsageMarkerFileCreatedOrOpenedAndClosed(kServiceInstanceUsageFilePath,
                                                         kServiceInstanceUsageFileDescriptor);

    // and that flocking the service instance usage marker file fails
    ExpectServiceUsageMarkerFileAlreadyFlocked(kServiceInstanceUsageFileDescriptor);

    // When trying to open QM control segment succeeds
    ServiceDataControl service_data_control_qm{control_qm_shared_memory_resource_mock_->getMemoryResourceProxy()};
    ExpectControlSegmentOpened(QualityType::kASIL_QM, service_data_control_qm);

    // But when trying to create a control asil b segment fails by returning a nullptr
    EXPECT_CALL(shared_memory_factory_mock_, Open(test::kControlChannelPathAsilB, true, _)).WillOnce(Return(nullptr));

    // Then PrepareOffer will fail
    EXPECT_FALSE(skeleton_->PrepareOffer(events, fields, std::move(register_shm_object_trace_callback)).has_value());
}

TEST_F(SkeletonPrepareOfferFixture, PrepareOfferWillUpdateThePidInTheDataSegmentWhenOpeningSharedMemory)
{
    const ElementFqId element_fq_id{1U, 2U, 3U, ElementType::EVENT};
    const pid_t pid{7654};
    ServiceDataControl service_data_control_qm{CreateServiceDataControlWithEvent(element_fq_id, QualityType::kASIL_QM)};

    SkeletonBinding::SkeletonEventBindings events{};
    SkeletonBinding::SkeletonFieldBindings fields{};
    amp::optional<SkeletonBinding::RegisterShmObjectTraceCallback> register_shm_object_trace_callback{};

    // Given a Skeleton constructed from a valid identifier referencing a QM deployment
    InitialiseSkeleton(GetValidInstanceIdentifier());

    // and that opening the service instance usage marker file succeeds
    ExpectServiceUsageMarkerFileCreatedOrOpenedAndClosed(kServiceInstanceUsageFilePath,
                                                         kServiceInstanceUsageFileDescriptor);

    // and that flocking the service instance usage marker file fails
    ExpectServiceUsageMarkerFileAlreadyFlocked(kServiceInstanceUsageFileDescriptor);

    // and that the PID will be retrieved from the lola runtime
    EXPECT_CALL(lola_runtime_mock_, GetPid()).WillOnce(Return(pid));

    // When trying to open QM control segment succeeds
    ExpectControlSegmentOpened(QualityType::kASIL_QM, service_data_control_qm);

    // and when opening the data segment succeeds
    ServiceDataStorage service_data_storage{data_shared_memory_resource_mock_->getMemoryResourceProxy()};
    ExpectDataSegmentOpened(service_data_storage);

    // Then PrepareOffer will succeed and clean up the service data controls
    EXPECT_TRUE(skeleton_->PrepareOffer(events, fields, std::move(register_shm_object_trace_callback)).has_value());

    // and the ServiceDataStorage contains the PID returned by the lola runtime
    EXPECT_EQ(service_data_storage.skeleton_pid_, pid);
}

using SkeletonPrepareStopOfferFixture = SkeletonTestMockedSharedMemoryFixture;
TEST_F(SkeletonPrepareStopOfferFixture, PrepareStopOfferRemovesSharedMemoryIfUsageMarkerFileCanBeLocked)
{
    SkeletonBinding::SkeletonEventBindings events{};
    SkeletonBinding::SkeletonFieldBindings fields{};
    amp::optional<SkeletonBinding::RegisterShmObjectTraceCallback> register_shm_object_trace_callback{};

    // Given a Skeleton constructed from a valid identifier referencing a QM deployment
    InitialiseSkeleton(GetValidInstanceIdentifier());

    // and that opening the service instance usage marker file succeeds
    ExpectServiceUsageMarkerFileCreatedOrOpenedAndClosed(kServiceInstanceUsageFilePath,
                                                         kServiceInstanceUsageFileDescriptor);

    // and that flocking the service instance usage marker file succeeds in PrepareOffer and in PrepareStopOffer
    EXPECT_CALL(*fcntl_mock_, flock(kServiceInstanceUsageFileDescriptor, kNonBlockingExclusiveLockOperation))
        .Times(2)
        .WillRepeatedly(Return(amp::blank{}));
    EXPECT_CALL(*fcntl_mock_, flock(kServiceInstanceUsageFileDescriptor, kUnlockOperation))
        .Times(2)
        .WillRepeatedly(Return(amp::blank{}));

    // When trying to create QM control and data segments succeed
    ExpectControlSegmentCreated(QualityType::kASIL_QM);
    ExpectDataSegmentCreated();

    // Then the shared memory will be cleaned up in PrepareStopOffer
    EXPECT_CALL(shared_memory_factory_mock_, Remove(test::kControlChannelPathQm));
    EXPECT_CALL(shared_memory_factory_mock_, Remove(test::kDataChannelPath));

    // When PrepareOffer succeeds
    EXPECT_TRUE(skeleton_->PrepareOffer(events, fields, std::move(register_shm_object_trace_callback)).has_value());

    const auto before_shared_memory_control_qm_usage_counter = control_qm_shared_memory_resource_mock_.use_count();
    const auto before_shared_memory_data_usage_counter = data_shared_memory_resource_mock_.use_count();

    // and PrepareStopOffer is called
    skeleton_->PrepareStopOffer({});

    const auto after_shared_memory_control_qm_usage_counter = control_qm_shared_memory_resource_mock_.use_count();
    const auto after_shared_memory_data_usage_counter = data_shared_memory_resource_mock_.use_count();

    // Then the shared memory shared pointers in the skeleton will be destroyed
    EXPECT_EQ(after_shared_memory_control_qm_usage_counter, before_shared_memory_control_qm_usage_counter - 1);
    EXPECT_EQ(after_shared_memory_data_usage_counter, before_shared_memory_data_usage_counter - 1);
}

TEST_F(SkeletonPrepareStopOfferFixture, PrepareStopOfferRemovesUsageMarkerFileIfUsageMarkerFileCanBeLocked)
{
    SkeletonBinding::SkeletonEventBindings events{};
    SkeletonBinding::SkeletonFieldBindings fields{};
    amp::optional<SkeletonBinding::RegisterShmObjectTraceCallback> register_shm_object_trace_callback{};

    bool was_usage_marker_file_closed{false};

    // Given a Skeleton constructed from a valid identifier referencing a QM deployment
    InitialiseSkeleton(GetValidInstanceIdentifier());

    // and that opening the service instance usage marker file succeeds
    EXPECT_CALL(*partial_restart_path_builder_mock_, GetServiceInstanceUsageMarkerFilePath(_))
        .WillOnce(Return(kServiceInstanceUsageFilePath));
    EXPECT_CALL(*fcntl_mock_, open(StrEq(kServiceInstanceUsageFilePath.data()), _, _))
        .WillOnce(Return(kServiceInstanceUsageFileDescriptor));

    // and that flocking the service instance usage marker file succeeds in PrepareOffer and in PrepareStopOffer
    EXPECT_CALL(*fcntl_mock_, flock(kServiceInstanceUsageFileDescriptor, kNonBlockingExclusiveLockOperation))
        .Times(2)
        .WillRepeatedly(Return(amp::blank{}));
    EXPECT_CALL(*fcntl_mock_, flock(kServiceInstanceUsageFileDescriptor, kUnlockOperation))
        .Times(2)
        .WillRepeatedly(Return(amp::blank{}));

    // When trying to create QM control and data segments succeed
    ExpectControlSegmentCreated(QualityType::kASIL_QM);
    ExpectDataSegmentCreated();

    // Then the shared memory will be cleaned up in PrepareStopOffer
    EXPECT_CALL(shared_memory_factory_mock_, Remove(test::kControlChannelPathQm));
    EXPECT_CALL(shared_memory_factory_mock_, Remove(test::kDataChannelPath));

    // and the service usage marker file will be closed in PrepareStopOffer
    EXPECT_CALL(*unistd_mock_, close(kServiceInstanceUsageFileDescriptor))
        .WillOnce(InvokeWithoutArgs(
            [&was_usage_marker_file_closed]() mutable noexcept -> amp::expected_blank<bmw::os::Error> {
                was_usage_marker_file_closed = true;
                return {};
            }));

    // When PrepareOffer succeeds
    EXPECT_TRUE(skeleton_->PrepareOffer(events, fields, std::move(register_shm_object_trace_callback)).has_value());

    // and PrepareStopOffer is called
    EXPECT_FALSE(was_usage_marker_file_closed);
    skeleton_->PrepareStopOffer({});

    // Then the service usage marker file will be closed in PrepareStopOffer
    EXPECT_TRUE(was_usage_marker_file_closed);
}

TEST_F(SkeletonPrepareStopOfferFixture, PrepareStopOfferDoesNotRemoveSharedMemoryIfUsageMarkerFileCannotBeLocked)
{
    SkeletonBinding::SkeletonEventBindings events{};
    SkeletonBinding::SkeletonFieldBindings fields{};
    amp::optional<SkeletonBinding::RegisterShmObjectTraceCallback> register_shm_object_trace_callback{};

    // Given a Skeleton constructed from a valid identifier referencing a QM deployment
    InitialiseSkeleton(GetValidInstanceIdentifier());

    // and that opening the service instance usage marker file succeeds
    ExpectServiceUsageMarkerFileCreatedOrOpenedAndClosed(kServiceInstanceUsageFilePath,
                                                         kServiceInstanceUsageFileDescriptor);

    // and that flocking the service instance usage marker file fails in PrepareStopOffer
    EXPECT_CALL(*fcntl_mock_, flock(kServiceInstanceUsageFileDescriptor, kNonBlockingExclusiveLockOperation))
        .WillRepeatedly(Return(amp::make_unexpected(os::Error::createFromErrno(EWOULDBLOCK))));

    // but succeeds in PrepareOffer
    EXPECT_CALL(*fcntl_mock_, flock(kServiceInstanceUsageFileDescriptor, kNonBlockingExclusiveLockOperation))
        .WillOnce(Return(amp::blank{}))
        .RetiresOnSaturation();
    EXPECT_CALL(*fcntl_mock_, flock(kServiceInstanceUsageFileDescriptor, kUnlockOperation))
        .WillOnce(Return(amp::blank{}))
        .RetiresOnSaturation();

    // When trying to create QM control and data segments succeed
    ExpectControlSegmentCreated(QualityType::kASIL_QM);
    ExpectDataSegmentCreated();

    // Then the shared memory will be not be cleaned up in PrepareStopOffer
    EXPECT_CALL(shared_memory_factory_mock_, Remove(test::kControlChannelPathQm)).Times(0);
    EXPECT_CALL(shared_memory_factory_mock_, Remove(test::kDataChannelPath)).Times(0);

    // When PrepareOffer succeeds
    EXPECT_TRUE(skeleton_->PrepareOffer(events, fields, std::move(register_shm_object_trace_callback)).has_value());

    const auto before_shared_memory_control_qm_usage_counter = control_qm_shared_memory_resource_mock_.use_count();
    const auto before_shared_memory_data_usage_counter = data_shared_memory_resource_mock_.use_count();

    // and PrepareStopOffer is called
    skeleton_->PrepareStopOffer({});

    const auto after_shared_memory_control_qm_usage_counter = control_qm_shared_memory_resource_mock_.use_count();
    const auto after_shared_memory_data_usage_counter = data_shared_memory_resource_mock_.use_count();

    // Then the shared memory shared pointers in the skeleton will not be destroyed in PrepareStopOffer
    EXPECT_EQ(after_shared_memory_control_qm_usage_counter, before_shared_memory_control_qm_usage_counter);
    EXPECT_EQ(after_shared_memory_data_usage_counter, before_shared_memory_data_usage_counter);
}

TEST_F(SkeletonPrepareStopOfferFixture, PrepareStopOfferDoesNotRemoveUsageMarkerFileIfUsageMarkerFileCannotBeLocked)
{
    // Since close and unlink will be called during destruction of the skeleton, which is done in the fixture
    // destruction after these flags will be destroyed, we pass them as weak pointers to the lambdas to be invoked when
    // close / unlink are called.
    auto was_usage_marker_file_closed{std::make_shared<bool>(false)};

    SkeletonBinding::SkeletonEventBindings events{};
    SkeletonBinding::SkeletonFieldBindings fields{};
    amp::optional<SkeletonBinding::RegisterShmObjectTraceCallback> register_shm_object_trace_callback{};

    // Given a Skeleton constructed from a valid identifier referencing a QM deployment
    InitialiseSkeleton(GetValidInstanceIdentifier());

    // and that opening the service instance usage marker file succeeds
    EXPECT_CALL(*partial_restart_path_builder_mock_, GetServiceInstanceUsageMarkerFilePath(_))
        .WillOnce(Return(kServiceInstanceUsageFilePath));
    EXPECT_CALL(*fcntl_mock_, open(StrEq(kServiceInstanceUsageFilePath.data()), _, _))
        .WillOnce(Return(kServiceInstanceUsageFileDescriptor));

    // and that flocking the service instance usage marker file fails in PrepareStopOffer
    EXPECT_CALL(*fcntl_mock_, flock(kServiceInstanceUsageFileDescriptor, kNonBlockingExclusiveLockOperation))
        .WillRepeatedly(Return(amp::make_unexpected(os::Error::createFromErrno(EWOULDBLOCK))));

    // but succeeds in PrepareOffer
    EXPECT_CALL(*fcntl_mock_, flock(kServiceInstanceUsageFileDescriptor, kNonBlockingExclusiveLockOperation))
        .WillOnce(Return(amp::blank{}))
        .RetiresOnSaturation();
    EXPECT_CALL(*fcntl_mock_, flock(kServiceInstanceUsageFileDescriptor, kUnlockOperation))
        .WillOnce(Return(amp::blank{}))
        .RetiresOnSaturation();

    // When trying to create QM control and data segments succeed
    ExpectControlSegmentCreated(QualityType::kASIL_QM);
    ExpectDataSegmentCreated();

    // and the service usage marker file will be closed and unlinked when the Skeleton is destructed
    std::weak_ptr<bool> was_usage_marker_file_closed_weak_ptr{was_usage_marker_file_closed};

    // and the service usage marker file will be closed when the Skeleton is destructed
    EXPECT_CALL(*unistd_mock_, close(kServiceInstanceUsageFileDescriptor))
        .WillOnce(InvokeWithoutArgs(
            [was_usage_marker_file_closed_weak_ptr]() mutable noexcept -> amp::expected_blank<bmw::os::Error> {
                if (std::shared_ptr<bool> was_usage_marker_file_closed_shared_ptr =
                        was_usage_marker_file_closed_weak_ptr.lock())
                {
                    *was_usage_marker_file_closed_shared_ptr = true;
                }
                return {};
            }));

    // When PrepareOffer succeeds
    EXPECT_TRUE(skeleton_->PrepareOffer(events, fields, std::move(register_shm_object_trace_callback)).has_value());

    // and PrepareStopOffer is called
    EXPECT_FALSE(*was_usage_marker_file_closed);
    skeleton_->PrepareStopOffer({});

    // Then the service usage marker file will not be closed in PrepareStopOffer
    EXPECT_FALSE(*was_usage_marker_file_closed);
}

class SkeletonRegisterParamaterisedFixture : public SkeletonTestMockedSharedMemoryFixture,
                                             public ::testing::WithParamInterface<ElementType>
{
};

TEST_P(SkeletonRegisterParamaterisedFixture, RegisterWillCreateEventDataIfShmRegionWasCreated)
{
    SkeletonBinding::SkeletonEventBindings events{};
    SkeletonBinding::SkeletonFieldBindings fields{};
    amp::optional<SkeletonBinding::RegisterShmObjectTraceCallback> register_shm_object_trace_callback{};

    // Given a Skeleton constructed from a valid identifier referencing a QM deployment
    InitialiseSkeleton(GetValidASILInstanceIdentifier());

    // and that opening the service instance usage marker file succeeds
    ExpectServiceUsageMarkerFileCreatedOrOpenedAndClosed(kServiceInstanceUsageFilePath,
                                                         kServiceInstanceUsageFileDescriptor);

    // and that flocking the service instance usage marker file succeeds
    ExpectServiceUsageMarkerFileFlockAcquired(kServiceInstanceUsageFileDescriptor);

    // and that control (QM and ASIL-B) and data segments are successfully created
    ExpectControlSegmentCreated(QualityType::kASIL_QM);
    ExpectControlSegmentCreated(QualityType::kASIL_B);
    ExpectDataSegmentCreated();

    // when calling PrepareOffer ... expect, that it succeeds
    EXPECT_TRUE(skeleton_->PrepareOffer(events, fields, std::move(register_shm_object_trace_callback)).has_value());

    // when the event is registered with the skeleton
    const auto* const lola_service_type_deployment = GetLolaServiceTypeDeployment(test::kValidMinimalTypeDeployment);
    ElementFqId event_fqn{
        lola_service_type_deployment->service_id_, test::kFooEventId, test::kDefaultLolaInstanceId, ElementType::EVENT};
    auto [typed_event_data_storage_ptr, event_data_control_composite] =
        skeleton_->Register<test::TestSampleType>(event_fqn, test::kDefaultEventProperties);

    // Then the Register call should return pointers to the created control and data sections which can be used to
    // allocate slots
    auto allocation = event_data_control_composite.AllocateNextSlot();
    ASSERT_TRUE(allocation.first.has_value());

    EXPECT_NE(typed_event_data_storage_ptr, nullptr);
}

TEST_P(SkeletonRegisterParamaterisedFixture, RegisterWillOpenEventDataIfShmRegionWasOpened)
{
    const ElementFqId element_fq_id{1U, 2U, 3U, ElementType::EVENT};

    ServiceDataControl service_data_control_qm{CreateServiceDataControlWithEvent(element_fq_id, QualityType::kASIL_QM)};
    ServiceDataControl service_data_control_asil_b{
        CreateServiceDataControlWithEvent(element_fq_id, QualityType::kASIL_B)};
    ServiceDataStorage service_data_storage{CreateServiceDataStorageWithEvent<test::TestSampleType>(element_fq_id)};

    const ElementType element_type = GetParam();

    mock_binding::SkeletonEvent<std::string> event{};
    SkeletonBinding::SkeletonEventBindings events{};
    SkeletonBinding::SkeletonFieldBindings fields{};
    amp::optional<SkeletonBinding::RegisterShmObjectTraceCallback> register_shm_object_trace_callback{};

    if (element_type == ElementType::EVENT)
    {
        events.emplace(test::kFooEventName, event);
    }
    else
    {
        fields.emplace(test::kFooEventName, event);
    }
    const InstanceIdentifier instance_identifier{element_type == ElementType::EVENT
                                                     ? GetValidASILInstanceIdentifierWithEvent()
                                                     : GetValidASILInstanceIdentifierWithField()};

    // Given a Skeleton constructed from a valid identifier referencing a QM deployment
    InitialiseSkeleton(instance_identifier);

    // and that opening the service instance usage marker file succeeds
    ExpectServiceUsageMarkerFileCreatedOrOpenedAndClosed(kServiceInstanceUsageFilePath,
                                                         kServiceInstanceUsageFileDescriptor);

    // and that flocking the service instance usage marker file fails
    ExpectServiceUsageMarkerFileAlreadyFlocked(kServiceInstanceUsageFileDescriptor);

    // and that the control (QM and ASIL-B) and data segments are successfully opened
    ExpectControlSegmentOpened(QualityType::kASIL_QM, service_data_control_qm);
    ExpectControlSegmentOpened(QualityType::kASIL_B, service_data_control_asil_b);
    ExpectDataSegmentOpened(service_data_storage);

    // when calling PrepareOffer ... expect, that it succeeds
    EXPECT_TRUE(skeleton_->PrepareOffer(events, fields, std::move(register_shm_object_trace_callback)).has_value());

    // when the event is registered with the skeleton
    auto [typed_event_data_storage_ptr, event_data_control_composite] =
        skeleton_->Register<test::TestSampleType>(element_fq_id, test::kDefaultEventProperties);

    // Then the Register call should return pointers to the opened control and data sections in the opened shared
    // memory region
    auto& event_control_qm = GetEventControlFromServiceDataControl(element_fq_id, service_data_control_qm);
    auto& event_control_asil_b = GetEventControlFromServiceDataControl(element_fq_id, service_data_control_asil_b);
    auto& event_data_storage =
        GetEventStorageFromServiceDataStorage<test::TestSampleType>(element_fq_id, service_data_storage);

    EXPECT_EQ(&event_data_control_composite.GetQmEventDataControl(), &event_control_qm.data_control);
    ASSERT_TRUE(event_data_control_composite.GetAsilBEventDataControl().has_value());
    EXPECT_EQ(event_data_control_composite.GetAsilBEventDataControl().value(), &event_control_asil_b.data_control);
    EXPECT_EQ(typed_event_data_storage_ptr, &event_data_storage);
}

TEST_P(SkeletonRegisterParamaterisedFixture, RollbackWillBeCalledIfShmRegionWasOpened)
{
    const ElementFqId element_fq_id{1U, 2U, 3U, ElementType::EVENT};

    ServiceDataStorage service_data_storage{CreateServiceDataStorageWithEvent<test::TestSampleType>(element_fq_id)};

    // Given a QM ServiceDataControl which contains a TransactionLogSet with valid transactions
    ServiceDataControl service_data_control_qm{CreateServiceDataControlWithEvent(element_fq_id, QualityType::kASIL_QM)};
    auto& event_data_control_qm =
        GetEventControlFromServiceDataControl(element_fq_id, service_data_control_qm).data_control;
    InsertSkeletonTransactionLogWithValidTransactions(event_data_control_qm);
    EXPECT_TRUE(IsSkeletonTransactionLogRegistered(event_data_control_qm));

    const ElementType element_type = GetParam();

    mock_binding::SkeletonEvent<std::string> event{};
    SkeletonBinding::SkeletonEventBindings events{};
    SkeletonBinding::SkeletonFieldBindings fields{};
    amp::optional<SkeletonBinding::RegisterShmObjectTraceCallback> register_shm_object_trace_callback{};

    if (element_type == ElementType::EVENT)
    {
        events.emplace(test::kFooEventName, event);
    }
    else
    {
        fields.emplace(test::kFooEventName, event);
    }
    const InstanceIdentifier instance_identifier{element_type == ElementType::EVENT
                                                     ? GetValidInstanceIdentifierWithEvent()
                                                     : GetValidInstanceIdentifierWithField()};

    // Given a Skeleton constructed from a valid identifier referencing a QM deployment
    InitialiseSkeleton(instance_identifier);

    // and that opening the service instance usage marker file succeeds
    ExpectServiceUsageMarkerFileCreatedOrOpenedAndClosed(kServiceInstanceUsageFilePath,
                                                         kServiceInstanceUsageFileDescriptor);

    // and that flocking the service instance usage marker file fails
    ExpectServiceUsageMarkerFileAlreadyFlocked(kServiceInstanceUsageFileDescriptor);

    // and that QM control segment and data segments are successfully opened
    ExpectControlSegmentOpened(QualityType::kASIL_QM, service_data_control_qm);
    ExpectDataSegmentOpened(service_data_storage);

    // when calling PrepareOffer ... expect, that it succeeds
    EXPECT_TRUE(skeleton_->PrepareOffer(events, fields, std::move(register_shm_object_trace_callback)).has_value());

    // when the event is registered with the skeleton
    amp::ignore = skeleton_->Register<test::TestSampleType>(element_fq_id, test::kDefaultEventProperties);

    // Then the TransactionLog should be rollbacked during construction and removed
    EXPECT_FALSE(IsSkeletonTransactionLogRegistered(event_data_control_qm));
}

TEST_P(SkeletonRegisterParamaterisedFixture, RollbackWillOnlyBeCalledOnQmControlSection)
{
    // Note. this test is artificially inserting transactions into the AsilB control section's TransactionLogSet. In
    // practice, this TransactionLogSet will never be used.
    const ElementFqId element_fq_id{1U, 2U, 3U, ElementType::EVENT};

    ServiceDataStorage service_data_storage{CreateServiceDataStorageWithEvent<test::TestSampleType>(element_fq_id)};
    ServiceDataControl service_data_control_qm{CreateServiceDataControlWithEvent(element_fq_id, QualityType::kASIL_QM)};

    // Given an Asil B ServiceDataControl which contains a TransactionLogSet with valid transactions
    ServiceDataControl service_data_control_asil_b{
        CreateServiceDataControlWithEvent(element_fq_id, QualityType::kASIL_B)};
    auto& event_data_control_asil_b =
        GetEventControlFromServiceDataControl(element_fq_id, service_data_control_asil_b).data_control;
    InsertSkeletonTransactionLogWithValidTransactions(event_data_control_asil_b);
    EXPECT_TRUE(IsSkeletonTransactionLogRegistered(event_data_control_asil_b));

    const ElementType element_type = GetParam();

    mock_binding::SkeletonEvent<std::string> event{};
    SkeletonBinding::SkeletonEventBindings events{};
    SkeletonBinding::SkeletonFieldBindings fields{};
    amp::optional<SkeletonBinding::RegisterShmObjectTraceCallback> register_shm_object_trace_callback{};

    if (element_type == ElementType::EVENT)
    {
        events.emplace(test::kFooEventName, event);
    }
    else
    {
        fields.emplace(test::kFooEventName, event);
    }
    const InstanceIdentifier instance_identifier{element_type == ElementType::EVENT
                                                     ? GetValidASILInstanceIdentifierWithEvent()
                                                     : GetValidASILInstanceIdentifierWithField()};

    // Given a Skeleton constructed from a valid identifier referencing a QM deployment
    InitialiseSkeleton(instance_identifier);

    // and that opening the service instance usage marker file succeeds
    ExpectServiceUsageMarkerFileCreatedOrOpenedAndClosed(kServiceInstanceUsageFilePath,
                                                         kServiceInstanceUsageFileDescriptor);

    // and that flocking the service instance usage marker file fails
    ExpectServiceUsageMarkerFileAlreadyFlocked(kServiceInstanceUsageFileDescriptor);

    // and that the control (QM and ASIL-B) and data segments are successfully opened
    ExpectControlSegmentOpened(QualityType::kASIL_QM, service_data_control_qm);
    ExpectControlSegmentOpened(QualityType::kASIL_B, service_data_control_asil_b);
    ExpectDataSegmentOpened(service_data_storage);

    // when calling PrepareOffer ... expect, that it succeeds
    EXPECT_TRUE(skeleton_->PrepareOffer(events, fields, std::move(register_shm_object_trace_callback)).has_value());

    // when the event is registered with the skeleton
    amp::ignore = skeleton_->Register<test::TestSampleType>(element_fq_id, test::kDefaultEventProperties);

    // Then the Asil B TransactionLog will still exist as it was not rolled back
    EXPECT_TRUE(IsSkeletonTransactionLogRegistered(event_data_control_asil_b));
}

TEST_P(SkeletonRegisterParamaterisedFixture, TracingWillBeDisabledAndTransactionLogRemainsIfRollbackFails)
{
    const ElementFqId element_fq_id{1U, 2U, 3U, ElementType::EVENT};
    impl::tracing::TracingRuntimeMock tracing_runtime_mock{};
    ON_CALL(runtime_mock_, GetTracingRuntime()).WillByDefault(Return(&tracing_runtime_mock));

    ServiceDataStorage service_data_storage{CreateServiceDataStorageWithEvent<test::TestSampleType>(element_fq_id)};

    // Given a QM ServiceDataControl which contains a TransactionLogSet with invalid transactions
    ServiceDataControl service_data_control_qm{CreateServiceDataControlWithEvent(element_fq_id, QualityType::kASIL_QM)};
    auto& event_data_control_qm =
        GetEventControlFromServiceDataControl(element_fq_id, service_data_control_qm).data_control;
    InsertSkeletonTransactionLogWithInvalidTransactions(event_data_control_qm);
    EXPECT_TRUE(IsSkeletonTransactionLogRegistered(event_data_control_qm));

    const ElementType element_type = GetParam();

    mock_binding::SkeletonEvent<std::string> event{};
    SkeletonBinding::SkeletonEventBindings events{};
    SkeletonBinding::SkeletonFieldBindings fields{};
    amp::optional<SkeletonBinding::RegisterShmObjectTraceCallback> register_shm_object_trace_callback{};

    if (element_type == ElementType::EVENT)
    {
        events.emplace(test::kFooEventName, event);
    }
    else
    {
        fields.emplace(test::kFooEventName, event);
    }
    const InstanceIdentifier instance_identifier{element_type == ElementType::EVENT
                                                     ? GetValidInstanceIdentifierWithEvent()
                                                     : GetValidInstanceIdentifierWithField()};

    // Given a Skeleton constructed from a valid identifier referencing a QM deployment
    InitialiseSkeleton(instance_identifier);

    // and that opening the service instance usage marker file succeeds
    ExpectServiceUsageMarkerFileCreatedOrOpenedAndClosed(kServiceInstanceUsageFilePath,
                                                         kServiceInstanceUsageFileDescriptor);

    // and that flocking the service instance usage marker file fails
    ExpectServiceUsageMarkerFileAlreadyFlocked(kServiceInstanceUsageFileDescriptor);

    // and that the QM control and data segments are successfully opened
    ExpectControlSegmentOpened(QualityType::kASIL_QM, service_data_control_qm);
    ExpectDataSegmentOpened(service_data_storage);

    // and that tracing will be disabled
    EXPECT_CALL(tracing_runtime_mock, DisableTracing());

    // when calling PrepareOffer ... expect, that it succeeds
    skeleton_->PrepareOffer(events, fields, std::move(register_shm_object_trace_callback));

    // when the event is registered with the skeleton
    amp::ignore = skeleton_->Register<test::TestSampleType>(element_fq_id, test::kDefaultEventProperties);

    // Then the TransactionLog should still exist as it was not removed due to the rollback failing
    EXPECT_TRUE(IsSkeletonTransactionLogRegistered(event_data_control_qm));
}

TEST_P(SkeletonRegisterParamaterisedFixture, TransactionLogWillBeRegisteredIfShmRegionWasCreated)
{
    SkeletonBinding::SkeletonEventBindings events{};
    SkeletonBinding::SkeletonFieldBindings fields{};
    amp::optional<SkeletonBinding::RegisterShmObjectTraceCallback> register_shm_object_trace_callback{};

    impl::tracing::SkeletonEventTracingData skeleton_event_tracing_data{};
    skeleton_event_tracing_data.enable_send = true;

    // Given a Skeleton constructed from a valid identifier referencing a QM deployment
    InitialiseSkeleton(GetValidASILInstanceIdentifier());

    // and that opening the service instance usage marker file succeeds
    ExpectServiceUsageMarkerFileCreatedOrOpenedAndClosed(kServiceInstanceUsageFilePath,
                                                         kServiceInstanceUsageFileDescriptor);

    // and that flocking the service instance usage marker file succeeds
    ExpectServiceUsageMarkerFileFlockAcquired(kServiceInstanceUsageFileDescriptor);

    // and that control (QM and ASIL-B) and data segments are successfully created
    ExpectControlSegmentCreated(QualityType::kASIL_QM);
    ExpectControlSegmentCreated(QualityType::kASIL_B);
    ExpectDataSegmentCreated();

    // when calling PrepareOffer ... expect, that it succeeds
    EXPECT_TRUE(skeleton_->PrepareOffer(events, fields, std::move(register_shm_object_trace_callback)).has_value());

    // when the event is registered with the skeleton while tracing is enabled
    const auto* const lola_service_type_deployment = GetLolaServiceTypeDeployment(test::kValidMinimalTypeDeployment);
    ElementFqId event_fqn{
        lola_service_type_deployment->service_id_, test::kFooEventId, test::kDefaultLolaInstanceId, ElementType::EVENT};
    auto [typed_event_data_storage_ptr, event_data_control_composite] = skeleton_->Register<test::TestSampleType>(
        event_fqn, test::kDefaultEventProperties, skeleton_event_tracing_data);

    // Then a TransactionLog should have been registered
    EXPECT_TRUE(IsSkeletonTransactionLogRegistered(event_data_control_composite.GetQmEventDataControl()));
}

TEST_P(SkeletonRegisterParamaterisedFixture,
       TransactionLogWillBeRegisteredIfShmRegionWasOpenedAndRollbackSucceededAndTracingEnabled)
{
    const ElementFqId element_fq_id{1U, 2U, 3U, ElementType::EVENT};

    impl::tracing::SkeletonEventTracingData skeleton_event_tracing_data{};
    skeleton_event_tracing_data.enable_send = true;

    ServiceDataStorage service_data_storage{CreateServiceDataStorageWithEvent<test::TestSampleType>(element_fq_id)};

    // Given a QM ServiceDataControl which contains a TransactionLogSet with valid transactions
    ServiceDataControl service_data_control_qm{CreateServiceDataControlWithEvent(element_fq_id, QualityType::kASIL_QM)};
    auto& event_data_control_qm =
        GetEventControlFromServiceDataControl(element_fq_id, service_data_control_qm).data_control;
    InsertSkeletonTransactionLogWithValidTransactions(event_data_control_qm);
    EXPECT_TRUE(IsSkeletonTransactionLogRegistered(event_data_control_qm));

    const ElementType element_type = GetParam();

    mock_binding::SkeletonEvent<std::string> event{};
    SkeletonBinding::SkeletonEventBindings events{};
    SkeletonBinding::SkeletonFieldBindings fields{};
    amp::optional<SkeletonBinding::RegisterShmObjectTraceCallback> register_shm_object_trace_callback{};

    if (element_type == ElementType::EVENT)
    {
        events.emplace(test::kFooEventName, event);
    }
    else
    {
        fields.emplace(test::kFooEventName, event);
    }
    const InstanceIdentifier instance_identifier{element_type == ElementType::EVENT
                                                     ? GetValidInstanceIdentifierWithEvent()
                                                     : GetValidInstanceIdentifierWithField()};

    // Given a Skeleton constructed from a valid identifier referencing a QM deployment
    InitialiseSkeleton(instance_identifier);

    // and that opening the service instance usage marker file succeeds
    ExpectServiceUsageMarkerFileCreatedOrOpenedAndClosed(kServiceInstanceUsageFilePath,
                                                         kServiceInstanceUsageFileDescriptor);

    // and that flocking the service instance usage marker file fails
    ExpectServiceUsageMarkerFileAlreadyFlocked(kServiceInstanceUsageFileDescriptor);

    // and that the QM control and data segments are successfully opened
    ExpectControlSegmentOpened(QualityType::kASIL_QM, service_data_control_qm);
    ExpectDataSegmentOpened(service_data_storage);

    // when calling PrepareOffer ... expect, that it succeeds
    EXPECT_TRUE(skeleton_->PrepareOffer(events, fields, std::move(register_shm_object_trace_callback)).has_value());

    // when the event is registered with the skeleton
    amp::ignore = skeleton_->Register<test::TestSampleType>(
        element_fq_id, test::kDefaultEventProperties, skeleton_event_tracing_data);

    // Then a new TransactionLog containing no transactions should have been registered (after the previous
    // TransactionLog was rolled back)
    EXPECT_TRUE(IsSkeletonTransactionLogRegistered(event_data_control_qm));
    EXPECT_FALSE(DoesSkeletonTransactionLogContainTransactions(event_data_control_qm));
}

/// \brief Test case simulates (via a mock SkeletonEvent) the registration of a SkeletonEvent at its parent Skeleton
/// during Skeleton::PrepareOffer() and checks, that after this registration, the related event-data-slots
/// are accessible.
TEST_P(SkeletonRegisterParamaterisedFixture, ValidEventDataSlotsExistAfterEventIsRegistered)
{
    using namespace memory::shared;

    const ElementType element_type = GetParam();

    mock_binding::SkeletonEvent<std::string> event{};
    SkeletonBinding::SkeletonEventBindings events{};
    SkeletonBinding::SkeletonFieldBindings fields{};
    amp::optional<SkeletonBinding::RegisterShmObjectTraceCallback> register_shm_object_trace_callback{};

    if (element_type == ElementType::EVENT)
    {
        events.emplace(test::kFooEventName, event);
    }
    else
    {
        fields.emplace(test::kFooEventName, event);
    }
    const InstanceIdentifier instance_identifier{element_type == ElementType::EVENT
                                                     ? GetValidInstanceIdentifierWithEvent()
                                                     : GetValidInstanceIdentifierWithField()};

    // Given a skeleton with one event "fooEvent" registered
    InitialiseSkeleton(instance_identifier);

    // Expect that the usage marker file path is created and closed
    ExpectServiceUsageMarkerFileCreatedOrOpenedAndClosed();

    // When trying to create a QM control segment succeeds
    ExpectControlSegmentCreated(QualityType::kASIL_QM);

    // and trying to create a data segment succeeds
    ExpectDataSegmentCreated();

    // when PrepareOffer the service
    skeleton_->PrepareOffer(events, fields, std::move(register_shm_object_trace_callback));

    // when the event is registered with the skeleton
    const auto* const lola_service_type_deployment = GetLolaServiceTypeDeployment(test::kValidMinimalTypeDeployment);
    ElementFqId event_fqn{
        lola_service_type_deployment->service_id_, test::kFooEventId, test::kDefaultLolaInstanceId, element_type};
    auto event_reg_result = skeleton_->Register<test::TestSampleType>(event_fqn, test::kDefaultEventProperties);

    // Then a valid slot-vector with the right size exists and we can access/write to it:
    ASSERT_NE(event_reg_result.first, nullptr);
    ASSERT_EQ(event_reg_result.first->size(), test::kMaxSlots);
    event_reg_result.first->at(3) = 0x42;

    CleanUpSkeleton();
}

/// \brief Test case is almost identical to test ValidEventDataSlotsExistAfterEventIsRegistered above. Instead of just
/// testing the event-data-slot existence, it really does an AllocateNextSlot() call on the data structures.
TEST_P(SkeletonRegisterParamaterisedFixture, CanAllocateSlotAfterEventIsRegistered)
{
    using namespace memory::shared;

    const ElementType element_type = GetParam();

    mock_binding::SkeletonEvent<std::string> event{};
    SkeletonBinding::SkeletonEventBindings events{};
    SkeletonBinding::SkeletonFieldBindings fields{};
    amp::optional<SkeletonBinding::RegisterShmObjectTraceCallback> register_shm_object_trace_callback{};

    if (element_type == ElementType::EVENT)
    {
        events.emplace(test::kFooEventName, event);
    }
    else
    {
        fields.emplace(test::kFooEventName, event);
    }
    const InstanceIdentifier instance_identifier{element_type == ElementType::EVENT
                                                     ? GetValidInstanceIdentifierWithEvent()
                                                     : GetValidInstanceIdentifierWithField()};

    // Given a skeleton with one event "fooEvent" registered
    InitialiseSkeleton(instance_identifier);

    // Expect that the usage marker file path is created and closed
    ExpectServiceUsageMarkerFileCreatedOrOpenedAndClosed();

    // When trying to create a QM control segment succeeds
    ExpectControlSegmentCreated(QualityType::kASIL_QM);

    // and trying to create a data segment succeeds
    ExpectDataSegmentCreated();

    // when PrepareOffer the service
    skeleton_->PrepareOffer(events, fields, std::move(register_shm_object_trace_callback));

    // when the event is registered with the skeleton
    const auto* const lola_service_type_deployment = GetLolaServiceTypeDeployment(test::kValidMinimalTypeDeployment);
    ElementFqId event_fqn{
        lola_service_type_deployment->service_id_, test::kFooEventId, test::kDefaultLolaInstanceId, element_type};
    auto event_reg_result = skeleton_->Register<test::TestSampleType>(event_fqn, test::kDefaultEventProperties);

    // Then we can allocate and free slots on that event
    auto allocation = event_reg_result.second.AllocateNextSlot();
    ASSERT_TRUE(allocation.first.has_value());
    EXPECT_EQ(allocation.first.value(), 0);

    CleanUpSkeleton();
}

TEST_P(SkeletonRegisterParamaterisedFixture, AllocateAfterCleanUp)
{
    using namespace memory::shared;

    const ElementType element_type = GetParam();

    mock_binding::SkeletonEvent<std::string> event{};
    SkeletonBinding::SkeletonEventBindings events{};
    SkeletonBinding::SkeletonFieldBindings fields{};
    amp::optional<SkeletonBinding::RegisterShmObjectTraceCallback> register_shm_object_trace_callback{};

    if (element_type == ElementType::EVENT)
    {
        events.emplace(test::kFooEventName, event);
    }
    else
    {
        fields.emplace(test::kFooEventName, event);
    }
    const InstanceIdentifier instance_identifier{element_type == ElementType::EVENT
                                                     ? GetValidInstanceIdentifierWithEvent()
                                                     : GetValidInstanceIdentifierWithField()};

    // Given a skeleton with one event "fooEvent" registered
    InitialiseSkeleton(instance_identifier);

    // Expect that the usage marker file path is created and closed
    ExpectServiceUsageMarkerFileCreatedOrOpenedAndClosed();

    ExpectControlSegmentCreated(QualityType::kASIL_QM);

    ExpectDataSegmentCreated();

    skeleton_->PrepareOffer(events, fields, std::move(register_shm_object_trace_callback));

    const auto* const lola_service_type_deployment = GetLolaServiceTypeDeployment(test::kValidMinimalTypeDeployment);
    ElementFqId event_fqn{
        lola_service_type_deployment->service_id_, test::kFooEventId, test::kDefaultLolaInstanceId, element_type};
    auto event_reg_result = skeleton_->Register<test::TestSampleType>(event_fqn, test::kDefaultEventProperties);

    auto allocation = event_reg_result.second.AllocateNextSlot();

    // When cleaning up
    skeleton_->CleanupSharedMemoryAfterCrash();

    // Then the same slot can get allocated
    auto foo_allocation = event_reg_result.second.AllocateNextSlot();
    EXPECT_EQ(allocation.first.value(), foo_allocation.first.value());

    CleanUpSkeleton();
}

TEST_P(SkeletonRegisterParamaterisedFixture, ValidEventMetaInfoExistAfterEventIsRegistered)
{
    RecordProperty("ParentRequirement", "4");
    RecordProperty("Description", "Checks that the event meta info for an event is published by the Skeleton.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    /// \brief only locally used complex SampleType/event-data-type
    struct VeryComplexType
    {
        std::uint64_t m1_;
        std::float_t m2_;
        std::array<std::uint16_t, 7u> m3_;
    };

    constexpr std::size_t number_of_slots = 3U;

    const ElementType element_type = GetParam();

    // Given a skeleton with two events FOO_EVENT_NAME, "dumbEvent" registered
    // Note: We are using only maxSamples = 3 for these events as we base this test on a configured shm-size
    // CONFIGURED_DEPL_SHM_SIZE ... where the slots have to fit in.
    ServiceTypeDeployment service_type_depl = CreateTypeDeployment(
        1U, {{test::kFooEventName, test::kFooEventId}, {test::kDumbEventName, test::kDumbEventId}});

    mock_binding::SkeletonEvent<std::string> foo_event{}, dumb_event{};

    SkeletonBinding::SkeletonEventBindings events{};
    SkeletonBinding::SkeletonFieldBindings fields{};
    amp::optional<SkeletonBinding::RegisterShmObjectTraceCallback> register_shm_object_trace_callback{};

    std::vector<std::pair<std::string, LolaEventInstanceDeployment>> lola_event_inst_depls;
    std::vector<std::pair<std::string, LolaFieldInstanceDeployment>> lola_field_inst_depls;
    if (element_type == ElementType::EVENT)
    {
        events.emplace(test::kFooEventName, foo_event);
        events.emplace(test::kDumbEventName, dumb_event);

        lola_event_inst_depls.push_back(
            {test::kFooEventName, LolaEventInstanceDeployment{number_of_slots, 10U, 1U, true}});
        lola_event_inst_depls.push_back(
            {test::kDumbEventName, LolaEventInstanceDeployment{number_of_slots, 10U, 1U, true}});
    }
    else
    {
        fields.emplace(test::kFooEventName, foo_event);
        fields.emplace(test::kDumbEventName, dumb_event);

        lola_field_inst_depls.push_back(
            {test::kFooEventName, LolaFieldInstanceDeployment{number_of_slots, 10U, 1U, true}});
        lola_field_inst_depls.push_back(
            {test::kDumbEventName, LolaFieldInstanceDeployment{number_of_slots, 10U, 1U, true}});
    }
    ServiceInstanceDeployment service_instance_deployment{
        test::kFooService,
        CreateLolaServiceInstanceDeployment(test::kDefaultLolaInstanceId,
                                            lola_event_inst_depls,
                                            lola_field_inst_depls,
                                            {},
                                            {},
                                            test::kConfiguredDeploymentShmSize),
        QualityType::kASIL_QM,
        test::kFooInstanceSpecifier};

    const auto instance_identifier = make_InstanceIdentifier(service_instance_deployment, service_type_depl);
    InitialiseSkeleton(instance_identifier);

    // Expect that the usage marker file path is created and closed
    ExpectServiceUsageMarkerFileCreatedOrOpenedAndClosed();

    // When trying to create a QM control segment succeeds
    ExpectControlSegmentCreated(QualityType::kASIL_QM);

    // and trying to create a data segment succeeds
    ExpectDataSegmentCreated();

    // when the service offering is prepared
    skeleton_->PrepareOffer(events, fields, std::move(register_shm_object_trace_callback));

    // and foo_event is registered with the skeleton with 5 slots
    const auto* const lola_service_type_deployment = GetLolaServiceTypeDeployment(test::kValidMinimalTypeDeployment);
    ElementFqId foo_event_fqn{
        lola_service_type_deployment->service_id_, test::kFooEventId, test::kDefaultLolaInstanceId, element_type};
    auto foo_event_reg_result = skeleton_->Register<uint8_t>(foo_event_fqn, test::kDefaultEventProperties);
    void* const foo_event_data_storage = foo_event_reg_result.first->data();

    // and dumb_event is registered with the skeleton with 5 slots
    ElementFqId dumb_event_fqn{
        lola_service_type_deployment->service_id_, test::kDumbEventId, test::kDefaultLolaInstanceId, element_type};
    auto dumb_event_reg_result = skeleton_->Register<VeryComplexType>(dumb_event_fqn, test::kDefaultEventProperties);
    void* const dumb_event_data_storage = dumb_event_reg_result.first->data();

    // Expect, that we can then retrieve the meta-info of the registered events
    auto event_foo_meta_info_ptr = skeleton_->GetEventMetaInfo(ElementFqId{
        lola_service_type_deployment->service_id_, test::kFooEventId, test::kDefaultLolaInstanceId, element_type});
    auto event_dumb_meta_info_ptr = skeleton_->GetEventMetaInfo(ElementFqId{
        lola_service_type_deployment->service_id_, test::kDumbEventId, test::kDefaultLolaInstanceId, element_type});

    // and the meta-info for these events is valid
    ASSERT_TRUE(event_foo_meta_info_ptr.has_value());
    ASSERT_TRUE(event_dumb_meta_info_ptr.has_value());
    // and they have the expected properties
    ASSERT_EQ(event_foo_meta_info_ptr->data_type_info_.size_of_, sizeof(std::uint8_t));
    ASSERT_EQ(event_foo_meta_info_ptr->data_type_info_.align_of_, alignof(std::uint8_t));
    ASSERT_EQ(event_foo_meta_info_ptr->event_slots_raw_array_.get(), foo_event_data_storage);

    ASSERT_EQ(event_dumb_meta_info_ptr->data_type_info_.size_of_, sizeof(VeryComplexType));
    ASSERT_EQ(event_dumb_meta_info_ptr->data_type_info_.align_of_, alignof(VeryComplexType));
    ASSERT_EQ(event_dumb_meta_info_ptr->event_slots_raw_array_.get(), dumb_event_data_storage);

    CleanUpSkeleton();
}

TEST_P(SkeletonRegisterParamaterisedFixture, NoMetaInfoExistsForInvalidElementId)
{
    const ElementType element_type = GetParam();

    mock_binding::SkeletonEvent<std::string> event{};
    SkeletonBinding::SkeletonEventBindings events{};
    SkeletonBinding::SkeletonFieldBindings fields{};
    amp::optional<SkeletonBinding::RegisterShmObjectTraceCallback> register_shm_object_trace_callback{};

    if (element_type == ElementType::EVENT)
    {
        events.emplace(test::kFooEventName, event);
    }
    else
    {
        fields.emplace(test::kFooEventName, event);
    }
    const InstanceIdentifier instance_identifier{element_type == ElementType::EVENT
                                                     ? GetValidInstanceIdentifierWithEvent()
                                                     : GetValidInstanceIdentifierWithField()};

    // Given a skeleton with one event "fooEvent" registered
    InitialiseSkeleton(instance_identifier);

    // Expect that the usage marker file path is created and closed
    ExpectServiceUsageMarkerFileCreatedOrOpenedAndClosed();

    // When trying to create a QM control segment succeeds
    ExpectControlSegmentCreated(QualityType::kASIL_QM);

    // and trying to create a data segment succeeds
    ExpectDataSegmentCreated();

    // when PrepareOffer the service
    skeleton_->PrepareOffer(events, fields, std::move(register_shm_object_trace_callback));

    // and the event is registered with the skeleton
    const auto* const lola_service_type_deployment = GetLolaServiceTypeDeployment(test::kValidMinimalTypeDeployment);
    ElementFqId event_fqn{
        lola_service_type_deployment->service_id_, test::kFooEventId, test::kDefaultLolaInstanceId, element_type};
    skeleton_->Register<uint8_t>(event_fqn, test::kDefaultEventProperties);

    // but when retrieving meta-info for a not registered ElementFqId
    const std::uint16_t UNKNOWN_EVENT_ID{99U};
    ElementFqId event_unknown_fqn{
        lola_service_type_deployment->service_id_, UNKNOWN_EVENT_ID, test::kDefaultLolaInstanceId, element_type};
    auto event_unknown_meta_info = skeleton_->GetEventMetaInfo(event_unknown_fqn);

    // we expect the meta-info for this event is invalid
    ASSERT_FALSE(event_unknown_meta_info.has_value());

    CleanUpSkeleton();
}

TEST_P(SkeletonRegisterParamaterisedFixture, CallingRegisterWithSameServiceElementTwiceWillTerminate)
{
    RecordProperty("Verifies", "9");
    RecordProperty("Description",
                   "Checks that trying to register 2 service elements with the same ElementFqId will terminate. If "
                   "the identification of a service element is incorrect (e.g. different service elements map to the "
                   "same ElementFqId), then registering the second service element will not overwrite the first "
                   "service element. It will instead lead to a termination.");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("Priority", "1");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    auto test_function = [this]() noexcept {
        SkeletonBinding::SkeletonEventBindings events{};
        SkeletonBinding::SkeletonFieldBindings fields{};
        amp::optional<SkeletonBinding::RegisterShmObjectTraceCallback> register_shm_object_trace_callback{};

        // Given a Skeleton constructed from a valid identifier referencing a QM deployment
        InitialiseSkeleton(GetValidASILInstanceIdentifier());

        // and that opening the service instance usage marker file succeeds
        ExpectServiceUsageMarkerFileCreatedOrOpenedAndClosed(kServiceInstanceUsageFilePath,
                                                             kServiceInstanceUsageFileDescriptor);

        // and that control (QM and ASIL-B) and data segments are successfully created
        ExpectControlSegmentCreated(QualityType::kASIL_QM);
        ExpectControlSegmentCreated(QualityType::kASIL_B);
        ExpectDataSegmentCreated();

        EXPECT_TRUE(skeleton_->PrepareOffer(events, fields, std::move(register_shm_object_trace_callback)).has_value());

        const auto* const lola_service_type_deployment =
            GetLolaServiceTypeDeployment(test::kValidMinimalTypeDeployment);
        ElementFqId event_fqn{lola_service_type_deployment->service_id_,
                              test::kFooEventId,
                              test::kDefaultLolaInstanceId,
                              ElementType::EVENT};

        // When calling register twice with the same ElementFqId
        amp::ignore = skeleton_->Register<test::TestSampleType>(event_fqn, test::kDefaultEventProperties);
        amp::ignore = skeleton_->Register<test::TestSampleType>(event_fqn, test::kDefaultEventProperties);
    };
    // Then we should terminate
    EXPECT_DEATH(test_function(), ".*");
}

INSTANTIATE_TEST_SUITE_P(SkeletonRegisterParamaterisedFixture,
                         SkeletonRegisterParamaterisedFixture,
                         ::testing::Values(ElementType::EVENT, ElementType::FIELD));

class SkeletonCreateFixture : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        ON_CALL(*fcntl_mock_, flock(_, kNonBlockingExclusiveLockOperation)).WillByDefault(Return(amp::blank{}));
        ON_CALL(*fcntl_mock_, flock(_, kUnlockOperation)).WillByDefault(Return(amp::blank{}));

        ON_CALL(partial_restart_path_builder_mock_, GetLolaPartialRestartDirectoryPath())
            .WillByDefault(Return(partial_restart_directory_path_.Native()));
        ON_CALL(partial_restart_path_builder_mock_,
                GetServiceInstanceExistenceMarkerFilePath(test::kDefaultLolaInstanceId))
            .WillByDefault(Return(service_existence_marker_file_path_));

        ON_CALL(*fcntl_mock_, open(StrEq(service_existence_marker_file_path_.data()), _, _))
            .WillByDefault(Return(existence_marker_file_descriptor_));
    }

#if defined(__QNXNTO__)
    filesystem::Path partial_restart_directory_path_{"/tmp_discovery/partial_restart_directory_path"};
    std::string service_existence_marker_file_path_{"/tmp_discovery/service_existence_marker_file_path"};
#else
    filesystem::Path partial_restart_directory_path_{"/tmp/partial_restart_directory_path"};
    std::string service_existence_marker_file_path_{"/tmp/service_existence_marker_file_path"};
#endif

    InstanceIdentifier instance_identifier_ =
        make_InstanceIdentifier(test::kValidMinimalAsilInstanceDeployment, test::kValidMinimalTypeDeployment);
    os::MockGuard<os::FcntlMock> fcntl_mock_{};

    std::unique_ptr<ShmPathBuilderMock> shm_path_builder_mock_ptr_{std::make_unique<ShmPathBuilderMock>()};
    std::unique_ptr<PartialRestartPathBuilderMock> partial_restart_path_builder_mock_ptr_{
        std::make_unique<PartialRestartPathBuilderMock>()};

    ShmPathBuilderMock& shm_path_builder_mock_{*shm_path_builder_mock_ptr_};
    PartialRestartPathBuilderMock& partial_restart_path_builder_mock_{*partial_restart_path_builder_mock_ptr_};

    const std::int32_t existence_marker_file_descriptor_{10U};
};

TEST_F(SkeletonCreateFixture, CreateWorks)
{
    EXPECT_NE(lola::Skeleton::Create(instance_identifier_,
                                     filesystem::FilesystemFactory{}.CreateInstance(),
                                     std::move(shm_path_builder_mock_ptr_),
                                     std::move(partial_restart_path_builder_mock_ptr_)),
              nullptr);
}

TEST_F(SkeletonCreateFixture, CreatingSkeletonWillCreateExistenceMarkerFile)
{
    RecordProperty("Verifies", "6");
    RecordProperty("Description", "Checks that creating a Skeleton will create a service existence marker file.");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("Priority", "1");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    // Expecting that the service existence marker file will be opened
    EXPECT_CALL(*fcntl_mock_, open(StrEq(service_existence_marker_file_path_.data()), _, _))
        .WillOnce(Return(existence_marker_file_descriptor_));

    // When creating a Skeleton
    amp::ignore = lola::Skeleton::Create(instance_identifier_,
                                         filesystem::FilesystemFactory{}.CreateInstance(),
                                         std::move(shm_path_builder_mock_ptr_),
                                         std::move(partial_restart_path_builder_mock_ptr_));
}

TEST_F(SkeletonCreateFixture, CreatingSkeletonWillTryToLockExistenceMarkerFile)
{
    RecordProperty("Verifies", "6");
    RecordProperty("Description", "Checks that creating a Skeleton will flock the service existence marker file.");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("Priority", "1");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    // Expecting that the service existence marker file will be flocked
    EXPECT_CALL(*fcntl_mock_, flock(existence_marker_file_descriptor_, kNonBlockingExclusiveLockOperation))
        .WillOnce(Return(amp::blank{}));
    EXPECT_CALL(*fcntl_mock_, flock(existence_marker_file_descriptor_, kUnlockOperation))
        .WillOnce(Return(amp::blank{}));

    // When creating a Skeleton
    amp::ignore = lola::Skeleton::Create(instance_identifier_,
                                         filesystem::FilesystemFactory{}.CreateInstance(),
                                         std::move(shm_path_builder_mock_ptr_),
                                         std::move(partial_restart_path_builder_mock_ptr_));
}

TEST_F(SkeletonCreateFixture, CreateReturnsNullPtrIfAnotherInstanceOfTheSameSkeletonStillExists)
{
    auto skeleton_0 = lola::Skeleton::Create(instance_identifier_,
                                             filesystem::FilesystemFactory{}.CreateInstance(),
                                             std::move(shm_path_builder_mock_ptr_),
                                             std::move(partial_restart_path_builder_mock_ptr_));
    EXPECT_NE(skeleton_0, nullptr);

    std::unique_ptr<ShmPathBuilderMock> shm_path_builder_mock_ptr_1{std::make_unique<ShmPathBuilderMock>()};
    std::unique_ptr<PartialRestartPathBuilderMock> partial_restart_path_builder_mock_ptr_1{
        std::make_unique<PartialRestartPathBuilderMock>()};
    auto skeleton_1 = lola::Skeleton::Create(instance_identifier_,
                                             filesystem::FilesystemFactory{}.CreateInstance(),
                                             std::move(shm_path_builder_mock_ptr_1),
                                             std::move(partial_restart_path_builder_mock_ptr_1));
    EXPECT_EQ(skeleton_1, nullptr);
}

TEST_F(SkeletonCreateFixture, CreateReturnsNullPtrIfCreatePartialRestartDirFails)
{
    bmw::filesystem::FilesystemFactoryFake filesystem_fake{};
    EXPECT_CALL(filesystem_fake.GetUtils(), CreateDirectories(partial_restart_directory_path_, _))
        .WillOnce(Return(MakeUnexpected(bmw::filesystem::ErrorCode::kCouldNotCreateDirectory)));

    EXPECT_EQ(lola::Skeleton::Create(instance_identifier_,
                                     filesystem_fake.CreateInstance(),
                                     std::move(shm_path_builder_mock_ptr_),
                                     std::move(partial_restart_path_builder_mock_ptr_)),
              nullptr);
}

TEST_F(SkeletonCreateFixture, CreateReturnsNullPtrIfOpeningServiceExistenceMarkerFileFails)
{
    EXPECT_CALL(*fcntl_mock_, open(StrEq(service_existence_marker_file_path_.data()), _, _))
        .WillOnce(Return(amp::make_unexpected(os::Error::createFromErrno(EPERM))));

    EXPECT_EQ(lola::Skeleton::Create(instance_identifier_,
                                     filesystem::FilesystemFactory{}.CreateInstance(),
                                     std::move(shm_path_builder_mock_ptr_),
                                     std::move(partial_restart_path_builder_mock_ptr_)),
              nullptr);
}

TEST_F(SkeletonCreateFixture, CreateReturnsSkeletonIfExistenceMarkerFileCanBeExclusivelyLocked)
{
    ON_CALL(*fcntl_mock_, flock(existence_marker_file_descriptor_, kNonBlockingExclusiveLockOperation))
        .WillByDefault(Return(amp::blank{}));
    ON_CALL(*fcntl_mock_, flock(existence_marker_file_descriptor_, kUnlockOperation))
        .WillByDefault(Return(amp::blank{}));

    EXPECT_NE(lola::Skeleton::Create(instance_identifier_,
                                     filesystem::FilesystemFactory{}.CreateInstance(),
                                     std::move(shm_path_builder_mock_ptr_),
                                     std::move(partial_restart_path_builder_mock_ptr_)),
              nullptr);
}

TEST_F(SkeletonCreateFixture, CreateReturnsNullPtrIfExistenceMarkerFileCannotBeExclusivelyLocked)
{
    RecordProperty("Verifies", "6, 7");
    RecordProperty("Description",
                   "Checks that a Skeleton cannot be created if the service existence marker file cannot be flocked.");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("Priority", "1");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    EXPECT_CALL(*fcntl_mock_, flock(existence_marker_file_descriptor_, kNonBlockingExclusiveLockOperation))
        .WillOnce(Return(amp::make_unexpected(os::Error::createFromErrno(EWOULDBLOCK))));

    EXPECT_EQ(lola::Skeleton::Create(instance_identifier_,
                                     filesystem::FilesystemFactory{}.CreateInstance(),
                                     std::move(shm_path_builder_mock_ptr_),
                                     std::move(partial_restart_path_builder_mock_ptr_)),
              nullptr);
}

}  // namespace
}  // namespace bmw::mw::com::impl::lola
