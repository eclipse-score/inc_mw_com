// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#include "platform/aas/mw/com/impl/bindings/lola/service_discovery/flag_file_crawler.h"

#include "platform/aas/lib/filesystem/factory/filesystem_factory_fake.h"
#include "platform/aas/lib/os/utils/inotify/inotify_instance_mock.h"

#include <gtest/gtest.h>

namespace bmw::mw::com::impl::lola
{

using ::testing::_;
using ::testing::Contains;
using ::testing::IsEmpty;
using ::testing::Not;
using ::testing::Return;

InstanceSpecifier kInstanceSpecifier{InstanceSpecifier::Create("/bla/blub/specifier").value()};
LolaServiceTypeDeployment kServiceId{1U};
ServiceTypeDeployment kServiceTypeDeployment{kServiceId};

LolaServiceInstanceId kInstanceId1{1U};
LolaServiceInstanceId kInstanceId2{2U};

ServiceInstanceDeployment kInstanceDeployment1{make_ServiceIdentifierType("/bla/blub/service1"),
                                               LolaServiceInstanceDeployment{kInstanceId1},
                                               QualityType::kASIL_QM,
                                               kInstanceSpecifier};
ServiceInstanceDeployment kInstanceDeployment2B{make_ServiceIdentifierType("/bla/blub/service1"),
                                                LolaServiceInstanceDeployment{kInstanceId2},
                                                QualityType::kASIL_B,
                                                kInstanceSpecifier};
ServiceInstanceDeployment kInstanceDeployment2QM{make_ServiceIdentifierType("/bla/blub/service1"),
                                                 LolaServiceInstanceDeployment{kInstanceId2},
                                                 QualityType::kASIL_QM,
                                                 kInstanceSpecifier};
ServiceInstanceDeployment kInstanceDeploymentAnyB{make_ServiceIdentifierType("/bla/blub/service1"),
                                                  LolaServiceInstanceDeployment{},
                                                  QualityType::kASIL_B,
                                                  kInstanceSpecifier};
ServiceInstanceDeployment kInstanceDeploymentAnyQM{make_ServiceIdentifierType("/bla/blub/service1"),
                                                   LolaServiceInstanceDeployment{},
                                                   QualityType::kASIL_QM,
                                                   kInstanceSpecifier};
ServiceInstanceDeployment kInstanceDeploymentAnyInvalid{make_ServiceIdentifierType("/bla/blub/service1"),
                                                        LolaServiceInstanceDeployment{},
                                                        QualityType::kInvalid,
                                                        kInstanceSpecifier};
InstanceIdentifier kInstanceIdentifier1{make_InstanceIdentifier(kInstanceDeployment1, kServiceTypeDeployment)};
InstanceIdentifier kInstanceIdentifier2B{make_InstanceIdentifier(kInstanceDeployment2B, kServiceTypeDeployment)};
InstanceIdentifier kInstanceIdentifier2QM{make_InstanceIdentifier(kInstanceDeployment2QM, kServiceTypeDeployment)};
InstanceIdentifier kInstanceIdentifierAnyB{make_InstanceIdentifier(kInstanceDeploymentAnyB, kServiceTypeDeployment)};
InstanceIdentifier kInstanceIdentifierAnyQM{make_InstanceIdentifier(kInstanceDeploymentAnyQM, kServiceTypeDeployment)};
InstanceIdentifier kInstanceIdentifierAnyInvalid{
    make_InstanceIdentifier(kInstanceDeploymentAnyInvalid, kServiceTypeDeployment)};
EnrichedInstanceIdentifier kEnrichedInstanceIdentifier1{kInstanceIdentifier1};
EnrichedInstanceIdentifier kEnrichedInstanceIdentifier1Invalid{kEnrichedInstanceIdentifier1, QualityType::kInvalid};
EnrichedInstanceIdentifier kEnrichedInstanceIdentifier2B{kInstanceIdentifier2B};
EnrichedInstanceIdentifier kEnrichedInstanceIdentifier2QM{kInstanceIdentifier2QM};
EnrichedInstanceIdentifier kEnrichedInstanceIdentifierAnyB{kInstanceIdentifierAnyB};
EnrichedInstanceIdentifier kEnrichedInstanceIdentifierAnyQM{kInstanceIdentifierAnyQM};
EnrichedInstanceIdentifier kEnrichedInstanceIdentifierAnyInvalid{kEnrichedInstanceIdentifierAnyQM,
                                                                 QualityType::kInvalid};
EnrichedInstanceIdentifier kEnrichedInstanceIdentifierAnyQM1{kInstanceIdentifierAnyQM, ServiceInstanceId{kInstanceId1}};
EnrichedInstanceIdentifier kEnrichedInstanceIdentifierAnyInvalid1{kEnrichedInstanceIdentifierAnyQM1,
                                                                  QualityType::kInvalid};
HandleType kHandleTypeAnyQM1{make_HandleType(kInstanceIdentifierAnyQM, kInstanceId1)};
HandleType kHandleTypeAnyB2{make_HandleType(kInstanceIdentifierAnyB, kInstanceId2)};
HandleType kHandleTypeAnyQM2{make_HandleType(kInstanceIdentifierAnyQM, kInstanceId2)};
HandleType kHandleType2B{make_HandleType(kInstanceIdentifier2B)};
HandleType kHandleType2QM{make_HandleType(kInstanceIdentifier2QM)};

filesystem::Perms kUserWriteRestRead{filesystem::Perms::kReadUser | filesystem::Perms::kWriteUser |
                                     filesystem::Perms::kReadGroup | filesystem::Perms::kReadOthers};

class FlagFileCrawlerTest : public ::testing::Test
{
  public:
    void SetUp() override
    {
        filesystem::StandardFilesystem::set_testing_instance(*(filesystem_.standard));
        ON_CALL(inotify_instance_, AddWatch(_, _)).WillByDefault(Return(os::InotifyWatchDescriptor{0}));
    }

