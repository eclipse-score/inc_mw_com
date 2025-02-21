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


#ifndef PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_I_SHM_PATH_BUILDER_H
#define PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_I_SHM_PATH_BUILDER_H

#include "platform/aas/mw/com/impl/configuration/lola_service_instance_id.h"
#include "platform/aas/mw/com/impl/configuration/quality_type.h"

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
/// The instance is identified by its LolaServiceInstanceDeployment. This object must outlive the lifetime
/// of ShmPathBuilder since the reference is stored inside the class.
class IShmPathBuilder
{
  public:
    virtual ~IShmPathBuilder() = default;

    /// Returns the file name to the control shared memory file.
    ///
    /// \param asil_b Whether to return the ASIL QM or ASIL B path.
    /// \return The name of the file
    virtual std::string GetControlChannelFileName(const LolaServiceInstanceId::InstanceId instance_id,
                                                  const QualityType channel_type) const = 0;

    /// Returns the path to the data shared memory file.
    ///
    /// \return The name of the file
    virtual std::string GetDataChannelFileName(const LolaServiceInstanceId::InstanceId instance_id) const = 0;

    /// Returns the file path to the control shared memory file.
    ///
    /// \param asil_b Whether to return the ASIL QM or ASIL B path.
    /// \return The path to the file
    virtual std::string GetControlChannelPath(const LolaServiceInstanceId::InstanceId instance_id,
                                              const QualityType channel_type) const = 0;

    /// Returns the file path to the data shared memory file.
    ///
    /// \return The path to the file
    virtual std::string GetDataChannelPath(const LolaServiceInstanceId::InstanceId instance_id) const = 0;

    /// Returns the path suitable for shm_open to the data shared memory.
    ///
    /// \return The shm file name
    virtual std::string GetDataChannelShmName(const LolaServiceInstanceId::InstanceId instance_id) const = 0;

    /// Returns the path suitable for shm_open to the control shared memory.
    ///
    /// \param asil_b Whether to return the ASIL QM or ASIL B path.
    /// \return The shm file name
    virtual std::string GetControlChannelShmName(const LolaServiceInstanceId::InstanceId instance_id,
                                                 const QualityType channel_type) const = 0;
};

}  // namespace lola
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_I_SHM_PATH_BUILDER_H
