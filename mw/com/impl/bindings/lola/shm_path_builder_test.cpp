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


#include "platform/aas/mw/com/impl/bindings/lola/shm_path_builder.h"

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

const std::uint16_t kServiceId{4660};
const LolaServiceInstanceId::InstanceId kInstanceId{43981};
TEST(ShmPathBuilderTest, BuildPaths)
{
    ShmPathBuilder builder{kServiceId};
    EXPECT_STREQ("lola-ctl-0000000000004660-43981",
                 builder.GetControlChannelFileName(kInstanceId, QualityType::kASIL_QM).c_str());
    EXPECT_STREQ("lola-ctl-0000000000004660-43981-b",
                 builder.GetControlChannelFileName(kInstanceId, QualityType::kASIL_B).c_str());
    EXPECT_STREQ("lola-data-0000000000004660-43981", builder.GetDataChannelFileName(kInstanceId).c_str());
    EXPECT_STREQ("/lola-ctl-0000000000004660-43981",
                 builder.GetControlChannelShmName(kInstanceId, QualityType::kASIL_QM).c_str());
    EXPECT_STREQ("/lola-ctl-0000000000004660-43981-b",
                 builder.GetControlChannelShmName(kInstanceId, QualityType::kASIL_B).c_str());
    EXPECT_STREQ("/lola-data-0000000000004660-43981", builder.GetDataChannelShmName(kInstanceId).c_str());
}

TEST(ShmPathBuilderTest, BuildPathswithLeadingZeroes)
{
    const LolaServiceInstanceId::InstanceId instance_id{1};
    ShmPathBuilder builder{kServiceId};
    EXPECT_STREQ("lola-ctl-0000000000004660-00001",
                 builder.GetControlChannelFileName(instance_id, QualityType::kASIL_QM).c_str());
    EXPECT_STREQ("lola-ctl-0000000000004660-00001-b",
                 builder.GetControlChannelFileName(instance_id, QualityType::kASIL_B).c_str());
    EXPECT_STREQ("lola-data-0000000000004660-00001", builder.GetDataChannelFileName(instance_id).c_str());
}

TEST(ShmPathBuilderTest, GetPrefixContainingControlChannelAndServiceIdWorks)
{
    EXPECT_STREQ("lola-ctl-0000000000004660-",
                 ShmPathBuilder::GetPrefixContainingControlChannelAndServiceId(4660).c_str());
}

TEST(ShmPathBuilderTest, GetAsilBSuffixWorks)
{
    EXPECT_STREQ(ShmPathBuilder::GetAsilBSuffix().c_str(), "-b");
}

TEST(ShmPathBuilderTest, GetSharedMemoryPrefixWorks)
{
#if defined(__QNXNTO__)
    constexpr auto kSharedMemoryPathPrefix = "/dev/shmem/";
#else
    constexpr auto kSharedMemoryPathPrefix = "/dev/shm/";
#endif
    EXPECT_STREQ(ShmPathBuilder::GetSharedMemoryPrefix().c_str(), kSharedMemoryPathPrefix);
}

TEST(ShmPathBuilderDeathTest, BuildingPathsWithInvalidQualityTypeTerminates)
{
    ShmPathBuilder builder{kServiceId};
    EXPECT_DEATH(builder.GetControlChannelFileName(kInstanceId, QualityType::kInvalid), ".*");
}

}  // namespace
}  // namespace lola
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
