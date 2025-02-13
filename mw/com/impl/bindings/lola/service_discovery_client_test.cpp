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


#include "platform/aas/mw/com/impl/bindings/lola/service_discovery_client.h"

#include "platform/aas/lib/concurrency/executor_mock.h"
#include "platform/aas/lib/concurrency/long_running_threads_container.h"
#include "platform/aas/lib/filesystem/factory/filesystem_factory_fake.h"
#include "platform/aas/lib/filesystem/filesystem.h"
#include "platform/aas/lib/os/utils/inotify/inotify_instance.h"
#include "platform/aas/lib/os/utils/inotify/inotify_instance_facade.h"
#include "platform/aas/lib/os/utils/inotify/inotify_instance_mock.h"

#include "platform/aas/lib/os/utils/inotify/inotify_watch_descriptor.h"
#include "platform/aas/mw/com/impl/handle_type.h"
#include "platform/aas/mw/com/impl/i_service_discovery.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <unistd.h>
#include <atomic>
#include <cstdint>
#include <memory>
#include <string>

namespace bmw::mw::com::impl::lola
{
namespace
{

using ::testing::_;
using ::testing::AtLeast;
using ::testing::ByMove;
using ::testing::Contains;
using ::testing::DoDefault;
using ::testing::Eq;
using ::testing::InSequence;
using ::testing::Invoke;
using ::testing::InvokeWithoutArgs;
using ::testing::MockFunction;
using ::testing::Return;
using ::testing::StrictMock;
using ::testing::WithArg;

#ifdef __QNXNTO__
const filesystem::Path kTmpPath{"/tmp_discovery/mw_com_lola/service_discovery"};
#else
const filesystem::Path kTmpPath{"/tmp/mw_com_lola/service_discovery"};
#endif

const auto kOldFlagFileDirectory = kTmpPath / "1/1";
const auto kOldFlagFile = kOldFlagFileDirectory / "123456_asil-qm_1234";

constexpr auto kQmPathLabel{"asil-qm"};
constexpr auto kAsilBPathLabel{"asil-b"};

InstanceSpecifier kInstanceSpecifier{InstanceSpecifier::Create("/bla/blub/specifier").value()};
LolaServiceTypeDeployment kServiceId{1U};
LolaServiceInstanceId kInstanceId1{1U};
LolaServiceInstanceId kInstanceId2{2U};
LolaServiceInstanceId kInstanceId3{3U};

ServiceTypeDeployment kServiceTypeDeployment{kServiceId};
ServiceInstanceDeployment kInstanceDeployment1{make_ServiceIdentifierType("/bla/blub/service1"),
                                               LolaServiceInstanceDeployment{kInstanceId1},
                                               QualityType::kASIL_QM,
                                               kInstanceSpecifier};
ServiceInstanceDeployment kInstanceDeployment2{make_ServiceIdentifierType("/bla/blub/service1"),
                                               LolaServiceInstanceDeployment{kInstanceId2},
                                               QualityType::kASIL_QM,
                                               kInstanceSpecifier};
ServiceInstanceDeployment kInstanceDeployment3{make_ServiceIdentifierType("/bla/blub/service1"),
                                               LolaServiceInstanceDeployment{kInstanceId3},
                                               QualityType::kASIL_B,
                                               kInstanceSpecifier};
ServiceInstanceDeployment kInstanceDeploymentAny{make_ServiceIdentifierType("/bla/blub/service1"),
                                                 LolaServiceInstanceDeployment{},
                                                 QualityType::kASIL_QM,
                                                 kInstanceSpecifier};
InstanceIdentifier kInstanceIdentifier1{make_InstanceIdentifier(kInstanceDeployment1, kServiceTypeDeployment)};
InstanceIdentifier kInstanceIdentifier2{make_InstanceIdentifier(kInstanceDeployment2, kServiceTypeDeployment)};
InstanceIdentifier kInstanceIdentifier3{make_InstanceIdentifier(kInstanceDeployment3, kServiceTypeDeployment)};
InstanceIdentifier kInstanceIdentifierAny{make_InstanceIdentifier(kInstanceDeploymentAny, kServiceTypeDeployment)};
HandleType kHandle1{make_HandleType(kInstanceIdentifier1)};
HandleType kHandle2{make_HandleType(kInstanceIdentifier2)};
HandleType kHandle3{make_HandleType(kInstanceIdentifier3)};
HandleType kHandleFindAny1{make_HandleType(kInstanceIdentifierAny, kInstanceId1)};
HandleType kHandleFindAny2{make_HandleType(kInstanceIdentifierAny, kInstanceId2)};
HandleType kHandleFindAny3{make_HandleType(kInstanceIdentifierAny, kInstanceId3)};

constexpr auto kAllPermissions = filesystem::Perms::kReadWriteExecUser | filesystem::Perms::kReadWriteExecGroup |
                                 filesystem::Perms::kReadWriteExecOthers;

// Generates the file path to the service ID directory (which contains the instance ID)
filesystem::Path GenerateExpectedServiceDirectoryPath(const LolaServiceId service_id)
{
    return kTmpPath / std::to_string(static_cast<std::uint32_t>(service_id));
}

// Generates the file path to the instance ID directory (which contains the flag files)
filesystem::Path GenerateExpectedInstanceDirectoryPath(const LolaServiceId service_id,
                                                       const LolaServiceInstanceId::InstanceId instance_id)
{
    return GenerateExpectedServiceDirectoryPath(service_id) / std::to_string(static_cast<std::uint32_t>(instance_id));
}

// Creates an amp::callback wrapper which dispatches to a pointer to a MockFunction. We do this since a MockFunction
// does not fit inside an amp::callback with default capacity.
amp::callback<void(ServiceHandleContainer<HandleType>, FindServiceHandle)> CreateWrappedMockFindServiceHandler(
    MockFunction<void(ServiceHandleContainer<HandleType>, FindServiceHandle)>& mock_find_service_handler)
{
    return
        [&mock_find_service_handler](ServiceHandleContainer<HandleType> containers, FindServiceHandle handle) noexcept {
            mock_find_service_handler.AsStdFunction()(containers, handle);
        };
}

class FileSystemGuard
{
  public:
    explicit FileSystemGuard(filesystem::Filesystem& filesystem) noexcept : filesystem_{filesystem} {}
    ~FileSystemGuard() noexcept { filesystem_.standard->RemoveAll(kTmpPath); }

  private:
    filesystem::Filesystem& filesystem_;
};

class DestructorNotifier
{
  public:
    explicit DestructorNotifier(std::promise<void>* handler_destruction_barrier) noexcept
        : handler_destruction_barrier_{handler_destruction_barrier}
    {
    }
    ~DestructorNotifier() noexcept
    {
        if (handler_destruction_barrier_ != nullptr)
        {
            handler_destruction_barrier_->set_value();
        }
    }

    DestructorNotifier(DestructorNotifier&& other) noexcept
        : handler_destruction_barrier_{other.handler_destruction_barrier_}
    {
        other.handler_destruction_barrier_ = nullptr;
    }

    DestructorNotifier(const DestructorNotifier&) = delete;
    DestructorNotifier& operator=(const DestructorNotifier&) = delete;
    DestructorNotifier& operator=(DestructorNotifier&&) = delete;

  private:
    std::promise<void>* handler_destruction_barrier_;
};

class ServiceDiscoveryClientFixture : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        unistd_ = std::make_unique<os::internal::UnistdImpl>();
        inotify_instance_ = std::make_unique<os::InotifyInstanceImpl>();
        ASSERT_TRUE(inotify_instance_->IsValid());

        ON_CALL(inotify_instance_mock_, IsValid()).WillByDefault([this]() { return inotify_instance_->IsValid(); });
        ON_CALL(inotify_instance_mock_, Close()).WillByDefault([this]() { return inotify_instance_->Close(); });
        ON_CALL(inotify_instance_mock_, AddWatch(_, _)).WillByDefault([this](auto path, auto mask) {
            return inotify_instance_->AddWatch(path, mask);
        });
        ON_CALL(inotify_instance_mock_, RemoveWatch(_)).WillByDefault([this](auto watch_descriptor) {
            return inotify_instance_->RemoveWatch(watch_descriptor);
        });
        ON_CALL(inotify_instance_mock_, Read()).WillByDefault([this]() { return inotify_instance_->Read(); });
    }

    void TearDown() override { filesystem::StandardFilesystem::restore_instance(); }

    ServiceDiscoveryClient CreateAServiceDiscoveryClient()
    {
        auto inotify_instance_facade = std::make_unique<os::InotifyInstanceFacade>(inotify_instance_mock_);
        return ServiceDiscoveryClient{
            long_running_threads_container_, std::move(inotify_instance_facade), std::move(unistd_), filesystem_};
    }

    filesystem::Path GetFlagFilePrefix(const LolaServiceId service_id, const LolaServiceInstanceId instance_id) noexcept
    {
        const auto service_id_str = std::to_string(static_cast<std::uint32_t>(service_id));
        const auto instance_id_str = std::to_string(static_cast<std::uint32_t>(instance_id.id_));
        const auto pid = std::to_string(os::internal::UnistdImpl{}.getpid());
        return kTmpPath / service_id_str / instance_id_str / pid;
    }

    void CreateRegularFile(filesystem::Filesystem& filesystem, const filesystem::Path& path) noexcept
    {
        ASSERT_TRUE(filesystem.utils->CreateDirectories(path.ParentPath(), kAllPermissions).has_value());
        ASSERT_TRUE(filesystem.streams->Open(path, std::ios_base::out).has_value());
    }

    filesystem::Filesystem filesystem_{filesystem::FilesystemFactory{}.CreateInstance()};
    FileSystemGuard filesystem_guard_{filesystem_};
    std::unique_ptr<os::Unistd> unistd_{};

    std::unique_ptr<os::InotifyInstanceImpl> inotify_instance_{};
    ::testing::NiceMock<os::InotifyInstanceMock> inotify_instance_mock_{};

    concurrency::LongRunningThreadsContainer long_running_threads_container_{};
};

TEST_F(ServiceDiscoveryClientFixture, CanConstructFixture) {}

class ServiceDiscoveryClientWithFakeFileSystemFixture : public ServiceDiscoveryClientFixture
{
  public:
    void SetUp() override
    {
        ServiceDiscoveryClientFixture::SetUp();
        CreateFakeFilesystem();
    }

    void TearDown() override { ServiceDiscoveryClientFixture::TearDown(); }

