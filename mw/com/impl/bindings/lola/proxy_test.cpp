// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#include "platform/aas/mw/com/impl/bindings/lola/proxy.h"
#include "platform/aas/mw/com/impl/bindings/lola/element_fq_id.h"
#include "platform/aas/mw/com/impl/bindings/lola/runtime_mock.h"
#include "platform/aas/mw/com/impl/bindings/lola/service_data_control.h"
#include "platform/aas/mw/com/impl/bindings/lola/service_data_storage.h"
#include "platform/aas/mw/com/impl/bindings/lola/shm_path_builder_mock.h"
#include "platform/aas/mw/com/impl/bindings/lola/test/proxy_event_test_resources.h"
#include "platform/aas/mw/com/impl/bindings/lola/test/transaction_log_test_resources.h"
#include "platform/aas/mw/com/impl/bindings/lola/test_doubles/fake_memory_resource.h"
#include "platform/aas/mw/com/impl/bindings/mock_binding/proxy_event.h"
#include "platform/aas/mw/com/impl/configuration/quality_type.h"
#include "platform/aas/mw/com/impl/configuration/service_identifier_type.h"
#include "platform/aas/mw/com/impl/handle_type.h"
#include "platform/aas/mw/com/impl/instance_identifier.h"
#include "platform/aas/mw/com/impl/runtime.h"
#include "platform/aas/mw/com/impl/runtime_mock.h"

#include "platform/aas/lib/memory/shared/shared_memory_factory.h"
#include "platform/aas/lib/memory/shared/shared_memory_factory_mock.h"
#include "platform/aas/lib/memory/shared/shared_memory_resource_mock.h"
#include "platform/aas/lib/os/mocklib/fcntl_mock.h"

#include "platform/aas/lib/os/mocklib/stat_mock.h"
#include "platform/aas/lib/os/mocklib/unistdmock.h"
#include "platform/aas/lib/result/result.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <limits>
#include <string>
#include <utility>
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

using ::testing::_;
using ::testing::Expectation;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::StrEq;
using ::testing::StrictMock;
using ::testing::WithArg;

#ifdef __linux__
const std::string kSharedMemoryPathPrefix{"/dev/shm/lola-ctl-"};
#else
const std::string kSharedMemoryPathPrefix{"/dev/shmem/lola-ctl-"};
#endif

const std::string kDummyControlPath{"/test_control_path"};
const std::string kDummyDataPath{"/test_data_path"};
const std::string kDummyLockFilePath{"/test_lock_path"};

const auto service = make_ServiceIdentifierType("foo");
const ServiceTypeDeployment kServiceTypeDeployment{LolaServiceTypeDeployment{0x1234}};
const LolaServiceInstanceId kLolaServiceInstanceId{0x5678};
const auto kInstanceSpecifier = InstanceSpecifier::Create("abc/abc/TirePressurePort").value();
auto kServiceInstanceDeployment =
    ServiceInstanceDeployment{service,
                              LolaServiceInstanceDeployment{LolaServiceInstanceId{kLolaServiceInstanceId}},
                              QualityType::kASIL_QM,
                              kInstanceSpecifier};

using ProxyCreationFixture = ProxyMockedMemoryFixture;
TEST_F(ProxyCreationFixture, SharedMemoryFactoryOpenReturningNullPtrOnProxyCreationReturnsNullPtr)
{
    RecordProperty("Verifies", ", 2, 6");
    RecordProperty("Description",
                   "Checks that the Lola Proxy binding returns a nullptr when the shared memory cannot be opened.");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("Priority", "1");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    // Given a valid deployment information
    auto identifier = make_InstanceIdentifier(kServiceInstanceDeployment, kServiceTypeDeployment);

    // Expecting that the SharedMemoryFactory Open call will return a nullptr
    ON_CALL(shared_memory_factory_mock_guard_.mock_, Open(_, _, _)).WillByDefault(Return(nullptr));

    // When creating a proxy
    InitialiseProxyWithCreate(identifier);

    // Then the result will be a nullptr
    EXPECT_EQ(parent_, nullptr);
}

