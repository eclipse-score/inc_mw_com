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


#ifndef PLATFORM_AAS_MW_COM_IMPL_SERVICE_DISCOVERY_MOCK_H
#define PLATFORM_AAS_MW_COM_IMPL_SERVICE_DISCOVERY_MOCK_H

#include "platform/aas/mw/com/impl/enriched_instance_identifier.h"
#include "platform/aas/mw/com/impl/find_service_handle.h"
#include "platform/aas/mw/com/impl/find_service_handler.h"
#include "platform/aas/mw/com/impl/handle_type.h"
#include "platform/aas/mw/com/impl/i_service_discovery.h"
#include "platform/aas/mw/com/impl/instance_identifier.h"
#include "platform/aas/mw/com/impl/instance_specifier.h"

#include "platform/aas/lib/result/result.h"

#include <gmock/gmock.h>

namespace bmw
{
namespace mw
{
namespace com
{
namespace impl
{

class ServiceDiscoveryMock : public IServiceDiscovery
{
  public:
    MOCK_METHOD(ResultBlank, OfferService, (InstanceIdentifier), (noexcept, override));
    MOCK_METHOD(ResultBlank, StopOfferService, (InstanceIdentifier), (noexcept, override));
    MOCK_METHOD(ResultBlank,
                StopOfferService,
                (InstanceIdentifier, IServiceDiscovery::QualityTypeSelector),
                (noexcept, override));
    MOCK_METHOD(Result<FindServiceHandle>,
                StartFindService,
                (FindServiceHandler<HandleType>, InstanceSpecifier),
                (noexcept, override));
    MOCK_METHOD(Result<FindServiceHandle>,
                StartFindService,
                (FindServiceHandler<HandleType>, InstanceIdentifier),
                (noexcept, override));
    MOCK_METHOD(Result<FindServiceHandle>,
                StartFindService,
                (FindServiceHandler<HandleType>, EnrichedInstanceIdentifier),
                (noexcept, override));
    MOCK_METHOD(ResultBlank, StopFindService, (FindServiceHandle), (noexcept, override));
    MOCK_METHOD(Result<ServiceHandleContainer<HandleType>>, FindService, (InstanceIdentifier), (noexcept, override));
    MOCK_METHOD(Result<ServiceHandleContainer<HandleType>>, FindService, (InstanceSpecifier), (noexcept, override));
};

}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_SERVICE_DISCOVERY_MOCK_H
