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

#include "platform/aas/mw/com/impl/bindings/lola/path_builder.h"

#include <amp_string_view.hpp>

#include <iomanip>
#include <sstream>
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

constexpr auto kDataChannelPrefix{"lola-data-"};
constexpr auto kControlChannelPrefix{"lola-ctl-"};
constexpr auto kAsilBControlChannelSuffix{"-b"};
#if defined(__QNXNTO__)
constexpr auto kSharedMemoryPathPrefix = "/dev/shmem/";
#else
constexpr auto kSharedMemoryPathPrefix = "/dev/shm/";
#endif

/// Emit file name of the control file to an ostream
///
/// \param out output ostream to use
/// \param channel_type ASIL level of the channel
/// \param instance_id InstanceId of path to be created
void EmitControlFileName(std::ostream& out,
                         const QualityType channel_type,
                         const std::uint16_t service_id,
                         const LolaServiceInstanceId::InstanceId instance_id) noexcept
{
    out << kControlChannelPrefix;
    AppendServiceAndInstance(out, service_id, instance_id);

    switch (channel_type)
    {
        case QualityType::kASIL_QM:
        {
            return;
        }
        case QualityType::kASIL_B:
        {
            out << kAsilBControlChannelSuffix;
            return;
        }
        case QualityType::kInvalid:
        default:
        {
            AMP_ASSERT_PRD_MESSAGE(0, "Invalid quality type");
        }
    }
}

/// Emit file name of the data file to an ostream
///
/// \param out output ostream to use
void EmitDataFileName(std::ostream& out,
                      const std::uint16_t service_id,
                      const LolaServiceInstanceId::InstanceId instance_id) noexcept
{
    out << kDataChannelPrefix;
    AppendServiceAndInstance(out, service_id, instance_id);
}

}  // namespace

std::string ShmPathBuilder::GetControlChannelFileName(const LolaServiceInstanceId::InstanceId instance_id,
                                                      const QualityType channel_type) const
{
    return EmitWithPrefix("", [this, channel_type, instance_id](auto& out) noexcept {
        EmitControlFileName(out, channel_type, service_id_, instance_id);
    });
}

std::string ShmPathBuilder::GetDataChannelFileName(const LolaServiceInstanceId::InstanceId instance_id) const
{
    return EmitWithPrefix("",
                          [this, instance_id](auto& out) noexcept { EmitDataFileName(out, service_id_, instance_id); });
}

std::string ShmPathBuilder::GetControlChannelPath(const LolaServiceInstanceId::InstanceId instance_id,
                                                  const QualityType channel_type) const
{
    return EmitWithPrefix(kSharedMemoryPathPrefix, [this, channel_type, instance_id](auto& out) noexcept {
        EmitControlFileName(out, channel_type, service_id_, instance_id);
    });
}

std::string ShmPathBuilder::GetDataChannelPath(const LolaServiceInstanceId::InstanceId instance_id) const
{
    return EmitWithPrefix(kSharedMemoryPathPrefix,
                          [this, instance_id](auto& out) noexcept { EmitDataFileName(out, service_id_, instance_id); });
}

std::string ShmPathBuilder::GetDataChannelShmName(const LolaServiceInstanceId::InstanceId instance_id) const
{
    return EmitWithPrefix('/', [this, instance_id](auto& out) { EmitDataFileName(out, service_id_, instance_id); });
}

std::string ShmPathBuilder::GetControlChannelShmName(const LolaServiceInstanceId::InstanceId instance_id,
                                                     const QualityType channel_type) const
{
    return EmitWithPrefix('/', [this, channel_type, instance_id](auto& out) noexcept {
        EmitControlFileName(out, channel_type, service_id_, instance_id);
    });
}

std::string ShmPathBuilder::GetPrefixContainingControlChannelAndServiceId(const std::uint16_t service_id)
{
    std::stringstream out{};
    out << kControlChannelPrefix;
    AppendService(out, service_id);
    return out.str();
}

std::string ShmPathBuilder::GetAsilBSuffix()
{
    return kAsilBControlChannelSuffix;
}

std::string ShmPathBuilder::GetSharedMemoryPrefix()
{
    return kSharedMemoryPathPrefix;
}

}  // namespace lola
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