    void CreateFakeFilesystem()
    {
        filesystem::FilesystemFactoryFake filesystem_factory_fake{};
        filesystem_mock_ = filesystem_factory_fake.CreateInstance();
        standard_filesystem_fake_ = &(filesystem_factory_fake.GetStandard());
        file_factory_mock_ = &(filesystem_factory_fake.GetStreams());
        file_utils_mock_ = &(filesystem_factory_fake.GetUtils());
        filesystem::StandardFilesystem::set_testing_instance(*standard_filesystem_fake_);
    }

    ServiceDiscoveryClient CreateAServiceDiscoveryClient()
    {
        auto inotify_instance_facade = std::make_unique<os::InotifyInstanceFacade>(inotify_instance_mock_);
        return ServiceDiscoveryClient{
            long_running_threads_container_, std::move(inotify_instance_facade), std::move(unistd_), filesystem_mock_};
    }

    ServiceDiscoveryClientWithFakeFileSystemFixture& SaveTheFlagFilePath()
    {
        GetFlagFilePath(flag_file_path_);
        return *this;
    }

    void GetFlagFilePath(std::list<filesystem::Path>& flag_file_path) noexcept
    {
        GetFlagFilePath(flag_file_path, [](const auto&, const auto&) noexcept {});
    }

    template <typename Callable>
    void GetFlagFilePath(std::list<filesystem::Path>& flag_file_path, Callable callable) noexcept
    {
        ON_CALL(*file_factory_mock_, Open(_, _))
            .WillByDefault([this, &flag_file_path, callable](const auto& path, const auto& mode) {
                flag_file_path.push_back(path);

                callable(path, mode);

                return filesystem::FileFactoryFake{*standard_filesystem_fake_}.Open(path, mode);
            });
    }

    std::list<filesystem::Path> flag_file_path_{};
    filesystem::Filesystem filesystem_mock_{};
    filesystem::StandardFilesystemFake* standard_filesystem_fake_{};
    filesystem::FileFactoryMock* file_factory_mock_{};
    filesystem::FileUtilsMock* file_utils_mock_{};
};

TEST_F(ServiceDiscoveryClientWithFakeFileSystemFixture, CreatesFlagFileOnAsilQmServiceOffer)
{
    SaveTheFlagFilePath();
    auto service_discovery_client = CreateAServiceDiscoveryClient();

    ASSERT_TRUE(service_discovery_client.OfferService(kInstanceIdentifier1).has_value());

    ASSERT_EQ(flag_file_path_.size(), 1);
    EXPECT_NE(flag_file_path_.front().Native().find(kQmPathLabel), std::string::npos);
    EXPECT_TRUE(filesystem_mock_.standard->Exists(flag_file_path_.front()).value());
}

TEST_F(ServiceDiscoveryClientWithFakeFileSystemFixture, CreatesFlagFilesOnAsilBServiceOffer)
{
    SaveTheFlagFilePath();
    auto service_discovery_client = CreateAServiceDiscoveryClient();

    ASSERT_TRUE(service_discovery_client.OfferService(kInstanceIdentifier3).has_value());

    ASSERT_EQ(flag_file_path_.size(), 2);
    EXPECT_NE(flag_file_path_.front().Native().find(kAsilBPathLabel), std::string::npos);
    EXPECT_TRUE(filesystem_mock_.standard->Exists(flag_file_path_.front()).value());
    EXPECT_NE(flag_file_path_.back().Native().find(kQmPathLabel), std::string::npos);
    EXPECT_TRUE(filesystem_mock_.standard->Exists(flag_file_path_.back()).value());
}

TEST_F(ServiceDiscoveryClientWithFakeFileSystemFixture, QMFlagFilePathIsMappedFromQMInstanceIdentifier)
{
    RecordProperty("Verifies", "0");
    RecordProperty("Description",
                   "Checks that the QM flag file path is derived from the corresponding QM instance identifier.");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("Priority", "1");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    // Given a ServiceDiscoveryClient which saves the generated flag file path
    SaveTheFlagFilePath();
    auto service_discovery_client = CreateAServiceDiscoveryClient();

    // When offering the service
    amp::ignore = service_discovery_client.OfferService(kInstanceIdentifier1);

    // Then the generated QM flag file path should match the expected pattern
    const auto expected_instance_directory_path =
        GenerateExpectedInstanceDirectoryPath(kServiceId.service_id_, kInstanceId1.id_).Native();

    ASSERT_EQ(flag_file_path_.size(), 1);
    EXPECT_EQ(flag_file_path_.front().Native().find(expected_instance_directory_path), 0);
    EXPECT_NE(flag_file_path_.front().Native().find(kQmPathLabel), std::string::npos);
}

TEST_F(ServiceDiscoveryClientWithFakeFileSystemFixture, AsilBFlagFilePathIsMappedFromAsilBInstanceIdentifier)
{
    RecordProperty("Verifies", "0");
    RecordProperty(
        "Description",
        "Checks that the ASIL B flag file path is derived from the corresponding ASIL B instance identifier.");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("Priority", "1");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    // Given a ServiceDiscoveryClient which saves the generated flag file path
    SaveTheFlagFilePath();
    auto service_discovery_client = CreateAServiceDiscoveryClient();

    // When offering the service
    amp::ignore = service_discovery_client.OfferService(kInstanceIdentifier3);

    // Then the generated ASIL-B flag file path should match the expected pattern
    const std::string expected_instance_directory_path =
        GenerateExpectedInstanceDirectoryPath(kServiceId.service_id_, kInstanceId3.id_);

    ASSERT_EQ(flag_file_path_.size(), 2);
    EXPECT_EQ(flag_file_path_.front().Native().find(expected_instance_directory_path), 0);
    EXPECT_NE(flag_file_path_.front().Native().find(kAsilBPathLabel), std::string::npos);
}

TEST_F(ServiceDiscoveryClientWithFakeFileSystemFixture, QMFlagFilePathIsMappedFromAsilBInstanceIdentifier)
{
    RecordProperty("Verifies", "0");
    RecordProperty("Description",
                   "Checks that the QM flag file path is derived from the corresponding ASIL B instance identifier.");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("Priority", "1");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    // Given a ServiceDiscoveryClient which saves the generated flag file path
    SaveTheFlagFilePath();
    auto service_discovery_client = CreateAServiceDiscoveryClient();

    // When offering the service
    amp::ignore = service_discovery_client.OfferService(kInstanceIdentifier3);

    // Then the generated QM flag file path should match the expected pattern
    const std::string expected_instance_directory_path =
        GenerateExpectedInstanceDirectoryPath(kServiceId.service_id_, kInstanceId3.id_);

    ASSERT_EQ(flag_file_path_.size(), 2);
    EXPECT_EQ(flag_file_path_.back().Native().find(expected_instance_directory_path), 0);
    EXPECT_NE(flag_file_path_.back().Native().find(kQmPathLabel), std::string::npos);
}

TEST_F(ServiceDiscoveryClientWithFakeFileSystemFixture, TwoConsecutiveFlagFilesHaveDifferentName)
{
    SaveTheFlagFilePath();
    auto service_discovery_client = CreateAServiceDiscoveryClient();

    ASSERT_TRUE(service_discovery_client.OfferService(kInstanceIdentifier1).has_value());
    ASSERT_TRUE(
        service_discovery_client.StopOfferService(kInstanceIdentifier1, IServiceDiscovery::QualityTypeSelector::kBoth)
            .has_value());
    const auto first_flag_file_path = flag_file_path_;

    ASSERT_TRUE(service_discovery_client.OfferService(kInstanceIdentifier1).has_value());

    ASSERT_EQ(flag_file_path_.size(), 2);
    EXPECT_NE(flag_file_path_.front(), flag_file_path_.back());
}

TEST_F(ServiceDiscoveryClientWithFakeFileSystemFixture, OfferServiceReturnsErrorIfFlagFileCannotBeCreated)
{
    ON_CALL(*file_factory_mock_, Open(_, _))
        .WillByDefault(Return(ByMove(
            bmw::MakeUnexpected<std::unique_ptr<std::iostream>>(filesystem::ErrorCode::kCouldNotOpenFileStream))));

    auto service_discovery_client = CreateAServiceDiscoveryClient();

    const auto result = service_discovery_client.OfferService(kInstanceIdentifier1);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ComErrc::kServiceNotOffered);
}

TEST_F(ServiceDiscoveryClientWithFakeFileSystemFixture, OfferServiceRemovesOldFlagFilesInTheSearchPath)
{
    CreateRegularFile(filesystem_mock_, kOldFlagFile);

    SaveTheFlagFilePath();
    auto service_discovery_client = CreateAServiceDiscoveryClient();

    EXPECT_TRUE(standard_filesystem_fake_->Exists(kOldFlagFile).value());
    EXPECT_TRUE(service_discovery_client.OfferService(kInstanceIdentifier1).has_value());
    EXPECT_FALSE(standard_filesystem_fake_->Exists(kOldFlagFile).value());
    EXPECT_TRUE(standard_filesystem_fake_->Exists(flag_file_path_.front()).value());
}

TEST_F(ServiceDiscoveryClientWithFakeFileSystemFixture, RemovesFlagFileOnStopServiceOffer)
{
    SaveTheFlagFilePath();
    auto service_discovery_client = CreateAServiceDiscoveryClient();

    ASSERT_TRUE(service_discovery_client.OfferService(kInstanceIdentifier1).has_value());
    ASSERT_TRUE(
        service_discovery_client.StopOfferService(kInstanceIdentifier1, IServiceDiscovery::QualityTypeSelector::kBoth)
            .has_value());

    EXPECT_FALSE(filesystem_mock_.standard->Exists(flag_file_path_.front()).value());
}

TEST_F(ServiceDiscoveryClientWithFakeFileSystemFixture, RemovesQmFlagFileOnSelectiveStopServiceOffer)
{
    SaveTheFlagFilePath();
    auto service_discovery_client = CreateAServiceDiscoveryClient();

    ASSERT_TRUE(service_discovery_client.OfferService(kInstanceIdentifier3).has_value());
    ASSERT_TRUE(
        service_discovery_client.StopOfferService(kInstanceIdentifier3, IServiceDiscovery::QualityTypeSelector::kAsilQm)
            .has_value());

    EXPECT_TRUE(filesystem_mock_.standard->Exists(flag_file_path_.front()).value());
    EXPECT_NE(flag_file_path_.back().Native().find(kQmPathLabel), std::string::npos);
    EXPECT_FALSE(filesystem_mock_.standard->Exists(flag_file_path_.back()).value());
}

