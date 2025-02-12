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


#ifndef PLATFORM_AAS_MW_COM_IMPL_I_SERVICE_DISCOVERY_H
#define PLATFORM_AAS_MW_COM_IMPL_I_SERVICE_DISCOVERY_H

#include "platform/aas/mw/com/impl/enriched_instance_identifier.h"
#include "platform/aas/mw/com/impl/find_service_handle.h"
#include "platform/aas/mw/com/impl/find_service_handler.h"
#include "platform/aas/mw/com/impl/handle_type.h"
#include "platform/aas/mw/com/impl/instance_identifier.h"
#include "platform/aas/mw/com/impl/instance_specifier.h"

#include "platform/aas/lib/result/result.h"

namespace bmw
{
namespace mw
{
namespace com
{
namespace impl
{

class IServiceDiscovery
{
  public:
    enum class QualityTypeSelector : std::int32_t
    {
        kBoth = 0,
        kAsilQm = 1,
    };

    virtual ~IServiceDiscovery() = default;

    [[nodiscard]] virtual ResultBlank OfferService(InstanceIdentifier) noexcept = 0;
    [[nodiscard]] virtual ResultBlank StopOfferService(InstanceIdentifier) noexcept = 0;
    [[nodiscard]] virtual ResultBlank StopOfferService(InstanceIdentifier,
                                                       QualityTypeSelector quality_type) noexcept = 0;
    virtual Result<FindServiceHandle> StartFindService(FindServiceHandler<HandleType>,
                                                       const InstanceSpecifier) noexcept = 0;
    virtual Result<FindServiceHandle> StartFindService(FindServiceHandler<HandleType>, InstanceIdentifier) noexcept = 0;
    virtual Result<FindServiceHandle> StartFindService(FindServiceHandler<HandleType>,
                                                       const EnrichedInstanceIdentifier) noexcept = 0;
    [[nodiscard]] virtual ResultBlank StopFindService(const FindServiceHandle) noexcept = 0;
    [[nodiscard]] virtual Result<ServiceHandleContainer<HandleType>> FindService(
        InstanceIdentifier instance_identifier) noexcept = 0;
    [[nodiscard]] virtual Result<ServiceHandleContainer<HandleType>> FindService(
        InstanceSpecifier instance_specifier) noexcept = 0;
};

}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_I_SERVICE_DISCOVERY_H
