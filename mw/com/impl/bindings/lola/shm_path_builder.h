// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#ifndef PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_SHM_PATH_BUILDER_H
#define PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_SHM_PATH_BUILDER_H

#include "platform/aas/mw/com/impl/bindings/lola/i_shm_path_builder.h"
#include "platform/aas/mw/com/impl/configuration/lola_service_instance_id.h"
#include "platform/aas/mw/com/impl/configuration/quality_type.h"

#include <amp_optional.hpp>

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

/// Utility class to generate paths to the shm files.
///
/// There are up to three files per instance:
/// - The QM control file
/// - The ASIL B control file
/// - The data storage file
///
/// This class should be used to generate the paths to the files so that they can be mapped
/// into the processes address space for further usage.
///
/// The instance is identified by its service_id and instance_id.
class ShmPathBuilder : public IShmPathBuilder
{
  public:
    /// Create a path builder.
    ///
    /// \param service_id ServiceId of path to be created
    explicit ShmPathBuilder(std::uint16_t service_id) noexcept : IShmPathBuilder{}, service_id_{service_id} {}

    /// Returns the file name to the control shared memory file.
    ///
    /// \param instance_id InstanceId of path to be created
    /// \param channel_type Whether to return the ASIL QM or ASIL B name.
    /// \return The name of the file, or nullopt if the binding didn't contain a valid instance
    std::string GetControlChannelFileName(const LolaServiceInstanceId::InstanceId instance_id,
                                          const QualityType channel_type) const override;

    /// Returns the path to the data shared memory file.
    ///
    /// \param instance_id InstanceId of path to be created
    /// \return The name of the file
    std::string GetDataChannelFileName(const LolaServiceInstanceId::InstanceId instance_id) const override;

    /// Returns the file path to the control shared memory file.
    ///
    /// \param instance_id InstanceId of path to be created
    /// \param channel_type Whether to return the ASIL QM or ASIL B path.
    /// \return The path to the file, or nullopt if the binding didn't contain a valid instance

    std::string GetControlChannelPath(const LolaServiceInstanceId::InstanceId instance_id,
                                      const QualityType channel_type) const override;

    /// Returns the file path to the data shared memory file.
    ///
    /// \param instance_id InstanceId of path to be created
    /// \return The path to the file
    std::string GetDataChannelPath(const LolaServiceInstanceId::InstanceId instance_id) const override;

    /// Returns the path suitable for shm_open to the data shared memory.
    ///
    /// \param instance_id InstanceId of path to be created
    /// \return The shm file name
    std::string GetDataChannelShmName(const LolaServiceInstanceId::InstanceId instance_id) const override;

    /// Returns the path suitable for shm_open to the control shared memory.
    ///
    /// \param instance_id InstanceId of path to be created
    /// \param channel_type Whether to return the ASIL QM or ASIL B name.
    /// \return The shm file name, or nullopt if the binding didn't contain a valid instance
    std::string GetControlChannelShmName(const LolaServiceInstanceId::InstanceId instance_id,
                                         const QualityType channel_type) const override;

    static std::string GetPrefixContainingControlChannelAndServiceId(const std::uint16_t service_id);
    static std::string GetAsilBSuffix();
    static std::string GetSharedMemoryPrefix();

  private:
    const std::uint16_t service_id_;
};

}  // namespace lola
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_SHM_PATH_BUILDER_H