TEST_F(ServiceDiscoveryClientFixture, CallingStartFindServiceReturnsValidResult)
{
    // Given a ServiceDiscoveryClient
    auto service_discovery_client = CreateAServiceDiscoveryClient();

    // When calling StartFindService with an InstanceIdentifier with a specified instance ID
    FindServiceHandle handle{make_FindServiceHandle(1U)};
    const auto start_find_service_result = service_discovery_client.StartFindService(
        handle, [](auto, auto) noexcept {}, EnrichedInstanceIdentifier{kInstanceIdentifier1});

    // Then the result is valid
    EXPECT_TRUE(start_find_service_result.has_value());
}

TEST_F(ServiceDiscoveryClientFixture, CallingStartFindServiceForAnyInstanceIdsReturnsValidResult)
{
    // Given a ServiceDiscoveryClient
    auto service_discovery_client = CreateAServiceDiscoveryClient();

    // When calling StartFindService with an InstanceIdentifier without a specified instance ID
    FindServiceHandle handle{make_FindServiceHandle(1U)};
    const auto start_find_service_result = service_discovery_client.StartFindService(
        handle, [](auto, auto) noexcept {}, EnrichedInstanceIdentifier{kInstanceIdentifierAny});

    // Then the result is valid
    EXPECT_TRUE(start_find_service_result.has_value());
}

TEST_F(ServiceDiscoveryClientFixture, CallingStartFindServiceAddsWatchToInstancePath)
{
    RecordProperty("Verifies", "5");
    RecordProperty(
        "Description",
        "Checks that calling StartFindService with an InstanceIdentifier with a specified instance ID, then a watch "
        "will be added to the instance path (i.e. the search path that includes the service ID and instance ID).");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("Priority", "1");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    // Given a ServiceDiscoveryClient which saves the generated flag file path
    auto service_discovery_client = CreateAServiceDiscoveryClient();

    // Expecting that a watch is added to the instance path
    const auto expected_instance_directory_path =
        GenerateExpectedInstanceDirectoryPath(kServiceId.service_id_, kInstanceId1.id_).Native();
    const amp::string_view expected_instance_directory_path_view{expected_instance_directory_path.data(),
                                                                 expected_instance_directory_path.size()};
    EXPECT_CALL(inotify_instance_mock_, AddWatch(expected_instance_directory_path_view, _));

    // When calling StartFindService with an InstanceIdentifier with a specified instance ID
    FindServiceHandle handle{make_FindServiceHandle(1U)};
    amp::ignore = service_discovery_client.StartFindService(
        handle, [](auto, auto) noexcept {}, EnrichedInstanceIdentifier{kInstanceIdentifier1});
}

TEST_F(ServiceDiscoveryClientFixture, CallingStartFindServiceForAnyInstanceIdsAddsWatchToServicePath)
{
    RecordProperty("Verifies", "5");
    RecordProperty(
        "Description",
        "Checks that calling StartFindService with an InstanceIdentifier without a specified instance ID, then a watch "
        "will be added to the servce path (i.e. the search path that includes the service ID).");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("Priority", "1");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    // Given a ServiceDiscoveryClient which saves the generated flag file path
    auto service_discovery_client = CreateAServiceDiscoveryClient();

    // Expecting that a watch is added to the service path
    const auto expected_service_directory_path = GenerateExpectedServiceDirectoryPath(kServiceId.service_id_).Native();
    const amp::string_view expected_service_directory_path_view{expected_service_directory_path.data(),
                                                                expected_service_directory_path.size()};
    EXPECT_CALL(inotify_instance_mock_, AddWatch(expected_service_directory_path_view, _));

    // When calling StartFindService with an InstanceIdentifier without a specified instance ID
    FindServiceHandle handle{make_FindServiceHandle(1U)};
    amp::ignore = service_discovery_client.StartFindService(
        handle, [](auto, auto) noexcept {}, EnrichedInstanceIdentifier{kInstanceIdentifierAny});
}

TEST_F(ServiceDiscoveryClientFixture, StartsReadingInotifyInstanceOnConstruction)
{
    std::promise<void> barrier{};
    EXPECT_CALL(inotify_instance_mock_, Read())
        .WillOnce([&barrier]() {
            barrier.set_value();
            return amp::static_vector<os::InotifyEvent, os::InotifyInstance::max_events>{};
        })
        .WillRepeatedly([]() { return amp::static_vector<os::InotifyEvent, os::InotifyInstance::max_events>{}; });

    auto service_discovery_client = CreateAServiceDiscoveryClient();

    barrier.get_future().wait();
}

TEST_F(ServiceDiscoveryClientFixture, ClosesInotifyInstanceOnDestructionToUnblockWorker)
{
    std::promise<void> barrier{};
    EXPECT_CALL(inotify_instance_mock_, Close());

    EXPECT_CALL(inotify_instance_mock_, Read())
        .WillOnce([&barrier, this]() {
            barrier.set_value();
            return inotify_instance_->Read();
        })
        .WillRepeatedly(DoDefault());

    auto service_discovery_client = CreateAServiceDiscoveryClient();
    barrier.get_future().wait();
}

TEST_F(ServiceDiscoveryClientFixture, AddsWatchOnStartFindServiceWhileWorkerThreadIsBlockedOnRead)
{
    std::promise<void> first_barrier{};
    std::promise<void> second_barrier{};
    EXPECT_CALL(inotify_instance_mock_, Read())
        .WillOnce([&first_barrier, &second_barrier]() {
            first_barrier.set_value();
            second_barrier.get_future().wait();
            return amp::static_vector<os::InotifyEvent, os::InotifyInstance::max_events>{};
        })
        .WillRepeatedly([]() { return amp::static_vector<os::InotifyEvent, os::InotifyInstance::max_events>{}; });

    EXPECT_CALL(inotify_instance_mock_, AddWatch(_, _)).WillOnce([&second_barrier](auto, auto) {
        second_barrier.set_value();
        return os::InotifyWatchDescriptor{1};
    });

    auto service_discovery_client = CreateAServiceDiscoveryClient();

    first_barrier.get_future().wait();

    FindServiceHandle handle{make_FindServiceHandle(1U)};
    EXPECT_TRUE(service_discovery_client
                    .StartFindService(
                        handle, [](auto, auto) noexcept {}, EnrichedInstanceIdentifier{kInstanceIdentifier1})
                    .has_value());
}

TEST_F(ServiceDiscoveryClientFixture, AddsNoWatchOnFindService)
{
    // Expecting that _no_ watches are added
    EXPECT_CALL(inotify_instance_mock_, AddWatch(_, _)).Times(0);

    // Given a ServiceDiscovery client which offers a service
    auto service_discovery_client = CreateAServiceDiscoveryClient();
    EXPECT_TRUE(service_discovery_client.OfferService(kInstanceIdentifier1).has_value());

    // When finding a services as one shot
    const auto find_service_result =
        service_discovery_client.FindService(EnrichedInstanceIdentifier{kInstanceIdentifier1});

    // Then still a service is found
    ASSERT_TRUE(find_service_result.has_value());
    ASSERT_EQ(find_service_result.value().size(), 1);
    EXPECT_EQ(find_service_result.value()[0], kHandle1);
}

TEST_F(ServiceDiscoveryClientFixture, BailsOutOnInotifyQueueOverflow)
{
    amp::static_vector<os::InotifyEvent, os::InotifyInstance::max_events> event_vector{};
    ::inotify_event event{};
    event.mask = IN_Q_OVERFLOW;
    event_vector.emplace_back(event);

    ON_CALL(inotify_instance_mock_, Read()).WillByDefault(Return(event_vector));

    const auto test_function = [this] {
        auto inotify_instance_facade = std::make_unique<os::InotifyInstanceFacade>(inotify_instance_mock_);
        const ServiceDiscoveryClient service_discovery_client{
            long_running_threads_container_, std::move(inotify_instance_facade), std::move(unistd_), filesystem_};
        // We expect to die in an async thread - so a timeout is fine to violate the test if we do not die.
        std::this_thread::sleep_for(std::chrono::hours{1});
    };

    EXPECT_DEATH(test_function(), ".*");
}

TEST_F(ServiceDiscoveryClientFixture, CallsHandlerIfServiceInstanceAppearedBeforeSearchStarted)
{
    std::promise<void> barrier{};

    auto service_discovery_client = CreateAServiceDiscoveryClient();

    EXPECT_TRUE(service_discovery_client.OfferService(kInstanceIdentifier1).has_value());

    FindServiceHandle expected_handle{make_FindServiceHandle(1U)};
    const auto start_find_service_result = service_discovery_client.StartFindService(
        expected_handle,
        [&barrier, &expected_handle](auto container, auto handle) noexcept {
            EXPECT_EQ(container.size(), 1);
            EXPECT_EQ(container.front(), kHandle1);
            EXPECT_EQ(handle, expected_handle);
            barrier.set_value();
        },
        EnrichedInstanceIdentifier{kInstanceIdentifier1});
    EXPECT_TRUE(start_find_service_result.has_value());
    barrier.get_future().wait();
}

TEST_F(ServiceDiscoveryClientFixture, CallsHandlerIfServiceInstanceAppearsAfterSearchStarted)
{
    std::promise<void> service_found_barrier{};

    auto service_discovery_client = CreateAServiceDiscoveryClient();

    FindServiceHandle expected_handle{make_FindServiceHandle(1U)};
    const auto start_find_service_result = service_discovery_client.StartFindService(
        expected_handle,
        [&service_found_barrier, &expected_handle](auto container, auto handle) noexcept {
            EXPECT_EQ(container.size(), 1);
            EXPECT_EQ(container.front(), kHandle1);
            EXPECT_EQ(handle, expected_handle);
            service_found_barrier.set_value();
        },
        EnrichedInstanceIdentifier{kInstanceIdentifier1});
    EXPECT_TRUE(start_find_service_result.has_value());

    EXPECT_TRUE(service_discovery_client.OfferService(kInstanceIdentifier1).has_value());
    service_found_barrier.get_future().wait();
}

TEST_F(ServiceDiscoveryClientFixture, FindServiceReturnHandleIfServiceFound)
{
    auto service_discovery_client = CreateAServiceDiscoveryClient();
    EXPECT_TRUE(service_discovery_client.OfferService(kInstanceIdentifier1).has_value());

    const auto find_service_result =
        service_discovery_client.FindService(EnrichedInstanceIdentifier{kInstanceIdentifier1});
    EXPECT_TRUE(find_service_result.has_value());
    EXPECT_EQ(find_service_result.value().size(), 1);
    EXPECT_EQ(find_service_result.value()[0], kHandle1);
}