TEST_F(ProxyCreationFixture, ProxyCreationReturnsNullPtrWhenFailedToOpenUsageMarkerFile)
{
    // Given a valid deployment information
    auto identifier = make_InstanceIdentifier(kServiceInstanceDeployment, kServiceTypeDeployment);

    // if fails to open service instance usage marker file
    EXPECT_CALL(*fcntl_mock_, open(_, _, _))
        .WillOnce(::testing::Return(amp::make_unexpected(::bmw::os::Error::createFromErrno(EACCES))));

    // When creating a proxy
    const auto proxy_result = Proxy::Create(make_HandleType(identifier, ServiceInstanceId{kLolaServiceInstanceId}));

    // Then the result will be a nullptr
    EXPECT_EQ(proxy_result, nullptr);
}

TEST_F(ProxyCreationFixture, ProxyCreationReturnsNullPtrWhenSharedCannotBeAcquired)
{
    // Given a valid deployment information
    auto identifier = make_InstanceIdentifier(kServiceInstanceDeployment, kServiceTypeDeployment);

    // if fails to get shared lock on service instance usage marker file
    EXPECT_CALL(*fcntl_mock_, flock(_, _))
        .WillOnce(::testing::Return(amp::make_unexpected(::bmw::os::Error::createFromErrno(EWOULDBLOCK))));

    // When creating a proxy
    const auto proxy_result = Proxy::Create(make_HandleType(identifier, ServiceInstanceId{kLolaServiceInstanceId}));

    // Then the result will be a nullptr
    EXPECT_EQ(proxy_result, nullptr);
}

using ProxyCreationDeathFixture = ProxyCreationFixture;
TEST_F(ProxyCreationDeathFixture, GettingEventDataControlWithoutInitialisedEventDataControlTerminates)
{
    // Given a fake Skeleton which creates an empty ServiceDataControl

    // and a valid deployment information
    auto identifier = make_InstanceIdentifier(kServiceInstanceDeployment, kServiceTypeDeployment);

    // When creating a proxy
    InitialiseProxyWithConstructor(identifier);
    EXPECT_NE(parent_, nullptr);

    // Then trying to get the event data control for an event that was not registered in the ServiceDataStorage
    // Will terminate
    const ElementFqId uninitialised_element_fq_id{0xcdef, 0x5, 0x10, ElementType::EVENT};
    EXPECT_DEATH(parent_->GetEventControl(uninitialised_element_fq_id), ".*");
}

TEST_F(ProxyCreationDeathFixture, GettingRawDataStorageWithoutInitialisedEventDataStorageTerminates)
{
    // Given a fake Skeleton which creates an empty ServiceDataStorage

    // Given a valid deployment information
    auto identifier = make_InstanceIdentifier(kServiceInstanceDeployment, kServiceTypeDeployment);

    // When creating a proxy
    InitialiseProxyWithConstructor(identifier);
    EXPECT_NE(parent_, nullptr);

    // Then trying to get the event data storage for an event that was not registered in the ServiceDataStorage
    // Will terminate
    const ElementFqId uninitialised_element_fq_id{0xcdef, 0x5, 0x10, ElementType::EVENT};
    EXPECT_DEATH(parent_->GetRawDataStorage(uninitialised_element_fq_id), ".*");
}

using ProxyAutoReconnectFixture = ProxyMockedMemoryFixture;
TEST_F(ProxyAutoReconnectFixture, StartFindServiceIsCalledWhenProxyCreateSucceeds)
{
    const auto valid_find_service_handle = make_FindServiceHandle(10U);

    // Given a fake Skeleton which creates an empty ServiceDataStorage

    // Given a valid deployment information
    auto identifier = make_InstanceIdentifier(kServiceInstanceDeployment, kServiceTypeDeployment);

    // Expecting that StartFindService is called
    EXPECT_CALL(service_discovery_mock_, StartFindService(_, EnrichedInstanceIdentifier{identifier}))
        .WillOnce(Return(valid_find_service_handle));

    // When creating a proxy
    InitialiseProxyWithCreate(identifier);
    EXPECT_NE(parent_, nullptr);
}

