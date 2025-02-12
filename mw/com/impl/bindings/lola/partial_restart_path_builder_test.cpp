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


#include "platform/aas/mw/com/impl/bindings/lola/partial_restart_path_builder.h"

#include <gtest/gtest.h>
#include <cstdint>
#include <string>

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

constexpr auto kLoLaDir{"mw_com_lola/"};
#if defined(__QNXNTO__)
const std::string kTmpPathPrefix{"/tmp_discovery/"};
#else
const std::string kTmpPathPrefix{"/tmp/"};
#endif
constexpr auto kServiceUsageMarkerFileTag{"usage-"};
constexpr auto kServiceExistenceMarkerFileTag{"existence-"};
constexpr auto kPartialRestartDir{"partial_restart/"};

const std::uint16_t kServiceId{0x1234};
const LolaServiceInstanceId::InstanceId kInstanceId{0xABCD};

TEST(PartialRestartPathBuilderTest, BuildPaths)
{
    const PartialRestartPathBuilder builder{kServiceId};

    const std::string full_existence_marker_file_path =
        kTmpPathPrefix + kLoLaDir + kPartialRestartDir + kServiceExistenceMarkerFileTag + "0000000000004660-43981";
    EXPECT_STREQ(full_existence_marker_file_path.data(),
                 builder.GetServiceInstanceExistenceMarkerFilePath(kInstanceId).c_str());

    const std::string full_usage_marker_file_path =
        kTmpPathPrefix + kLoLaDir + kPartialRestartDir + kServiceUsageMarkerFileTag + "0000000000004660-43981";
    EXPECT_STREQ(full_usage_marker_file_path.data(),
                 builder.GetServiceInstanceUsageMarkerFilePath(kInstanceId).c_str());
}

TEST(PartialRestartPathBuilderTest, BuildPathswithLeadingZeroes)
{
    const LolaServiceInstanceId::InstanceId instance_id{1};
    const PartialRestartPathBuilder builder{kServiceId};

    const std::string full_existence_marker_file_path =
        kTmpPathPrefix + kLoLaDir + kPartialRestartDir + kServiceExistenceMarkerFileTag + "0000000000004660-00001";
    EXPECT_STREQ(full_existence_marker_file_path.data(),
                 builder.GetServiceInstanceExistenceMarkerFilePath(instance_id).c_str());

    const std::string full_usage_marker_file_path =
        kTmpPathPrefix + kLoLaDir + kPartialRestartDir + kServiceUsageMarkerFileTag + "0000000000004660-00001";
    EXPECT_STREQ(full_usage_marker_file_path.data(),
                 builder.GetServiceInstanceUsageMarkerFilePath(instance_id).c_str());
}

TEST(PartialRestartPathBuilderTest, GetLolaPartialRestartDirectoryPathWorks)
{
#if defined(__QNXNTO__)
    constexpr auto lola_dir = "/tmp_discovery/mw_com_lola/partial_restart/";
#else
    constexpr auto lola_dir = "/tmp/mw_com_lola/partial_restart/";
#endif

    const PartialRestartPathBuilder builder{kServiceId};
    const auto dir_path = builder.GetLolaPartialRestartDirectoryPath();
    EXPECT_STREQ(dir_path.c_str(), lola_dir);
}

}  // namespace
}  // namespace lola
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
