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


#ifndef PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_I_PARTIAL_RESTART_PATH_BUILDER_H
#define PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_I_PARTIAL_RESTART_PATH_BUILDER_H

#include "platform/aas/mw/com/impl/configuration/lola_service_instance_id.h"

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

/// \brief Utility class to generate paths related to Partial Restart
class IPartialRestartPathBuilder
{
  public:
    virtual ~IPartialRestartPathBuilder() = default;

    /// Returns the path for the lock file used to indicate existence of service instance.
    virtual std::string GetServiceInstanceExistenceMarkerFilePath(
        const LolaServiceInstanceId::InstanceId instance_id) const = 0;

    /// Returns the path for the lock file used to indicate usage of service instance.
    virtual std::string GetServiceInstanceUsageMarkerFilePath(
        const LolaServiceInstanceId::InstanceId instance_id) const = 0;

    /// Returns the path for folder where partial restart specific files shall be stored.
    virtual std::string GetLolaPartialRestartDirectoryPath() const = 0;
};

}  // namespace lola
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_I_PARTIAL_RESTART_PATH_BUILDER_H
