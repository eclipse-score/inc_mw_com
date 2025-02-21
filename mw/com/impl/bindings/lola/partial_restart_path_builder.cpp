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

#include "platform/aas/mw/com/impl/bindings/lola/path_builder.h"

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
constexpr auto kPartialRestartDir{"partial_restart/"};
#if defined(__QNXNTO__)
constexpr auto kTmpPathPrefix{"/tmp_discovery/"};
#else
constexpr auto kTmpPathPrefix{"/tmp/"};
#endif
constexpr auto kServiceUsageMarkerFileTag{"usage-"};
constexpr auto kServiceExistenceMarkerFileTag{"existence-"};

/// Emit file name of the service instance existence marker file to an ostream
///
/// \param out output ostream to use
void EmitServiceInstanceExistenceMarkerFileName(std::ostream& out,
                                                const std::uint16_t service_id,
                                                const LolaServiceInstanceId::InstanceId instance_id) noexcept
{
    out << kServiceExistenceMarkerFileTag;
    AppendServiceAndInstance(out, service_id, instance_id);
}

/// Emit file name of the service instance usage marker file to an ostream
///
/// \param out output ostream to use
void EmitServiceInstanceUsageMarkerFileName(std::ostream& out,
                                            const std::uint16_t service_id,
                                            const LolaServiceInstanceId::InstanceId instance_id) noexcept
{
    out << kServiceUsageMarkerFileTag;
    AppendServiceAndInstance(out, service_id, instance_id);
}

}  // namespace

std::string PartialRestartPathBuilder::GetServiceInstanceExistenceMarkerFilePath(
    const LolaServiceInstanceId::InstanceId instance_id) const
{
    return EmitWithPrefix(
        std::string{kTmpPathPrefix} + std::string{kLoLaDir} + std::string{kPartialRestartDir},
        [this, instance_id](auto& out) { EmitServiceInstanceExistenceMarkerFileName(out, service_id_, instance_id); });
}

std::string PartialRestartPathBuilder::GetServiceInstanceUsageMarkerFilePath(
    const LolaServiceInstanceId::InstanceId instance_id) const
{
    return EmitWithPrefix(
        std::string{kTmpPathPrefix} + std::string{kLoLaDir} + std::string{kPartialRestartDir},
        [this, instance_id](auto& out) { EmitServiceInstanceUsageMarkerFileName(out, service_id_, instance_id); });
}

std::string PartialRestartPathBuilder::GetLolaPartialRestartDirectoryPath() const
{
    return std::string{kTmpPathPrefix} + std::string{kLoLaDir} + std::string{kPartialRestartDir};
}

}  // namespace lola
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