TEST_F(ServiceDiscoveryClientFixture, FindServiceReturnHandlesForAny)
{
    auto service_discovery_client = CreateAServiceDiscoveryClient();

    // Given that two services are offered
    ASSERT_TRUE(service_discovery_client.OfferService(kInstanceIdentifier1).has_value());
    ASSERT_TRUE(service_discovery_client.OfferService(kInstanceIdentifier2).has_value());

    // When finding services one shot with ANY
    const auto find_service_result =
        service_discovery_client.FindService(EnrichedInstanceIdentifier{kInstanceIdentifierAny});

    // Then two services are found
    ASSERT_TRUE(find_service_result.has_value());
    EXPECT_EQ(find_service_result.value().size(), 2);
    EXPECT_THAT(find_service_result.value(), ::testing::Contains(kHandleFindAny1));
    EXPECT_THAT(find_service_result.value(), ::testing::Contains(kHandleFindAny2));
}

TEST_F(ServiceDiscoveryClientFixture, FindServiceReturnNoHandleIfServiceNotFound)
{
    auto service_discovery_client = CreateAServiceDiscoveryClient();

    const auto find_service_result =
        service_discovery_client.FindService(EnrichedInstanceIdentifier{kInstanceIdentifier1});
    EXPECT_TRUE(find_service_result.has_value());
    EXPECT_EQ(find_service_result.value().size(), 0);
}

TEST_F(ServiceDiscoveryClientFixture, CallsCorrectHandlerForDifferentInstanceIDs)
{
    RecordProperty("Verifies", "4");
    RecordProperty(
        "Description",
        "Checks that a service is only visible to consumers (i.e. to StartFindService) after OfferService is called.");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("Priority", "1");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    std::promise<void> service_found_barrier_1{};
    std::promise<void> service_found_barrier_2{};

    std::atomic<bool> handler_received_1{false};
    std::atomic<bool> handler_received_2{false};

    auto service_discovery_client = CreateAServiceDiscoveryClient();

    FindServiceHandle expected_handle_1{make_FindServiceHandle(1U)};
    const auto start_find_service_result = service_discovery_client.StartFindService(
        expected_handle_1,
        [&service_found_barrier_1, &expected_handle_1, &handler_received_1](auto container, auto handle) noexcept {
            EXPECT_EQ(container.size(), 1);
            EXPECT_EQ(container.front(), kHandle1);
            EXPECT_EQ(handle, expected_handle_1);
            handler_received_1.store(true);
            service_found_barrier_1.set_value();
        },
        EnrichedInstanceIdentifier{kInstanceIdentifier1});
    EXPECT_TRUE(start_find_service_result.has_value());

    FindServiceHandle expected_handle_2{make_FindServiceHandle(2U)};
    const auto start_find_service_result_2 = service_discovery_client.StartFindService(
        expected_handle_2,
        [&service_found_barrier_2, &expected_handle_2, &handler_received_2](auto container, auto handle) noexcept {
            EXPECT_EQ(container.size(), 1);
            EXPECT_EQ(container.front(), kHandle2);
            EXPECT_EQ(handle, expected_handle_2);
            handler_received_2.store(true);
            service_found_barrier_2.set_value();
        },
        EnrichedInstanceIdentifier{kInstanceIdentifier2});
    EXPECT_TRUE(start_find_service_result_2.has_value());

    EXPECT_FALSE(handler_received_1.load());
    EXPECT_FALSE(handler_received_2.load());
    EXPECT_TRUE(service_discovery_client.OfferService(kInstanceIdentifier1).has_value());
    EXPECT_TRUE(service_discovery_client.OfferService(kInstanceIdentifier2).has_value());

    service_found_barrier_1.get_future().wait();
    service_found_barrier_2.get_future().wait();

    EXPECT_TRUE(handler_received_1.load());
    EXPECT_TRUE(handler_received_2.load());
}

TEST_F(ServiceDiscoveryClientFixture, HandlersAreNotCalledWhenServiceIsNotOffered)
{
    RecordProperty("Verifies", "1");
    RecordProperty("Description", "Checks that FindServiceHandlers are not called when a service is not offered.");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("Priority", "1");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    MockFunction<void(ServiceHandleContainer<HandleType>, FindServiceHandle)> find_service_handler_1{};
    MockFunction<void(ServiceHandleContainer<HandleType>, FindServiceHandle)> find_service_handler_2{};

    EXPECT_CALL(find_service_handler_1, Call(_, _)).Times(0);
    EXPECT_CALL(find_service_handler_2, Call(_, _)).Times(0);

    auto service_discovery_client = CreateAServiceDiscoveryClient();

    amp::ignore = service_discovery_client.StartFindService(make_FindServiceHandle(1U),
                                                            CreateWrappedMockFindServiceHandler(find_service_handler_1),
                                                            EnrichedInstanceIdentifier{kInstanceIdentifier1});

    amp::ignore = service_discovery_client.StartFindService(make_FindServiceHandle(2U),
                                                            CreateWrappedMockFindServiceHandler(find_service_handler_2),
                                                            EnrichedInstanceIdentifier{kInstanceIdentifier2});
}

TEST_F(ServiceDiscoveryClientFixture, HandlersAreCalledOnceWhenServiceIsOffered)
{
    RecordProperty("Verifies", "1");
    RecordProperty("Description", "Checks that FindServiceHandlers are called once when a service is offered.");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("Priority", "1");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    std::promise<void> service_found_barrier_1{};
    std::promise<void> service_found_barrier_2{};

    MockFunction<void(ServiceHandleContainer<HandleType>, FindServiceHandle)> find_service_handler_1{};
    MockFunction<void(ServiceHandleContainer<HandleType>, FindServiceHandle)> find_service_handler_2{};

    EXPECT_CALL(find_service_handler_1, Call(_, _)).WillOnce(InvokeWithoutArgs([&service_found_barrier_1]() {
        service_found_barrier_1.set_value();
    }));
    EXPECT_CALL(find_service_handler_2, Call(_, _)).WillOnce(InvokeWithoutArgs([&service_found_barrier_2]() {
        service_found_barrier_2.set_value();
    }));

    auto service_discovery_client = CreateAServiceDiscoveryClient();

    amp::ignore = service_discovery_client.StartFindService(make_FindServiceHandle(1U),
                                                            CreateWrappedMockFindServiceHandler(find_service_handler_1),
                                                            EnrichedInstanceIdentifier{kInstanceIdentifier1});

    amp::ignore = service_discovery_client.StartFindService(make_FindServiceHandle(2U),
                                                            CreateWrappedMockFindServiceHandler(find_service_handler_2),
                                                            EnrichedInstanceIdentifier{kInstanceIdentifier2});

    amp::ignore = service_discovery_client.OfferService(kInstanceIdentifier1);
    amp::ignore = service_discovery_client.OfferService(kInstanceIdentifier2);

    service_found_barrier_1.get_future().wait();
    service_found_barrier_2.get_future().wait();
}

TEST_F(ServiceDiscoveryClientFixture, HandlersAreCalledOnceWhenServiceIsStopOffered)
{
    RecordProperty("Verifies", "1");
    RecordProperty(
        "Description",
        "Checks that FindServiceHandlers are called once when a service that was already offered is stop offered.");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("Priority", "1");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    std::promise<void> service_found_barrier_after_offer_1{};
    std::promise<void> service_found_barrier_after_offer_2{};
    std::promise<void> service_found_barrier_after_stop_offer_1{};
    std::promise<void> service_found_barrier_after_stop_offer_2{};

    MockFunction<void(ServiceHandleContainer<HandleType>, FindServiceHandle)> find_service_handler_1{};
    MockFunction<void(ServiceHandleContainer<HandleType>, FindServiceHandle)> find_service_handler_2{};

    EXPECT_CALL(find_service_handler_1, Call(_, _))
        .WillOnce(InvokeWithoutArgs(
            [&service_found_barrier_after_stop_offer_1]() { service_found_barrier_after_stop_offer_1.set_value(); }));
    EXPECT_CALL(find_service_handler_2, Call(_, _))
        .WillOnce(InvokeWithoutArgs(
            [&service_found_barrier_after_stop_offer_2]() { service_found_barrier_after_stop_offer_2.set_value(); }));
    EXPECT_CALL(find_service_handler_1, Call(_, _))
        .WillOnce(InvokeWithoutArgs(
            [&service_found_barrier_after_offer_1]() { service_found_barrier_after_offer_1.set_value(); }))
        .RetiresOnSaturation();
    EXPECT_CALL(find_service_handler_2, Call(_, _))
        .WillOnce(InvokeWithoutArgs(
            [&service_found_barrier_after_offer_2]() { service_found_barrier_after_offer_2.set_value(); }))
        .RetiresOnSaturation();

    auto service_discovery_client = CreateAServiceDiscoveryClient();

    amp::ignore = service_discovery_client.StartFindService(make_FindServiceHandle(1U),
                                                            CreateWrappedMockFindServiceHandler(find_service_handler_1),
                                                            EnrichedInstanceIdentifier{kInstanceIdentifier1});

    amp::ignore = service_discovery_client.StartFindService(make_FindServiceHandle(2U),
                                                            CreateWrappedMockFindServiceHandler(find_service_handler_2),
                                                            EnrichedInstanceIdentifier{kInstanceIdentifier2});

    amp::ignore = service_discovery_client.OfferService(kInstanceIdentifier1);
    amp::ignore = service_discovery_client.OfferService(kInstanceIdentifier2);

    service_found_barrier_after_offer_1.get_future().wait();
    service_found_barrier_after_offer_2.get_future().wait();

    amp::ignore =
        service_discovery_client.StopOfferService(kInstanceIdentifier1, IServiceDiscovery::QualityTypeSelector::kBoth);
    amp::ignore =
        service_discovery_client.StopOfferService(kInstanceIdentifier2, IServiceDiscovery::QualityTypeSelector::kBoth);

    service_found_barrier_after_stop_offer_1.get_future().wait();
    service_found_barrier_after_stop_offer_2.get_future().wait();
}

