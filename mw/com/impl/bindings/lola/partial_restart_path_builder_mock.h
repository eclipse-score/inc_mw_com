// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#ifndef PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_PARTIAL_RESTART_PATH_BUILDER_MOCK_H
#define PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_PARTIAL_RESTART_PATH_BUILDER_MOCK_H

#include "platform/aas/mw/com/impl/bindings/lola/i_partial_restart_path_builder.h"

#include <gmock/gmock.h>

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

class PartialRestartPathBuilderMock : public IPartialRestartPathBuilder
{
  public:
    MOCK_METHOD(std::string,
                GetServiceInstanceExistenceMarkerFilePath,
                (LolaServiceInstanceId::InstanceId instance_id),
                (const, override));
    MOCK_METHOD(std::string,
                GetServiceInstanceUsageMarkerFilePath,
                (LolaServiceInstanceId::InstanceId instance_id),
                (const, override));
    MOCK_METHOD(std::string, GetLolaPartialRestartDirectoryPath, (), (const, override));
};

}  // namespace lola
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_PARTIAL_RESTART_PATH_BUILDER_MOCK_H