    void TearDown() override { filesystem::StandardFilesystem::restore_instance(); }

    filesystem::FilesystemFactoryFake filesystem_factory_fake_{};
    filesystem::Filesystem filesystem_{filesystem_factory_fake_.CreateInstance()};
    os::InotifyInstanceMock inotify_instance_{};

    filesystem::Path GetSearchPath(const EnrichedInstanceIdentifier& instance_identifier)
    {
        if (instance_identifier == kEnrichedInstanceIdentifier1 ||
            instance_identifier == kEnrichedInstanceIdentifierAnyQM1)
        {
            return "/tmp/mw_com_lola/service_discovery/1/1";
        }
        else if (instance_identifier == kEnrichedInstanceIdentifier2B ||
                 instance_identifier == kEnrichedInstanceIdentifier2QM)
        {
            return "/tmp/mw_com_lola/service_discovery/1/2";
        }
        else if (instance_identifier == kEnrichedInstanceIdentifierAnyQM ||
                 instance_identifier == kEnrichedInstanceIdentifierAnyB)
        {
            return "/tmp/mw_com_lola/service_discovery/1";
        }
        else
        {
            std::terminate();
        }
    }

    filesystem::Path GetFlagFilePath(const EnrichedInstanceIdentifier& instance_identifier)
    {
        if (instance_identifier == kEnrichedInstanceIdentifier1)
        {
            return "/tmp/mw_com_lola/service_discovery/1/1/42_asil-qm_1234";
        }
        else if (instance_identifier == kEnrichedInstanceIdentifier2B)
        {
            return "/tmp/mw_com_lola/service_discovery/1/2/43_asil-b_5678";
        }
        else if (instance_identifier == kEnrichedInstanceIdentifier2QM)
        {
            return "/tmp/mw_com_lola/service_discovery/1/2/43_asil-qm_5678";
        }
        else
        {
            std::terminate();
        }
    }

    void CreateSearchPath(const EnrichedInstanceIdentifier& instance_identifier)
    {
        filesystem_factory_fake_.GetStandard().CreateDirectories(GetSearchPath(instance_identifier));
    }