TEST_F(ServiceDiscoveryClientFixture, CallsCorrectHandlerForAnyInstanceIDs)
{
    InSequence in_sequence{};

    std::promise<void> service_found_barrier_1{};
    std::promise<void> service_found_barrier_2{};
    FindServiceHandle expected_handle{make_FindServiceHandle(1U)};

    StrictMock<MockFunction<void(ServiceHandleContainer<HandleType>, FindServiceHandle)>> find_service_handler{};

    EXPECT_CALL(find_service_handler, Call(_, expected_handle))
        .WillOnce(WithArg<0>(Invoke([&service_found_barrier_1](auto container) {
            EXPECT_EQ(container.size(), 1);
            EXPECT_EQ(container[0], kHandleFindAny1);
            service_found_barrier_1.set_value();
        })));
    EXPECT_CALL(find_service_handler, Call(_, expected_handle))
        .WillOnce(WithArg<0>(Invoke([&service_found_barrier_2](auto container) {
            EXPECT_EQ(container.size(), 2);
            EXPECT_THAT(container, ::testing::Contains(kHandleFindAny1));
            EXPECT_THAT(container, ::testing::Contains(kHandleFindAny2));
            service_found_barrier_2.set_value();
        })));

    auto service_discovery_client = CreateAServiceDiscoveryClient();

    const auto start_find_service_result =
        service_discovery_client.StartFindService(expected_handle,
                                                  CreateWrappedMockFindServiceHandler(find_service_handler),
                                                  EnrichedInstanceIdentifier{kInstanceIdentifierAny});
    EXPECT_TRUE(start_find_service_result.has_value());

    EXPECT_TRUE(service_discovery_client.OfferService(kInstanceIdentifier1).has_value());
    service_found_barrier_1.get_future().wait();

    EXPECT_TRUE(service_discovery_client.OfferService(kInstanceIdentifier2).has_value());
    service_found_barrier_2.get_future().wait();
}

TEST_F(ServiceDiscoveryClientFixture, CorrectlyAssociatesOffersBasedOnQuality)
{
    std::promise<void> service_found_barrier_1{};
    std::promise<void> service_found_barrier_2{};
    FindServiceHandle expected_handle_1{make_FindServiceHandle(1U)};
    FindServiceHandle expected_handle_2{make_FindServiceHandle(2U)};

    StrictMock<MockFunction<void(ServiceHandleContainer<HandleType>, FindServiceHandle)>> find_service_handler_1{};

    EXPECT_CALL(find_service_handler_1, Call(_, expected_handle_1))
        .Times(::testing::Between(1, 2))
        .WillRepeatedly(WithArg<0>(Invoke([&service_found_barrier_1](auto container) {
            const std::unordered_set<HandleType> handles{container.begin(), container.end()};
            if (handles.find(kHandleFindAny1) != handles.cend() && handles.find(kHandleFindAny3) != handles.cend())
            {
                service_found_barrier_1.set_value();
            }
        })));

    StrictMock<MockFunction<void(ServiceHandleContainer<HandleType>, FindServiceHandle)>> find_service_handler_2{};
    EXPECT_CALL(find_service_handler_2, Call(_, expected_handle_2))
        .WillOnce(WithArg<0>(Invoke([&service_found_barrier_2](auto container) {
            EXPECT_EQ(container.size(), 1);
            EXPECT_EQ(container[0], kHandle3);
            service_found_barrier_2.set_value();
        })));

    auto service_discovery_client = CreateAServiceDiscoveryClient();

    const auto start_find_service_result_1 =
        service_discovery_client.StartFindService(expected_handle_1,
                                                  CreateWrappedMockFindServiceHandler(find_service_handler_1),
                                                  EnrichedInstanceIdentifier{kInstanceIdentifierAny});
    EXPECT_TRUE(start_find_service_result_1.has_value());

    const auto start_find_service_result_2 =
        service_discovery_client.StartFindService(expected_handle_2,
                                                  CreateWrappedMockFindServiceHandler(find_service_handler_2),
                                                  EnrichedInstanceIdentifier{kInstanceIdentifier3});
    EXPECT_TRUE(start_find_service_result_2.has_value());

    EXPECT_TRUE(service_discovery_client.OfferService(kInstanceIdentifier1).has_value());
    EXPECT_TRUE(service_discovery_client.OfferService(kInstanceIdentifier3).has_value());
    service_found_barrier_1.get_future().wait();
    service_found_barrier_2.get_future().wait();
}

TEST_F(ServiceDiscoveryClientFixture, RemovesWatchOnStopFindService)
{
    std::promise<void> barrier{};
    EXPECT_CALL(inotify_instance_mock_, RemoveWatch(_)).WillOnce([this, &barrier](auto watch_descriptor) {
        barrier.set_value();
        return inotify_instance_->RemoveWatch(watch_descriptor);
    });

    auto service_discovery_client = CreateAServiceDiscoveryClient();

    FindServiceHandle handle{make_FindServiceHandle(1U)};
    const auto start_find_service_result = service_discovery_client.StartFindService(
        handle, [](auto, auto) noexcept {}, EnrichedInstanceIdentifier{kInstanceIdentifier1});
    ASSERT_TRUE(start_find_service_result.has_value());
    const auto stop_find_service_result = service_discovery_client.StopFindService(handle);
    ASSERT_TRUE(stop_find_service_result.has_value());

    CreateRegularFile(filesystem_, GetFlagFilePrefix(kServiceId.service_id_, kInstanceId1));

    barrier.get_future().wait();
}

