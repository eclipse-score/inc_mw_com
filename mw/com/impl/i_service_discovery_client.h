// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#ifndef PLATFORM_AAS_MW_COM_IMPL_I_SERVICE_DISCOVERY_CLIENT_H
#define PLATFORM_AAS_MW_COM_IMPL_I_SERVICE_DISCOVERY_CLIENT_H

#include "platform/aas/mw/com/impl/enriched_instance_identifier.h"
#include "platform/aas/mw/com/impl/find_service_handle.h"
#include "platform/aas/mw/com/impl/find_service_handler.h"
#include "platform/aas/mw/com/impl/handle_type.h"
#include "platform/aas/mw/com/impl/i_service_discovery.h"
#include "platform/aas/mw/com/impl/instance_identifier.h"

#include "platform/aas/lib/result/result.h"

namespace bmw
{
namespace mw
{
namespace com
{
namespace impl
{

class IServiceDiscoveryClient
{
  public:
    virtual ~IServiceDiscoveryClient() = default;

    [[nodiscard]] virtual ResultBlank OfferService(InstanceIdentifier) noexcept = 0;
    [[nodiscard]] virtual ResultBlank StopOfferService(
        InstanceIdentifier,
        IServiceDiscovery::QualityTypeSelector quality_type) noexcept = 0;
    [[nodiscard]] virtual ResultBlank StartFindService(FindServiceHandle,
                                                       FindServiceHandler<HandleType>,
                                                       EnrichedInstanceIdentifier) noexcept = 0;
    [[nodiscard]] virtual ResultBlank StopFindService(FindServiceHandle) noexcept = 0;
    [[nodiscard]] virtual Result<ServiceHandleContainer<HandleType>> FindService(
        EnrichedInstanceIdentifier) noexcept = 0;
};

}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_I_SERVICE_DISCOVERY_CLIENT_H