TEST_F(ProxyAutoReconnectFixture, StartFindServiceIsNotCalledWhenProxyCreateFails)
{
    // Given a valid deployment information
    auto identifier = make_InstanceIdentifier(kServiceInstanceDeployment, kServiceTypeDeployment);

    // Expecting that the SharedMemoryFactory Open call will return a nullptr
    EXPECT_CALL(shared_memory_factory_mock_guard_.mock_, Open(_, _, _))
        .Times(2)
        .WillRepeatedly(::testing::Return(nullptr));

    // Then expecting that StartFindService will not be called
    EXPECT_CALL(service_discovery_mock_, StartFindService(_, EnrichedInstanceIdentifier{identifier})).Times(0);

    // When creating a proxy
    InitialiseProxyWithCreate(identifier);

    // and the result will be a nullptr
    EXPECT_EQ(parent_, nullptr);
}

TEST_F(ProxyAutoReconnectFixture, StopFindServiceIsCalledOnProxyDestruction)
{
    const auto valid_find_service_handle = make_FindServiceHandle(10U);

    // Given a fake Skeleton which creates an empty ServiceDataStorage

    // Given a valid deployment information
    auto identifier = make_InstanceIdentifier(kServiceInstanceDeployment, kServiceTypeDeployment);

    // Expecting that StartFindService is called
    ON_CALL(service_discovery_mock_, StartFindService(_, EnrichedInstanceIdentifier{identifier}))
        .WillByDefault(Return(valid_find_service_handle));

    // And that StopFindService is called on destruction of the Proxy
    EXPECT_CALL(service_discovery_mock_, StopFindService(valid_find_service_handle));

    // When creating a proxy
    InitialiseProxyWithConstructor(identifier);
    EXPECT_NE(parent_, nullptr);
}

TEST_F(ProxyAutoReconnectFixture, RegisterEventBindingCallsNotifyOnEventWithFalseWhenProviderInitiallyDoesNotExist)
{
    const auto valid_find_service_handle = make_FindServiceHandle(10U);
    mock_binding::ProxyEvent<std::uint8_t> proxy_event{};

    // Given a fake Skeleton which creates an empty ServiceDataStorage

    // Given a valid deployment information
    auto identifier = make_InstanceIdentifier(kServiceInstanceDeployment, kServiceTypeDeployment);

    // Expecting that StartFindService is called but the handler is not called since the provider does not exist
    ON_CALL(service_discovery_mock_, StartFindService(_, EnrichedInstanceIdentifier{identifier}))
        .WillByDefault(Return(valid_find_service_handle));

    // Then expecting that NotifyServiceInstanceChangedAvailability is called on the event with is_available false
    const bool is_available{false};
    EXPECT_CALL(proxy_event, NotifyServiceInstanceChangedAvailability(is_available, _));

    // When creating a proxy
    InitialiseProxyWithConstructor(identifier);
    EXPECT_NE(parent_, nullptr);

    // and the ProxyEvent registers itself with the Proxy
    parent_->RegisterEventBinding("Event0", proxy_event);
}