TEST_F(ServiceDiscoveryClientFixture, DoesNotCallHandlerIfFindServiceIsStopped)
{
    RecordProperty("ParentRequirement", "4");
    RecordProperty("Description",
                   "Stops the asynchronous search for available services.After the call has returned no further calls "
                   "to the user provided FindServiceHandler takes place.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    std::promise<void> handler_destruction_barrier{};
    bool handler_called{false};

    // Given a DestructorNotifier object which will set the value of the handler_destruction_barrier promise on
    // destruction
    DestructorNotifier destructor_notifier{&handler_destruction_barrier};

    auto service_discovery_client = CreateAServiceDiscoveryClient();

    // When calling StartFindService
    FindServiceHandle expected_handle{make_FindServiceHandle(1U)};
    const auto result = service_discovery_client.StartFindService(
        expected_handle,
        [&handler_called, destructor_notifier = std::move(destructor_notifier)](auto, auto) noexcept {
            handler_called = true;
        },
        EnrichedInstanceIdentifier{kInstanceIdentifier1});
    ASSERT_TRUE(result.has_value());

    // and calling StopFindService before calling OfferService
    EXPECT_TRUE(service_discovery_client.StopFindService(expected_handle).has_value());
    EXPECT_TRUE(service_discovery_client.OfferService(kInstanceIdentifier1).has_value());

    // Then the handler passed to StartFindService should never be called (we stop waiting once the handler is
    // destroyed, indicated by the destructor of DestructorNotifier).
    handler_destruction_barrier.get_future().wait();
    EXPECT_FALSE(handler_called);
}

TEST_F(ServiceDiscoveryClientFixture, DoesNotCallHandlerIfFindServiceIsStoppedAnyInstanceIds)
{
    std::promise<void> handler_destruction_barrier{};
    bool handler_called{false};

    // Given a DestructorNotifier object which will set the value of the handler_destruction_barrier promise on
    // destruction
    DestructorNotifier destructor_notifier{&handler_destruction_barrier};

    auto service_discovery_client = CreateAServiceDiscoveryClient();

    // When calling StartFindService
    FindServiceHandle expected_handle{make_FindServiceHandle(1U)};
    const auto start_find_service_result = service_discovery_client.StartFindService(
        expected_handle,
        [&handler_called, destructor_notifier = std::move(destructor_notifier)](auto, auto) noexcept {
            handler_called = true;
        },
        EnrichedInstanceIdentifier{kInstanceIdentifierAny});
    EXPECT_TRUE(start_find_service_result.has_value());

    // and calling StopFindService before calling OfferService
    EXPECT_TRUE(service_discovery_client.StopFindService(expected_handle).has_value());
    EXPECT_TRUE(service_discovery_client.OfferService(kInstanceIdentifier1).has_value());

    // Then the handler passed to StartFindService should never be called (we stop waiting once the handler is
    // destroyed, indicated by the destructor of DestructorNotifier).
    handler_destruction_barrier.get_future().wait();
    EXPECT_FALSE(handler_called);
}

TEST_F(ServiceDiscoveryClientFixture, CorrectlyAssociatesSubsearchWithCorrectDirectory)
{
    std::promise<void> services_offered_barrier{};
    auto services_offered_barrier_future = services_offered_barrier.get_future();
    std::promise<void> handler_destruction_barrier{};
    DestructorNotifier destructor_notifier{&handler_destruction_barrier};

    EXPECT_CALL(inotify_instance_mock_, Read())
        .WillOnce([this, &services_offered_barrier_future] {
            services_offered_barrier_future.wait();
            return inotify_instance_->Read();
        })
        .WillRepeatedly([] { return amp::static_vector<os::InotifyEvent, os::InotifyInstance::max_events>{}; });

    // Given a DestructorNotifier object which will set the value of the handler_destruction_barrier promise on
    // destruction
    auto service_discovery_client = CreateAServiceDiscoveryClient();

    // With a handler
    StrictMock<MockFunction<void(ServiceHandleContainer<HandleType>, FindServiceHandle)>> find_service_handler{};

    // Where the first invocation does nothing and the second calling StopFindService
    EXPECT_CALL(find_service_handler, Call(_, _))
        .WillOnce(DoDefault())
        .WillOnce(Invoke([&service_discovery_client](const auto& handles, const auto& find_service_handle) {
            ASSERT_TRUE(handles.empty());
            const auto result = service_discovery_client.StopFindService(find_service_handle);
            ASSERT_TRUE(result.has_value());
        }));

    // with one instance already offered
    EXPECT_TRUE(service_discovery_client.OfferService(kInstanceIdentifier1).has_value());

    // when calling StartFindService
    FindServiceHandle expected_handle{make_FindServiceHandle(1U)};
    const auto start_find_service_result = service_discovery_client.StartFindService(
        expected_handle,
        [&find_service_handler, destructor_notifier = std::move(destructor_notifier)](
            ServiceHandleContainer<HandleType> containers, FindServiceHandle handle) noexcept {
            find_service_handler.AsStdFunction()(containers, handle);
        },
        EnrichedInstanceIdentifier{kInstanceIdentifierAny});
    EXPECT_TRUE(start_find_service_result.has_value());

    // and a stop offer waiting in the event queue
    EXPECT_TRUE(
        service_discovery_client.StopOfferService(kInstanceIdentifier1, IServiceDiscovery::QualityTypeSelector::kBoth)
            .has_value());
    services_offered_barrier.set_value();

    // Then the handler passed to StartFindService should not crash and be called two times
    handler_destruction_barrier.get_future().wait();
}

TEST_F(ServiceDiscoveryClientFixture, DoesNotCallHandlerIfFindServiceIsStoppedButEventInSameBatchFits)
{
    std::promise<void> services_offered_barrier{};
    auto services_offered_barrier_future = services_offered_barrier.get_future();
    std::promise<void> handler_destruction_barrier{};
    DestructorNotifier destructor_notifier{&handler_destruction_barrier};

    EXPECT_CALL(inotify_instance_mock_, Read())
        .WillOnce([this, &services_offered_barrier_future] {
            services_offered_barrier_future.wait();
            return inotify_instance_->Read();
        })
        .WillRepeatedly([] { return amp::static_vector<os::InotifyEvent, os::InotifyInstance::max_events>{}; });

    // Given a DestructorNotifier object which will set the value of the handler_destruction_barrier promise on
    // destruction
    auto service_discovery_client = CreateAServiceDiscoveryClient();

    // With a handler
    StrictMock<MockFunction<void(ServiceHandleContainer<HandleType>, FindServiceHandle)>> find_service_handler{};

    // Where the first invocation does nothing and the second calling StopFindService
    EXPECT_CALL(find_service_handler, Call(_, _))
        .WillOnce(DoDefault())
        .WillOnce(WithArg<1>(Invoke([&service_discovery_client](const auto& find_service_handle) {
            const auto result = service_discovery_client.StopFindService(find_service_handle);
            ASSERT_TRUE(result.has_value());
        })));

    // with one instance already offered
    EXPECT_TRUE(service_discovery_client.OfferService(kInstanceIdentifier1).has_value());

    // when calling StartFindService
    FindServiceHandle expected_handle{make_FindServiceHandle(1U)};
    const auto start_find_service_result = service_discovery_client.StartFindService(
        expected_handle,
        [&find_service_handler, destructor_notifier = std::move(destructor_notifier)](
            ServiceHandleContainer<HandleType> containers, FindServiceHandle handle) noexcept {
            find_service_handler.AsStdFunction()(containers, handle);
        },
        EnrichedInstanceIdentifier{kInstanceIdentifierAny});
    EXPECT_TRUE(start_find_service_result.has_value());

    // and two additional events waiting in one batch after the search is started
    EXPECT_TRUE(
        service_discovery_client.StopOfferService(kInstanceIdentifier1, IServiceDiscovery::QualityTypeSelector::kBoth)
            .has_value());
    EXPECT_TRUE(service_discovery_client.OfferService(kInstanceIdentifier2).has_value());
    services_offered_barrier.set_value();

    // Then the handler passed to StartFindService should not be called a third time (we stop waiting once the handler
    // is destroyed, indicated by the destructor of DestructorNotifier).
    handler_destruction_barrier.get_future().wait();
}

TEST_F(ServiceDiscoveryClientFixture, DoesNotCallHandlerIfServiceOfferIsStoppedBeforeSearchStarts)
{
    RecordProperty("Verifies", "4");
    RecordProperty("Description",
                   "Checks that a service is not visible to consumers (i.e. to StartFindService) after "
                   "StopOfferService is called.");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("Priority", "1");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    bool handler_called{false};

    auto service_discovery_client = CreateAServiceDiscoveryClient();

    // When calling OfferService and then immediately StopOfferService
    EXPECT_TRUE(service_discovery_client.OfferService(kInstanceIdentifier1).has_value());
    ASSERT_TRUE(
        service_discovery_client.StopOfferService(kInstanceIdentifier1, IServiceDiscovery::QualityTypeSelector::kBoth)
            .has_value());

    // When calling StartFindService (which calls the handler synchronously if the offer is already present)
    FindServiceHandle expected_handle{make_FindServiceHandle(1U)};
    const auto start_find_service_result = service_discovery_client.StartFindService(
        expected_handle,
        [&handler_called](auto, auto) noexcept { handler_called = true; },
        EnrichedInstanceIdentifier{kInstanceIdentifier1});
    EXPECT_TRUE(start_find_service_result.has_value());

    // Then the handler should not be called
    EXPECT_FALSE(handler_called);
}

TEST_F(ServiceDiscoveryClientFixture, ReCallsCorrectHandlerForSpecificInstanceIDAfterReoffering)
{
    InSequence in_sequence{};

    StrictMock<MockFunction<void(ServiceHandleContainer<HandleType>, FindServiceHandle)>> find_service_handler_1{};
    StrictMock<MockFunction<void(ServiceHandleContainer<HandleType>, FindServiceHandle)>> find_service_handler_2{};

    std::promise<void> service_found_barrier_1{};
    std::promise<void> service_found_barrier_2{};
    std::promise<void> service_found_barrier_after_stop_offer{};
    std::promise<void> service_found_barrier_after_offer{};

    FindServiceHandle expected_handle_1{make_FindServiceHandle(1U)};
    FindServiceHandle expected_handle_2{make_FindServiceHandle(2U)};

    // Expecting that both handlers are called when the services are initially offered
    EXPECT_CALL(find_service_handler_1, Call(_, expected_handle_1))
        .WillOnce(WithArg<0>(Invoke([&service_found_barrier_1](auto container) {
            EXPECT_EQ(container.size(), 1);
            EXPECT_EQ(container.front(), kHandle1);
            service_found_barrier_1.set_value();
        })));
    EXPECT_CALL(find_service_handler_2, Call(_, expected_handle_2))
        .WillOnce(WithArg<0>(Invoke([&service_found_barrier_2](auto container) {
            EXPECT_EQ(container.size(), 1);
            EXPECT_EQ(container.front(), kHandle2);
            service_found_barrier_2.set_value();
        })));

    // and expecting that only the handler for the first service instance is called again when StopOfferService is
    // called on that instance
    EXPECT_CALL(find_service_handler_1, Call(_, expected_handle_1))
        .WillOnce(WithArg<0>(Invoke([&service_found_barrier_after_stop_offer](auto container) {
            EXPECT_EQ(container.size(), 0);
            service_found_barrier_after_stop_offer.set_value();
        })));

    // and expecting that only the handler for the first service instance is called again when OfferService is again
    // called on that instance
    EXPECT_CALL(find_service_handler_1, Call(_, expected_handle_1))
        .WillOnce(WithArg<0>(Invoke([&service_found_barrier_after_offer](auto container) {
            EXPECT_EQ(container.size(), 1);
            EXPECT_EQ(container.front(), kHandle1);
            service_found_barrier_after_offer.set_value();
        })));

    auto service_discovery_client = CreateAServiceDiscoveryClient();

    const auto start_find_service_result_1 =
        service_discovery_client.StartFindService(expected_handle_1,
                                                  CreateWrappedMockFindServiceHandler(find_service_handler_1),
                                                  EnrichedInstanceIdentifier{kInstanceIdentifier1});
    ASSERT_TRUE(start_find_service_result_1.has_value());

    const auto start_find_service_result_2 =
        service_discovery_client.StartFindService(expected_handle_2,
                                                  CreateWrappedMockFindServiceHandler(find_service_handler_2),
                                                  EnrichedInstanceIdentifier{kInstanceIdentifier2});
    ASSERT_TRUE(start_find_service_result_2.has_value());

    EXPECT_TRUE(service_discovery_client.OfferService(kInstanceIdentifier1).has_value());
    service_found_barrier_1.get_future().wait();

    EXPECT_TRUE(service_discovery_client.OfferService(kInstanceIdentifier2).has_value());
    service_found_barrier_2.get_future().wait();

    ASSERT_TRUE(
        service_discovery_client.StopOfferService(kInstanceIdentifier1, IServiceDiscovery::QualityTypeSelector::kBoth)
            .has_value());
    service_found_barrier_after_stop_offer.get_future().wait();

    EXPECT_TRUE(service_discovery_client.OfferService(kInstanceIdentifier1).has_value());
    service_found_barrier_after_offer.get_future().wait();
}

TEST_F(ServiceDiscoveryClientFixture, ReCallsCorrectHandlerForAnyInstanceIDsAfterReoffering)
{
    InSequence in_sequence{};

    std::promise<void> service_found_barrier_1{};
    std::promise<void> service_found_barrier_2{};
    std::promise<void> service_found_barrier_after_stop_offer{};
    std::promise<void> service_found_barrier_after_offer{};

    FindServiceHandle expected_handle{make_FindServiceHandle(1U)};

    StrictMock<MockFunction<void(ServiceHandleContainer<HandleType>, FindServiceHandle)>> find_service_handler{};

    // Expecting that the handler is called twice when the services are initially offered
    EXPECT_CALL(find_service_handler, Call(_, expected_handle))
        .WillOnce(WithArg<0>(Invoke([&service_found_barrier_1](auto container) {
            EXPECT_EQ(container.size(), 1);
            EXPECT_EQ(container[0], kHandleFindAny1);
            service_found_barrier_1.set_value();
        })));
    EXPECT_CALL(find_service_handler, Call(_, expected_handle))
        .WillOnce(WithArg<0>(Invoke([&service_found_barrier_2](auto container) {
            EXPECT_EQ(container.size(), 2);
            EXPECT_THAT(container, ::testing::Contains(kHandleFindAny1));
            EXPECT_THAT(container, ::testing::Contains(kHandleFindAny2));
            service_found_barrier_2.set_value();
        })));

    // and expecting that the handler is called again containing only the second instance when StopOfferService is
    // called on the first instance
    EXPECT_CALL(find_service_handler, Call(_, expected_handle))
        .WillOnce(WithArg<0>(Invoke([&service_found_barrier_after_stop_offer](auto container) {
            EXPECT_EQ(container.size(), 1);
            EXPECT_EQ(container[0], kHandleFindAny2);
            service_found_barrier_after_stop_offer.set_value();
        })));

    // and expecting that only the handler is called again when the first instance is offered again
    EXPECT_CALL(find_service_handler, Call(_, _))
        .WillOnce(WithArg<0>(Invoke([&service_found_barrier_after_offer](auto container) {
            EXPECT_EQ(container.size(), 2);
            EXPECT_THAT(container, ::testing::Contains(kHandleFindAny1));
            EXPECT_THAT(container, ::testing::Contains(kHandleFindAny2));
            service_found_barrier_after_offer.set_value();
        })));

    auto service_discovery_client = CreateAServiceDiscoveryClient();

    const auto start_find_service_result =
        service_discovery_client.StartFindService(expected_handle,
                                                  CreateWrappedMockFindServiceHandler(find_service_handler),
                                                  EnrichedInstanceIdentifier{kInstanceIdentifierAny});
    EXPECT_TRUE(start_find_service_result.has_value());

    EXPECT_TRUE(service_discovery_client.OfferService(kInstanceIdentifier1).has_value());
    service_found_barrier_1.get_future().wait();

    EXPECT_TRUE(service_discovery_client.OfferService(kInstanceIdentifier2).has_value());
    service_found_barrier_2.get_future().wait();

    ASSERT_TRUE(
        service_discovery_client.StopOfferService(kInstanceIdentifier1, IServiceDiscovery::QualityTypeSelector::kBoth)
            .has_value());
    service_found_barrier_after_stop_offer.get_future().wait();

    EXPECT_TRUE(service_discovery_client.OfferService(kInstanceIdentifier1).has_value());
    service_found_barrier_after_offer.get_future().wait();
}

TEST_F(ServiceDiscoveryClientFixture, ReCallsCorrectHandlerForDifferentInstanceIDsAfterRestartingStartFindService)
{
    InSequence in_sequence{};

    std::promise<void> service_found_barrier_1{};
    std::promise<void> service_found_barrier_2{};
    std::promise<void> service_found_barrier_after_start_find_service{};

    FindServiceHandle expected_handle_1{make_FindServiceHandle(1U)};
    FindServiceHandle expected_handle_2{make_FindServiceHandle(2U)};

    FindServiceHandle expected_handle_1_second_start_find_service{make_FindServiceHandle(3U)};

    StrictMock<MockFunction<void(ServiceHandleContainer<HandleType>, FindServiceHandle)>> find_service_handler{};
    StrictMock<MockFunction<void(ServiceHandleContainer<HandleType>, FindServiceHandle)>>
        find_service_handler_after_restart{};

    // Expecting that the handler is called twice when the services are initially offered
    EXPECT_CALL(find_service_handler, Call(_, expected_handle_1))
        .WillOnce(WithArg<0>(Invoke([&service_found_barrier_1](auto container) {
            EXPECT_EQ(container.size(), 1);
            EXPECT_EQ(container[0], kHandle1);
            service_found_barrier_1.set_value();
        })));
    EXPECT_CALL(find_service_handler, Call(_, expected_handle_2))
        .WillOnce(WithArg<0>(Invoke([&service_found_barrier_2](auto container) {
            EXPECT_EQ(container.size(), 1);
            EXPECT_EQ(container[0], kHandle2);
            service_found_barrier_2.set_value();
        })));

    // and expecting that the handler is called again for the first instance when StartFindService is called again after
    // StopFindService
    EXPECT_CALL(find_service_handler_after_restart, Call(_, expected_handle_1_second_start_find_service))
        .WillOnce(WithArg<0>(Invoke([&service_found_barrier_after_start_find_service](auto container) {
            EXPECT_EQ(container.size(), 1);
            EXPECT_EQ(container[0], kHandle1);
            service_found_barrier_after_start_find_service.set_value();
        })));

    auto service_discovery_client = CreateAServiceDiscoveryClient();

    const auto find_service_result =
        service_discovery_client.StartFindService(expected_handle_1,
                                                  CreateWrappedMockFindServiceHandler(find_service_handler),
                                                  EnrichedInstanceIdentifier{kInstanceIdentifier1});
    ASSERT_TRUE(find_service_result.has_value());

    const auto find_service_result_2 =
        service_discovery_client.StartFindService(expected_handle_2,
                                                  CreateWrappedMockFindServiceHandler(find_service_handler),
                                                  EnrichedInstanceIdentifier{kInstanceIdentifier2});
    ASSERT_TRUE(find_service_result_2.has_value());

    EXPECT_TRUE(service_discovery_client.OfferService(kInstanceIdentifier1).has_value());
    service_found_barrier_1.get_future().wait();

    EXPECT_TRUE(service_discovery_client.OfferService(kInstanceIdentifier2).has_value());
    service_found_barrier_2.get_future().wait();

    ASSERT_TRUE(service_discovery_client.StopFindService(expected_handle_1).has_value());

    const auto find_service_result_3 = service_discovery_client.StartFindService(
        expected_handle_1_second_start_find_service,
        CreateWrappedMockFindServiceHandler(find_service_handler_after_restart),
        EnrichedInstanceIdentifier{kInstanceIdentifier1});
    ASSERT_TRUE(find_service_result_3.has_value());
    service_found_barrier_after_start_find_service.get_future().wait();
}

TEST_F(ServiceDiscoveryClientFixture, ReCallsCorrectHandlerForAnyInstanceIDsAfterRestartingStartFindService)
{
    InSequence in_sequence{};

    std::promise<void> service_found_barrier_1{};
    std::promise<void> service_found_barrier_2{};
    std::promise<void> service_found_barrier_after_start_find_service{};

    FindServiceHandle expected_handle{make_FindServiceHandle(1U)};
    FindServiceHandle expected_handle_second_start_find_service{make_FindServiceHandle(2U)};

    StrictMock<MockFunction<void(ServiceHandleContainer<HandleType>, FindServiceHandle)>> find_service_handler{};
    StrictMock<MockFunction<void(ServiceHandleContainer<HandleType>, FindServiceHandle)>>
        find_service_handler_after_restart{};

    // Expecting that the handler is called twice when the services are initially offered
    EXPECT_CALL(find_service_handler, Call(_, expected_handle))
        .WillOnce(WithArg<0>(Invoke([&service_found_barrier_1](auto container) {
            EXPECT_EQ(container.size(), 1);
            EXPECT_EQ(container[0], kHandleFindAny1);
            service_found_barrier_1.set_value();
        })));
    EXPECT_CALL(find_service_handler, Call(_, expected_handle))
        .WillOnce(WithArg<0>(Invoke([&service_found_barrier_2](auto container) {
            EXPECT_EQ(container.size(), 2);
            EXPECT_THAT(container, ::testing::Contains(kHandleFindAny1));
            EXPECT_THAT(container, ::testing::Contains(kHandleFindAny2));
            service_found_barrier_2.set_value();
        })));

    // and expecting that the handler is called again when StartFindService is called again after StopFindService
    EXPECT_CALL(find_service_handler_after_restart, Call(_, expected_handle_second_start_find_service))
        .WillOnce(WithArg<0>(Invoke([&service_found_barrier_after_start_find_service](auto container) {
            EXPECT_EQ(container.size(), 2);
            EXPECT_THAT(container, ::testing::Contains(kHandleFindAny1));
            EXPECT_THAT(container, ::testing::Contains(kHandleFindAny2));
            service_found_barrier_after_start_find_service.set_value();
        })));

    auto service_discovery_client = CreateAServiceDiscoveryClient();

    const auto find_service_result =
        service_discovery_client.StartFindService(expected_handle,
                                                  CreateWrappedMockFindServiceHandler(find_service_handler),
                                                  EnrichedInstanceIdentifier{kInstanceIdentifierAny});
    ASSERT_TRUE(find_service_result.has_value());

    EXPECT_TRUE(service_discovery_client.OfferService(kInstanceIdentifier1).has_value());
    service_found_barrier_1.get_future().wait();

    EXPECT_TRUE(service_discovery_client.OfferService(kInstanceIdentifier2).has_value());
    service_found_barrier_2.get_future().wait();

    ASSERT_TRUE(service_discovery_client.StopFindService(expected_handle).has_value());

    const auto find_service_result_2 = service_discovery_client.StartFindService(
        expected_handle_second_start_find_service,
        CreateWrappedMockFindServiceHandler(find_service_handler_after_restart),
        EnrichedInstanceIdentifier{kInstanceIdentifierAny});

    ASSERT_TRUE(find_service_result_2.has_value());
    service_found_barrier_after_start_find_service.get_future().wait();
}

TEST_F(ServiceDiscoveryClientFixture, CanCallStartFindServiceInsideHandler)
{
    InSequence in_sequence{};

    std::promise<void> service_found_barrier{};
    FindServiceHandle expected_handle_first_search{make_FindServiceHandle(1U)};
    FindServiceHandle expected_handle_second_search{make_FindServiceHandle(2U)};

    StrictMock<MockFunction<void(ServiceHandleContainer<HandleType>, FindServiceHandle)>> find_service_handler{};

    auto service_discovery_client = CreateAServiceDiscoveryClient();

    // Expecting that the find service handler is called when the first service is offered.
    // and that StartFindService is called within that handler
    EXPECT_CALL(find_service_handler, Call(_, expected_handle_first_search))
        .WillOnce(
            Invoke([&service_discovery_client, &expected_handle_second_search, &find_service_handler](auto, auto) {
                const auto result =
                    service_discovery_client.StartFindService(expected_handle_second_search,
                                                              CreateWrappedMockFindServiceHandler(find_service_handler),
                                                              EnrichedInstanceIdentifier{kInstanceIdentifier2});
                ASSERT_TRUE(result.has_value());
            }));

    EXPECT_CALL(find_service_handler, Call(_, expected_handle_second_search))
        .WillOnce(Invoke([&service_found_barrier](auto, auto) { service_found_barrier.set_value(); }));

    // When calling StartFindService with a search
    const auto start_find_service_result =
        service_discovery_client.StartFindService(expected_handle_first_search,
                                                  CreateWrappedMockFindServiceHandler(find_service_handler),
                                                  EnrichedInstanceIdentifier{kInstanceIdentifier1});
    ASSERT_TRUE(start_find_service_result.has_value());

    // and OfferService is called offering the first instance
    const auto result_1 = service_discovery_client.OfferService(kInstanceIdentifier1);
    ASSERT_TRUE(result_1.has_value());

    // and OfferService is called offering the second instance
    const auto result_2 = service_discovery_client.OfferService(kInstanceIdentifier2);
    ASSERT_TRUE(result_2.has_value());

    // both handlers are invoked and do not block
    service_found_barrier.get_future().wait();
}

TEST_F(ServiceDiscoveryClientFixture, CanCallStopFindServiceInsideHandler)
{
    InSequence in_sequence{};

    FindServiceHandle expected_handle{make_FindServiceHandle(1U)};

    StrictMock<MockFunction<void(ServiceHandleContainer<HandleType>, FindServiceHandle)>> find_service_handler{};
    std::promise<void> handler_destroyed{};
    DestructorNotifier destructor_notifier{&handler_destroyed};

    auto service_discovery_client = CreateAServiceDiscoveryClient();

    // Expecting that the find service handler is called when the first service is offered.
    // and that StopFindService is called within that handler
    EXPECT_CALL(find_service_handler, Call(_, expected_handle))
        .WillOnce(Invoke([&service_discovery_client](auto container, auto find_service_handle) {
            EXPECT_EQ(container.size(), 1);
            EXPECT_EQ(container[0], kHandleFindAny1);
            const auto result = service_discovery_client.StopFindService(find_service_handle);
            ASSERT_TRUE(result.has_value());

            ASSERT_TRUE(service_discovery_client.OfferService(kInstanceIdentifier2).has_value());
        }));

    // Then the find service handler will not be called again when the second service is offered.
    EXPECT_CALL(find_service_handler, Call(_, expected_handle)).Times(0);

    // When calling StartFindService with a Find Any search
    const auto result = service_discovery_client.StartFindService(
        expected_handle,
        [&find_service_handler, notifier = std::move(destructor_notifier)](
            ServiceHandleContainer<HandleType> containers, FindServiceHandle handle) noexcept {
            find_service_handler.AsStdFunction()(containers, handle);
        },
        EnrichedInstanceIdentifier{kInstanceIdentifierAny});
    ASSERT_TRUE(result.has_value());

    // and OfferService is called offering the first instance
    ASSERT_TRUE(service_discovery_client.OfferService(kInstanceIdentifier1).has_value());

    // Unblock the worker thread to actually remove the search
    CreateRegularFile(filesystem_, GetFlagFilePrefix(kServiceId.service_id_, kInstanceId1));

    // Wait for the handler to be destructed since after that we can be sure that it is no longer called.
    handler_destroyed.get_future().wait();
}

TEST_F(ServiceDiscoveryClientFixture, StopFindServiceBlocksUntilHandlerFinishedWhenCalledOutsideHandler)
{
    InSequence in_sequence{};

    std::promise<void> service_found_barrier{};
    std::promise<void> search_stopped_barrier{};
    FindServiceHandle expected_handle{make_FindServiceHandle(1U)};

    StrictMock<MockFunction<void(ServiceHandleContainer<HandleType>, FindServiceHandle)>> find_service_handler{};

    auto service_discovery_client = CreateAServiceDiscoveryClient();

    // Expecting that the find service handler is called when the first service is offered.
    // and that StopFindService is called within that handler
    EXPECT_CALL(find_service_handler, Call(_, expected_handle))
        .WillOnce(Invoke([&service_found_barrier, &search_stopped_barrier](auto, auto) {
            service_found_barrier.set_value();
            // Give some chance for missing synchronization to become obvious
            const auto future_status = search_stopped_barrier.get_future().wait_for(std::chrono::milliseconds{5});
            ASSERT_EQ(future_status, std::future_status::timeout) << "StopFindService did not wait as promised";
        }));

    // When calling StartFindService with a Find Any search
    const auto start_find_service_result =
        service_discovery_client.StartFindService(expected_handle,
                                                  CreateWrappedMockFindServiceHandler(find_service_handler),
                                                  EnrichedInstanceIdentifier{kInstanceIdentifier1});
    ASSERT_TRUE(start_find_service_result.has_value());

    // and OfferService is called offering the first instance
    ASSERT_TRUE(service_discovery_client.OfferService(kInstanceIdentifier1).has_value());
    service_found_barrier.get_future().wait();

    // StopFindService blocks until the ongoing invocation is finished
    const auto stop_find_service_result = service_discovery_client.StopFindService(expected_handle);
    ASSERT_TRUE(stop_find_service_result.has_value());
    search_stopped_barrier.set_value();
}

TEST_F(ServiceDiscoveryClientFixture, FilesystemIsNotRecrawledIfExactSameSearchAlreadyExists)
{
    // Given a ServiceDiscoveryClient with mocked inotify instance
    std::promise<void> barrier{};

    // Expecting per crawl and watch of the filesystem for a specific instance we expect one "new" watch, no more!
    EXPECT_CALL(inotify_instance_mock_, AddWatch(_, _)).Times(1);

    auto service_discovery_client = CreateAServiceDiscoveryClient();

    FindServiceHandle expected_handle_1{make_FindServiceHandle(1U)};
    FindServiceHandle expected_handle_2{make_FindServiceHandle(2U)};

    // and given an offered service instance
    EXPECT_TRUE(service_discovery_client.OfferService(kInstanceIdentifier1).has_value());

    // When starting the same service discovery for above offer twice recursively
    testing::MockFunction<void(std::vector<HandleType>, FindServiceHandle)> inner_handler{};
    EXPECT_CALL(inner_handler, Call(_, _))
        .WillOnce([&barrier, &expected_handle_2](auto container, auto handle) noexcept {
            EXPECT_EQ(container.size(), 1);
            EXPECT_EQ(container.front(), kHandle1);
            EXPECT_EQ(handle, expected_handle_2);
            barrier.set_value();
        });

    testing::MockFunction<void(std::vector<HandleType>, FindServiceHandle)> outer_handler{};
    EXPECT_CALL(outer_handler, Call(_, _))
        .WillOnce([&service_discovery_client, &expected_handle_2, &inner_handler](auto handles, auto) noexcept {
            ASSERT_EQ(handles.size(), 1U);
            const auto start_find_service_result = service_discovery_client.StartFindService(
                expected_handle_2,
                [&inner_handler](auto container, auto handle) { inner_handler.AsStdFunction()(container, handle); },
                EnrichedInstanceIdentifier{handles.front()});
            EXPECT_TRUE(start_find_service_result.has_value());
        });

    const auto start_find_service_result_1 = service_discovery_client.StartFindService(
        expected_handle_1,
        [&outer_handler](auto container, auto handle) { outer_handler.AsStdFunction()(container, handle); },
        EnrichedInstanceIdentifier{kInstanceIdentifier1});
    ASSERT_TRUE(start_find_service_result_1.has_value());

    // Then the service is found both times
    barrier.get_future().wait();
}

TEST_F(ServiceDiscoveryClientFixture, SearchFromCachedSearchReceivesFollowupUpdates)
{
    // Given a ServiceDiscoveryClient with mocked inotify instance with an offered service instance
    std::promise<void> barrier{};
    std::promise<void> barrier2{};

    FindServiceHandle expected_handle_1{make_FindServiceHandle(1U)};
    FindServiceHandle expected_handle_2{make_FindServiceHandle(2U)};

    auto service_discovery_client = CreateAServiceDiscoveryClient();

    EXPECT_TRUE(service_discovery_client.OfferService(kInstanceIdentifier1).has_value());

    // Expect both service discoveries to receive the offer and stop-offer of the service instance
    testing::MockFunction<void(std::vector<HandleType>, FindServiceHandle)> inner_handler{};
    EXPECT_CALL(inner_handler, Call(_, _))
        .WillOnce([&barrier, &expected_handle_2](auto container, auto handle) noexcept {
            EXPECT_EQ(container.size(), 1);
            EXPECT_EQ(container.front(), kHandle1);
            EXPECT_EQ(handle, expected_handle_2);
            barrier.set_value();
        })
        .WillOnce([&barrier2, &expected_handle_2](auto container, auto handle) noexcept {
            EXPECT_EQ(container.size(), 0);
            EXPECT_EQ(handle, expected_handle_2);
            barrier2.set_value();
        });

    testing::MockFunction<void(std::vector<HandleType>, FindServiceHandle)> outer_handler{};
    EXPECT_CALL(outer_handler, Call(_, _))
        .WillOnce([&service_discovery_client, &expected_handle_2, &inner_handler](auto handles, auto) noexcept {
            ASSERT_EQ(handles.size(), 1U);
            const auto start_find_service_result = service_discovery_client.StartFindService(
                expected_handle_2,
                [&inner_handler](auto container, auto handle) { inner_handler.AsStdFunction()(container, handle); },
                EnrichedInstanceIdentifier{handles.front()});
            EXPECT_TRUE(start_find_service_result.has_value());
        })
        .WillRepeatedly([](auto, auto) {});

    // When recursively starting the service discovery
    const auto start_find_service_result_1 = service_discovery_client.StartFindService(
        expected_handle_1,
        [&outer_handler](auto container, auto handle) { outer_handler.AsStdFunction()(container, handle); },
        EnrichedInstanceIdentifier{kInstanceIdentifier1});
    ASSERT_TRUE(start_find_service_result_1.has_value());

    barrier.get_future().wait();

    // And then stopping the offer
    EXPECT_TRUE(
        service_discovery_client.StopOfferService(kInstanceIdentifier1, IServiceDiscovery::QualityTypeSelector::kBoth)
            .has_value());

    barrier2.get_future().wait();
}

TEST_F(ServiceDiscoveryClientFixture, FilesystemIsNotRecrawledIfAnySearchAlreadyExists)
{
    // Given a ServiceDiscoveryClient with mocked inotify instance
    std::promise<void> barrier{};

    FindServiceHandle expected_handle_1{make_FindServiceHandle(1U)};
    FindServiceHandle expected_handle_2{make_FindServiceHandle(2U)};

    // Expecting per crawl and watch of the filesystem for any instance we expect two "new" watches, no more!
    EXPECT_CALL(inotify_instance_mock_, AddWatch(_, _)).Times(2);
    auto service_discovery_client = CreateAServiceDiscoveryClient();

    // and given an offered service instance
    EXPECT_TRUE(service_discovery_client.OfferService(kInstanceIdentifier1).has_value());

    // When starting the same service discovery for above offer twice recursively
    testing::MockFunction<void(std::vector<HandleType>, FindServiceHandle)> inner_handler{};
    EXPECT_CALL(inner_handler, Call(_, _))
        .WillOnce([&barrier, &expected_handle_2](auto container, auto handle) noexcept {
            EXPECT_EQ(container.size(), 1);
            EXPECT_EQ(container.front(), kHandleFindAny1);
            EXPECT_EQ(handle, expected_handle_2);
            barrier.set_value();
        });

    testing::MockFunction<void(std::vector<HandleType>, FindServiceHandle)> outer_handler{};
    EXPECT_CALL(outer_handler, Call(_, _))
        .WillOnce([&service_discovery_client, &expected_handle_2, &inner_handler](auto handles, auto) noexcept {
            ASSERT_EQ(handles.size(), 1U);
            const auto start_find_service_result = service_discovery_client.StartFindService(
                expected_handle_2,
                [&inner_handler](auto container, auto handle) { inner_handler.AsStdFunction()(container, handle); },
                EnrichedInstanceIdentifier{handles.front()});
            EXPECT_TRUE(start_find_service_result.has_value());
        });

    const auto start_find_service_result_1 = service_discovery_client.StartFindService(
        expected_handle_1,
        [&outer_handler](auto container, auto handle) { outer_handler.AsStdFunction()(container, handle); },
        EnrichedInstanceIdentifier{kInstanceIdentifierAny});
    ASSERT_TRUE(start_find_service_result_1.has_value());

    // Then the service is found both times
    barrier.get_future().wait();
}

}  // namespace
}  // namespace bmw::mw::com::impl::lola