    void CreateFlagFile(const EnrichedInstanceIdentifier& instance_identifier)
    {
        CreateSearchPath(instance_identifier);
        filesystem_factory_fake_.GetStandard().CreateRegularFile(GetFlagFilePath(instance_identifier),
                                                                 kUserWriteRestRead);
    }
};

TEST_F(FlagFileCrawlerTest, AddsWatchForServiceIdIfCrawlingServiceId)
{
    os::InotifyWatchDescriptor expected_descriptor{1};

    const auto search_path = GetSearchPath(kEnrichedInstanceIdentifierAnyQM).Native();
    EXPECT_CALL(
        inotify_instance_,
        AddWatch(amp::string_view{search_path}, os::Inotify::EventMask::kInCreate | os::Inotify::EventMask::kInDelete))
        .WillOnce(Return(expected_descriptor));
    FlagFileCrawler flag_file_crawler{inotify_instance_, filesystem_};
    const auto crawler_result = flag_file_crawler.CrawlAndWatch(kEnrichedInstanceIdentifierAnyQM);
    ASSERT_TRUE(crawler_result.has_value());
    const auto& [descriptors, instances] = crawler_result.value();
    EXPECT_THAT(descriptors, Contains(std::make_pair(expected_descriptor, kEnrichedInstanceIdentifierAnyInvalid)));
}

TEST_F(FlagFileCrawlerTest, AddsWatchForExistingInstanceIdIfCrawlingServiceId)
{
    CreateSearchPath(kEnrichedInstanceIdentifier1);

    os::InotifyWatchDescriptor service_expected_descriptor{1};
    const auto service_search_path = GetSearchPath(kEnrichedInstanceIdentifierAnyQM).Native();
    EXPECT_CALL(inotify_instance_,
                AddWatch(amp::string_view{service_search_path},
                         os::Inotify::EventMask::kInCreate | os::Inotify::EventMask::kInDelete))
        .WillOnce(Return(service_expected_descriptor));

    os::InotifyWatchDescriptor instance_expected_descriptor{2};
    const auto instance_search_path = GetSearchPath(kEnrichedInstanceIdentifierAnyQM1).Native();
    EXPECT_CALL(inotify_instance_,
                AddWatch(amp::string_view{instance_search_path},
                         os::Inotify::EventMask::kInCreate | os::Inotify::EventMask::kInDelete))
        .WillOnce(Return(instance_expected_descriptor));

    FlagFileCrawler flag_file_crawler{inotify_instance_, filesystem_};
    const auto crawler_result = flag_file_crawler.CrawlAndWatch(kEnrichedInstanceIdentifierAnyQM);
    ASSERT_TRUE(crawler_result.has_value());
    const auto& [descriptors, instances] = crawler_result.value();
    EXPECT_THAT(descriptors,
                Contains(std::make_pair(service_expected_descriptor, kEnrichedInstanceIdentifierAnyInvalid)));
    EXPECT_THAT(descriptors,
                Contains(std::make_pair(instance_expected_descriptor, kEnrichedInstanceIdentifierAnyInvalid1)));
}

TEST_F(FlagFileCrawlerTest, AddsWatchForInstanceIdIfCrawlingInstanceId)
{
    os::InotifyWatchDescriptor expected_descriptor{1};

    const auto search_path = GetSearchPath(kEnrichedInstanceIdentifier1).Native();
    EXPECT_CALL(inotify_instance_,
                AddWatch({search_path}, os::Inotify::EventMask::kInCreate | os::Inotify::EventMask::kInDelete))
        .WillOnce(Return(expected_descriptor));
    FlagFileCrawler flag_file_crawler{inotify_instance_, filesystem_};
    const auto crawler_result = flag_file_crawler.CrawlAndWatch(kEnrichedInstanceIdentifier1);
    ASSERT_TRUE(crawler_result.has_value());
    const auto& [descriptors, instances] = crawler_result.value();
    EXPECT_THAT(descriptors, Contains(std::make_pair(expected_descriptor, kEnrichedInstanceIdentifier1Invalid)));
}

TEST_F(FlagFileCrawlerTest, ReturnsEmptyContainersIfNoInstancesFoundForServiceId)
{
    FlagFileCrawler flag_file_crawler{inotify_instance_, filesystem_};
    const auto crawler_result = flag_file_crawler.CrawlAndWatch(kEnrichedInstanceIdentifierAnyQM);
    ASSERT_TRUE(crawler_result.has_value());
    const auto& [descriptors, instances] = crawler_result.value();
    EXPECT_TRUE(instances.asil_b.Empty());
    EXPECT_TRUE(instances.asil_qm.Empty());
}

TEST_F(FlagFileCrawlerTest, ReturnsEmptyContainersIfNoInstancesFoundForInstanceId)
{
    FlagFileCrawler flag_file_crawler{inotify_instance_, filesystem_};
    const auto crawler_result = flag_file_crawler.CrawlAndWatch(kEnrichedInstanceIdentifier1);
    ASSERT_TRUE(crawler_result.has_value());
    const auto& [descriptors, instances] = crawler_result.value();
    EXPECT_TRUE(instances.asil_b.Empty());
    EXPECT_TRUE(instances.asil_qm.Empty());
}

TEST_F(FlagFileCrawlerTest, ReturnsAllInstancesInCorrectContainersIfFoundForAsilQmServiceId)
{
    CreateFlagFile(kEnrichedInstanceIdentifier1);
    CreateFlagFile(kEnrichedInstanceIdentifier2B);
    CreateFlagFile(kEnrichedInstanceIdentifier2QM);

    FlagFileCrawler flag_file_crawler{inotify_instance_, filesystem_};
    const auto crawler_result = flag_file_crawler.CrawlAndWatch(kEnrichedInstanceIdentifierAnyQM);
    ASSERT_TRUE(crawler_result.has_value());
    const auto& [descriptors, instances] = crawler_result.value();
    const auto asil_b_handles = instances.asil_b.GetKnownHandles(kEnrichedInstanceIdentifierAnyB);
    EXPECT_THAT(asil_b_handles, Contains(kHandleTypeAnyB2));
    const auto asil_qm_handles = instances.asil_qm.GetKnownHandles(kEnrichedInstanceIdentifierAnyQM);
    EXPECT_THAT(asil_qm_handles, Contains(kHandleTypeAnyQM1));
    EXPECT_THAT(asil_qm_handles, Contains(kHandleTypeAnyQM2));
    EXPECT_THAT(asil_qm_handles, Not(Contains(kHandleTypeAnyB2)));
}

TEST_F(FlagFileCrawlerTest, ReturnsAllInstancesInCorrectContainersIfFoundForAsilBServiceId)
{
    CreateFlagFile(kEnrichedInstanceIdentifier2B);
    CreateFlagFile(kEnrichedInstanceIdentifier2QM);

    FlagFileCrawler flag_file_crawler{inotify_instance_, filesystem_};
    const auto crawler_result = flag_file_crawler.CrawlAndWatch(kEnrichedInstanceIdentifierAnyB);
    ASSERT_TRUE(crawler_result.has_value());
    const auto& [descriptors, instances] = crawler_result.value();
    const auto asil_qm_handles = instances.asil_b.GetKnownHandles(kEnrichedInstanceIdentifierAnyB);
    EXPECT_THAT(asil_qm_handles, Contains(kHandleTypeAnyB2));
    EXPECT_THAT(asil_qm_handles, Not(Contains(kHandleTypeAnyQM2)));
}

TEST_F(FlagFileCrawlerTest, ReturnsAllInstancesInCorrectContainersIfFoundForAsilQmInstanceId)
{
    CreateFlagFile(kEnrichedInstanceIdentifier1);
    CreateFlagFile(kEnrichedInstanceIdentifier2B);
    CreateFlagFile(kEnrichedInstanceIdentifier2QM);

    FlagFileCrawler flag_file_crawler{inotify_instance_, filesystem_};
    const auto crawler_result = flag_file_crawler.CrawlAndWatch(kEnrichedInstanceIdentifier2QM);
    ASSERT_TRUE(crawler_result.has_value());
    const auto& [descriptors, instances] = crawler_result.value();
    const auto asil_qm_handles = instances.asil_qm.GetKnownHandles(kEnrichedInstanceIdentifierAnyQM);
    EXPECT_THAT(asil_qm_handles, Not(Contains(kHandleTypeAnyB2)));
    EXPECT_THAT(asil_qm_handles, Not(Contains(kHandleTypeAnyQM1)));
    EXPECT_THAT(asil_qm_handles, Contains(kHandleTypeAnyQM2));
    const auto asil_b_handles = instances.asil_b.GetKnownHandles(kEnrichedInstanceIdentifierAnyB);
    EXPECT_THAT(asil_b_handles, Contains(kHandleTypeAnyB2));
}

TEST_F(FlagFileCrawlerTest, ReturnsAllInstancesInCorrectContainersIfFoundForAsilBInstanceId)
{
    CreateFlagFile(kEnrichedInstanceIdentifier1);
    CreateFlagFile(kEnrichedInstanceIdentifier2B);
    CreateFlagFile(kEnrichedInstanceIdentifier2QM);

    FlagFileCrawler flag_file_crawler{inotify_instance_, filesystem_};
    const auto crawler_result = flag_file_crawler.CrawlAndWatch(kEnrichedInstanceIdentifier2QM);
    ASSERT_TRUE(crawler_result.has_value());
    const auto& [descriptors, instances] = crawler_result.value();
    const auto asil_qm_handles = instances.asil_qm.GetKnownHandles(kEnrichedInstanceIdentifier2QM);
    EXPECT_THAT(asil_qm_handles, Contains(kHandleType2QM));
    const auto asil_b_handles = instances.asil_b.GetKnownHandles(kEnrichedInstanceIdentifier2B);
    EXPECT_THAT(asil_b_handles, Contains(kHandleType2B));
}

TEST_F(FlagFileCrawlerTest, IgnoresDirectoriesInInstanceIdDirectories)
{
    CreateFlagFile(kEnrichedInstanceIdentifier1);

    // We create a flag file in the subdirectory for an ASIL-B offer that we expect to NOT find below
    const auto broken_path = GetSearchPath(kEnrichedInstanceIdentifier1) / "1234_asil-b_5678";
    filesystem_factory_fake_.GetStandard().CreateDirectories(broken_path);
    filesystem_factory_fake_.GetStandard().CreateRegularFile(broken_path / "1234_asil-b_5678", kUserWriteRestRead);

    FlagFileCrawler flag_file_crawler{inotify_instance_, filesystem_};
    const auto crawler_result = flag_file_crawler.CrawlAndWatch(kEnrichedInstanceIdentifierAnyQM);
    ASSERT_TRUE(crawler_result.has_value());
    const auto& [descriptors, instances] = crawler_result.value();
    EXPECT_TRUE(instances.asil_b.Empty());
    const auto asil_qm_handles = instances.asil_qm.GetKnownHandles(kEnrichedInstanceIdentifierAnyQM1);
    EXPECT_THAT(asil_qm_handles, Contains(kHandleTypeAnyQM1));
}

TEST_F(FlagFileCrawlerTest, ReturnsErrorIfInitialWatchDirectoryCouldNotBeCreated)
{
    const auto search_path = GetSearchPath(kEnrichedInstanceIdentifierAnyQM);
    ON_CALL(filesystem_factory_fake_.GetUtils(), CreateDirectories(search_path, _))
        .WillByDefault(Return(MakeUnexpected(filesystem::ErrorCode::kNotImplemented)));

    FlagFileCrawler flag_file_crawler{inotify_instance_, filesystem_};
    const auto crawler_result = flag_file_crawler.CrawlAndWatch(kEnrichedInstanceIdentifierAnyQM);
    ASSERT_FALSE(crawler_result.has_value());
    EXPECT_EQ(crawler_result.error(), ComErrc::kBindingFailure);
}

TEST_F(FlagFileCrawlerTest, ReturnsErrorIfInitialWatchCouldNotBeCreated)
{
    const auto search_path = GetSearchPath(kEnrichedInstanceIdentifierAnyQM).Native();
    ON_CALL(inotify_instance_, AddWatch({search_path}, _))
        .WillByDefault(Return(amp::make_unexpected(os::Error::createFromErrno(EINTR))));

    FlagFileCrawler flag_file_crawler{inotify_instance_, filesystem_};
    const auto crawler_result = flag_file_crawler.CrawlAndWatch(kEnrichedInstanceIdentifierAnyQM);
    ASSERT_FALSE(crawler_result.has_value());
    EXPECT_EQ(crawler_result.error(), ComErrc::kBindingFailure);
}

TEST_F(FlagFileCrawlerTest, ReturnsErrorIfSubdirectoryWatchCouldNotBeCreated)
{
    CreateFlagFile(kEnrichedInstanceIdentifier1);
    const auto search_path = GetSearchPath(kEnrichedInstanceIdentifier1).Native();
    ON_CALL(inotify_instance_, AddWatch({search_path}, _))
        .WillByDefault(Return(amp::make_unexpected(os::Error::createFromErrno(EINTR))));

    FlagFileCrawler flag_file_crawler{inotify_instance_, filesystem_};
    const auto crawler_result = flag_file_crawler.CrawlAndWatch(kEnrichedInstanceIdentifierAnyInvalid);
    ASSERT_FALSE(crawler_result.has_value());
    EXPECT_EQ(crawler_result.error(), ComErrc::kBindingFailure);
}

TEST_F(FlagFileCrawlerTest, ReturnsErrorIfSubdirectoryStatusCouldNotBeRetrieved)
{
    CreateFlagFile(kEnrichedInstanceIdentifier1);
    const auto search_path = GetSearchPath(kEnrichedInstanceIdentifier1).Native();
    ON_CALL(filesystem_factory_fake_.GetStandard(), Status(GetSearchPath(kEnrichedInstanceIdentifier1)))
        .WillByDefault(Return(MakeUnexpected(filesystem::ErrorCode::kNotImplemented)));

    FlagFileCrawler flag_file_crawler{inotify_instance_, filesystem_};
    const auto crawler_result = flag_file_crawler.CrawlAndWatch(kEnrichedInstanceIdentifierAnyInvalid);
    ASSERT_FALSE(crawler_result.has_value());
    EXPECT_EQ(crawler_result.error(), ComErrc::kBindingFailure);
}

TEST_F(FlagFileCrawlerTest, IgnoresFilesOnInstanceIdDirectoryLevel)
{
    const filesystem::Path service_path{"/tmp/mw_com_lola/service_discovery/1"};
    filesystem_factory_fake_.GetStandard().CreateDirectories(service_path);
    filesystem_factory_fake_.GetStandard().CreateRegularFile(service_path / "1", kUserWriteRestRead);
    CreateFlagFile(kEnrichedInstanceIdentifier2B);

    FlagFileCrawler flag_file_crawler{inotify_instance_, filesystem_};
    const auto crawler_result = flag_file_crawler.CrawlAndWatch(kEnrichedInstanceIdentifierAnyInvalid);
    ASSERT_TRUE(crawler_result.has_value());
    const auto& [descriptors, instances] = crawler_result.value();
    EXPECT_TRUE(instances.asil_qm.Empty());
    const auto asil_b_handles = instances.asil_b.GetKnownHandles(kEnrichedInstanceIdentifier2B);
    EXPECT_THAT(asil_b_handles, Contains(kHandleType2B));
}

TEST_F(FlagFileCrawlerTest, IgnoresDirectoryOnInstanceIdIfCannotBeParsedToInstanceId)
{
    const filesystem::Path service_path{"/tmp/mw_com_lola/service_discovery/whatever"};
    filesystem_factory_fake_.GetStandard().CreateDirectories(service_path);
    filesystem_factory_fake_.GetStandard().CreateRegularFile(service_path / "1", kUserWriteRestRead);
    CreateFlagFile(kEnrichedInstanceIdentifier2B);

    FlagFileCrawler flag_file_crawler{inotify_instance_, filesystem_};
    const auto crawler_result = flag_file_crawler.CrawlAndWatch(kEnrichedInstanceIdentifierAnyInvalid);
    ASSERT_TRUE(crawler_result.has_value());
    const auto& [descriptors, instances] = crawler_result.value();
    EXPECT_TRUE(instances.asil_qm.Empty());
    const auto asil_b_handles = instances.asil_b.GetKnownHandles(kEnrichedInstanceIdentifier2B);
    EXPECT_THAT(asil_b_handles, Contains(kHandleType2B));
}

}  // namespace bmw::mw::com::impl::lola
