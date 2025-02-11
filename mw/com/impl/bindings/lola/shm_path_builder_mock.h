// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#ifndef PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_SHM_PATH_BUILDER_MOCK_H
#define PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_SHM_PATH_BUILDER_MOCK_H

#include "platform/aas/mw/com/impl/bindings/lola/i_shm_path_builder.h"

#include <gmock/gmock.h>
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

class ShmPathBuilderMock : public IShmPathBuilder
{
  public:
    MOCK_METHOD(std::string,
                GetControlChannelFileName,
                (LolaServiceInstanceId::InstanceId instance_id, const QualityType),
                (const, override));
    MOCK_METHOD(std::string,
                GetDataChannelFileName,
                (LolaServiceInstanceId::InstanceId instance_id),
                (const, override));
    MOCK_METHOD(std::string,
                GetControlChannelPath,
                (LolaServiceInstanceId::InstanceId instance_id, const QualityType),
                (const, override));
    MOCK_METHOD(std::string, GetDataChannelPath, (LolaServiceInstanceId::InstanceId instance_id), (const, override));
    MOCK_METHOD(std::string, GetDataChannelShmName, (LolaServiceInstanceId::InstanceId instance_id), (const, override));
    MOCK_METHOD(std::string,
                GetControlChannelShmName,
                (LolaServiceInstanceId::InstanceId instance_id, const QualityType),
                (const, override));
};

}  // namespace lola
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_SHM_PATH_BUILDER_MOCK_H