TEST_F(ProxyAutoReconnectFixture, RegisterEventBindingCallsNotifyOnEventWithTrueWhenProviderInitiallyExists)
{
    const auto valid_find_service_handle = make_FindServiceHandle(10U);
    mock_binding::ProxyEvent<std::uint8_t> proxy_event{};

    // Given a fake Skeleton which creates an empty ServiceDataStorage

    // Given a valid deployment information
    auto identifier = make_InstanceIdentifier(kServiceInstanceDeployment, kServiceTypeDeployment);

    // Expecting that StartFindService is called and synchronously calls handler since provider exists
    EXPECT_CALL(service_discovery_mock_, StartFindService(_, EnrichedInstanceIdentifier{identifier}))
        .WillOnce(WithArg<0>(Invoke([valid_find_service_handle, identifier](auto find_service_handler) noexcept {
            ServiceHandleContainer<HandleType> service_handle_container{make_HandleType(identifier)};
            find_service_handler(service_handle_container, valid_find_service_handle);
            return valid_find_service_handle;
        })));

    // and expecting that NotifyServiceInstanceChangedAvailability is called on the event with is_available true
    const bool is_available{true};
    EXPECT_CALL(proxy_event, NotifyServiceInstanceChangedAvailability(is_available, _));

    // When creating a proxy
    InitialiseProxyWithConstructor(identifier);
    EXPECT_NE(parent_, nullptr);

    // and the ProxyEvent registers itself with the Proxy
    parent_->RegisterEventBinding("Event0", proxy_event);
}

TEST_F(ProxyAutoReconnectFixture, RegisterEventBindingCallsNotifyOnEventWithLatestValueFromFindServiceHandler)
{
    const auto valid_find_service_handle = make_FindServiceHandle(10U);
    mock_binding::ProxyEvent<std::uint8_t> proxy_event_0{};
    mock_binding::ProxyEvent<std::uint8_t> proxy_event_1{};

    // Given a fake Skeleton which creates an empty ServiceDataStorage

    // Given a valid deployment information
    auto identifier = make_InstanceIdentifier(kServiceInstanceDeployment, kServiceTypeDeployment);

    // Expecting that StartFindService is called and synchronously calls handler since provider exists
    FindServiceHandler<HandleType> saved_find_service_handler{};
    EXPECT_CALL(service_discovery_mock_, StartFindService(_, EnrichedInstanceIdentifier{identifier}))
        .WillOnce(WithArg<0>(Invoke(
            [&saved_find_service_handler, valid_find_service_handle, identifier](auto find_service_handler) noexcept {
                ServiceHandleContainer<HandleType> service_handle_container{make_HandleType(identifier)};
                find_service_handler(service_handle_container, valid_find_service_handle);
                saved_find_service_handler = std::move(find_service_handler);
                return valid_find_service_handle;
            })));

    // Then expecting that NotifyServiceInstanceChangedAvailability is called on the first event with is_available true
    // during registration
    // Note. We use Expectations and .After instead of an InSequence test since the events are stored in an
    // unordered_map, so the final expectations on proxy_event_0 and proxy_event_1 can occur in any order, although
    // after all of the preceeding calls.
    const bool initial_is_available{true};
    Expectation event_0_initial_notify =
        EXPECT_CALL(proxy_event_0, NotifyServiceInstanceChangedAvailability(initial_is_available, _));

    // and then NotifyServiceInstanceChangedAvailability is called on the first event by the find service handler
    const bool first_find_service_is_available{false};
    Expectation event_0_find_service_notify =
        EXPECT_CALL(proxy_event_0, NotifyServiceInstanceChangedAvailability(first_find_service_is_available, _))
            .After(event_0_initial_notify);

    // Then expecting that NotifyServiceInstanceChangedAvailability is called on the second event with is_available
    // false during registration
    Expectation event_1_find_service_notify =
        EXPECT_CALL(proxy_event_1, NotifyServiceInstanceChangedAvailability(first_find_service_is_available, _))
            .After(event_0_initial_notify, event_0_find_service_notify);

    // and then NotifyServiceInstanceChangedAvailability is called on both events by the second find service handler
    const bool second_find_service_is_available{true};
    EXPECT_CALL(proxy_event_0, NotifyServiceInstanceChangedAvailability(second_find_service_is_available, _))
        .After(event_0_initial_notify, event_0_find_service_notify, event_1_find_service_notify);
    EXPECT_CALL(proxy_event_1, NotifyServiceInstanceChangedAvailability(second_find_service_is_available, _))
        .After(event_0_initial_notify, event_0_find_service_notify, event_1_find_service_notify);

    // When creating a proxy
    InitialiseProxyWithConstructor(identifier);
    EXPECT_NE(parent_, nullptr);

    // and the first ProxyEvent registers itself with the Proxy
    parent_->RegisterEventBinding("Event0", proxy_event_0);

    // And then the FindService handler is called with an empty service handle container
    ServiceHandleContainer<HandleType> empty_service_handle_container{};
    saved_find_service_handler(empty_service_handle_container, valid_find_service_handle);

    // and the second ProxyEvent registers itself with the Proxy
    parent_->RegisterEventBinding("Event1", proxy_event_1);

    // And then the FindService handler is called again with a non-empty service handle container
    ServiceHandleContainer<HandleType> filled_service_handle_container{make_HandleType(identifier)};
    saved_find_service_handler(filled_service_handle_container, valid_find_service_handle);
}

using ProxyAutoReconnectDeathFixture = ProxyAutoReconnectFixture;
TEST_F(ProxyAutoReconnectDeathFixture, ProxyCreateWillTerminateIfStartFindServiceReturnsError)
{
    auto start_find_service_fails = [this] {
        // Given a fake Skeleton which creates an empty ServiceDataStorage

        // Given a valid deployment information
        auto identifier = make_InstanceIdentifier(kServiceInstanceDeployment, kServiceTypeDeployment);

        // Expecting that StartFindService is called and returns an error
        EXPECT_CALL(service_discovery_mock_, StartFindService(_, EnrichedInstanceIdentifier{identifier}))
            .WillOnce(Return(MakeUnexpected(ComErrc::kServiceNotOffered)));

        // Then when creating a proxy
        // We terminate
        InitialiseProxyWithConstructor(identifier);
    };
    EXPECT_DEATH(start_find_service_fails(), ".*");
}

class ProxyUidPidRegistrationFixture : public ProxyMockedMemoryFixture
{
  protected:
    ProxyUidPidRegistrationFixture() noexcept {}

    void AddUidPidMapping(uid_t uid, pid_t pid) noexcept
    {
        auto result = fake_data_.data_control->uid_pid_mapping_.RegisterPid(uid, pid);
        ASSERT_TRUE(result.has_value());
        ASSERT_EQ(result.value(), pid);
    }

    const InstanceIdentifier instance_identifier_{
        make_InstanceIdentifier(kServiceInstanceDeployment, kServiceTypeDeployment)};
};

TEST_F(ProxyUidPidRegistrationFixture, NoOutdatedPidNotificationWillBeSent)
{
    // Given a fake Skeleton which sets up ServiceDataControl with an initial empty UidPidMapping

    // we expect that IMessagePassingService::NotifyOutdatedNodeId() will NOT get called!
    EXPECT_CALL(*mock_service_, NotifyOutdatedNodeId(_, _, _)).Times(0);

    // When creating a proxy
    InitialiseProxyWithCreate(instance_identifier_);
    EXPECT_NE(parent_, nullptr);
}

TEST_F(ProxyUidPidRegistrationFixture, OutdatedPidNotificationWillBeSent)
{
    uid_t our_uid{22};
    pid_t old_pid{1};
    pid_t new_pid{2};

    // Given a fake Skeleton which sets up ServiceDataControl with an UidPidMapping, which contains an "old pid" for
    // our uid
    AddUidPidMapping(our_uid, old_pid);

    // expect, that the Loa runtime return our_uid and new_pid, when asked for current uid/pid
    EXPECT_CALL(binding_runtime_, GetUid()).WillRepeatedly(Return(our_uid));
    EXPECT_CALL(binding_runtime_, GetPid()).WillRepeatedly(Return(new_pid));

    // we expect that IMessagePassingService::NotifyOutdatedNodeId() will get called to notify about an outdated pid!
    EXPECT_CALL(*mock_service_, NotifyOutdatedNodeId(_, old_pid, _)).Times(1);

    // When creating a proxy
    InitialiseProxyWithCreate(instance_identifier_);
    EXPECT_NE(parent_, nullptr);
}

class ProxyTransactionLogRollbackFixture : public ProxyMockedMemoryFixture
{
  protected:
    ProxyTransactionLogRollbackFixture() noexcept
    {
        InitialiseDummySkeletonEvent(element_fq_id_, SkeletonEventProperties{max_num_slots_, max_subscribers_, true});
    }

    TransactionLogId transaction_log_id_{kDummyUid};
    const InstanceIdentifier instance_identifier_{
        make_InstanceIdentifier(kServiceInstanceDeployment, kServiceTypeDeployment)};
    const ElementFqId element_fq_id_{0xcdef, 0x5, 0x10, ElementType::EVENT};
    const std::size_t max_num_slots_{5U};
    const std::uint8_t max_subscribers_{10U};
    //::bmw::os::UnistdMock unistd_mock_{};

    const TransactionLog::MaxSampleCountType subscription_max_sample_count_{5U};
};

TEST_F(ProxyTransactionLogRollbackFixture, RollbackWillBeCalledOnExistingTransactionLogOnCreation)
{
    // Given a fake Skeleton and SkeletonEvent which sets up an EventDataControl containing a TransactionLogSet

    // When inserting a TransactionLog into the created TransactionLogSet which contains valid transactions
    InsertProxyTransactionLogWithValidTransactions(
        *event_control_, subscription_max_sample_count_, transaction_log_id_);
    EXPECT_TRUE(IsProxyTransactionLogIdRegistered(*event_control_, transaction_log_id_));

    // When creating a proxy
    InitialiseProxyWithCreate(instance_identifier_);
    EXPECT_NE(parent_, nullptr);

    // Then the TransactionLog should be rollbacked during construction and removed
    EXPECT_FALSE(IsProxyTransactionLogIdRegistered(*event_control_, transaction_log_id_));
}

TEST_F(ProxyTransactionLogRollbackFixture, RollbackWillBeNotBeCalledOnNonExistingTransactionLogOnCreation)
{
    // Given a fake Skeleton and SkeletonEvent which sets up an EventDataControl containing a TransactionLogSet

    // When no TransactionLog exists in the created TransactionLogSet
    EXPECT_FALSE(IsProxyTransactionLogIdRegistered(*event_control_, transaction_log_id_));

    // Given a valid deployment information
    auto identifier = make_InstanceIdentifier(kServiceInstanceDeployment, kServiceTypeDeployment);

    // When creating a proxy with the same TransactionLogId
    InitialiseProxyWithCreate(instance_identifier_);
    EXPECT_NE(parent_, nullptr);

    // Then there should still be no transaction log and we shouldn't crash
    EXPECT_FALSE(IsProxyTransactionLogIdRegistered(*event_control_, transaction_log_id_));
}

TEST_F(ProxyTransactionLogRollbackFixture, FailureInRollingBackExistingTransactionLogWillReturnEmptyProxyBinding)
{
    RecordProperty("Verifies", "2");
    RecordProperty(
        "Description",
        "error, which is represented by a nullptr, shall be returned if a transaction rollback is not possible.");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("Priority", "1");
    RecordProperty("DerivationTechnique", "Analysis of requirements");
    // Given a fake Skeleton and SkeletonEvent which sets up an EventDataControl containing a TransactionLogSet

    // When inserting a TransactionLog into the created TransactionLogSet which contains invalid transactions
    InsertProxyTransactionLogWithInvalidTransactions(
        *event_control_, subscription_max_sample_count_, transaction_log_id_);
    EXPECT_TRUE(IsProxyTransactionLogIdRegistered(*event_control_, transaction_log_id_));

    // Given a valid deployment information
    auto identifier = make_InstanceIdentifier(kServiceInstanceDeployment, kServiceTypeDeployment);

    // When creating a proxy
    InitialiseProxyWithCreate(instance_identifier_);

    // Then the Proxy binding will not be created.
    EXPECT_EQ(parent_, nullptr);
}

}  // namespace
}  // namespace lola
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
