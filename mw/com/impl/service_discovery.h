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


#ifndef PLATFORM_AAS_MW_COM_IMPL_SERVICE_DISCOVERY_H
#define PLATFORM_AAS_MW_COM_IMPL_SERVICE_DISCOVERY_H

#include "platform/aas/mw/com/impl/enriched_instance_identifier.h"
#include "platform/aas/mw/com/impl/find_service_handle.h"
#include "platform/aas/mw/com/impl/find_service_handler.h"
#include "platform/aas/mw/com/impl/handle_type.h"
#include "platform/aas/mw/com/impl/i_runtime.h"
#include "platform/aas/mw/com/impl/i_service_discovery.h"
#include "platform/aas/mw/com/impl/instance_identifier.h"
#include "platform/aas/mw/com/impl/instance_specifier.h"

#include "platform/aas/lib/result/result.h"

#include <atomic>
#include <mutex>
#include <unordered_map>

namespace bmw
{
namespace mw
{
namespace com
{
namespace impl
{

class ServiceDiscovery final : public IServiceDiscovery
{
  public:
    explicit ServiceDiscovery(IRuntime&);
    ServiceDiscovery(const ServiceDiscovery&) noexcept = delete;
    ServiceDiscovery& operator=(const ServiceDiscovery&) noexcept = delete;
    ServiceDiscovery(ServiceDiscovery&&) noexcept = delete;
    ServiceDiscovery& operator=(ServiceDiscovery&&) noexcept = delete;
    ~ServiceDiscovery();

    [[nodiscard]] ResultBlank OfferService(InstanceIdentifier) noexcept override;
    [[nodiscard]] ResultBlank StopOfferService(InstanceIdentifier) noexcept override;
    [[nodiscard]] ResultBlank StopOfferService(InstanceIdentifier, QualityTypeSelector quality_type) noexcept override;
    Result<FindServiceHandle> StartFindService(FindServiceHandler<HandleType>,
                                               const InstanceSpecifier) noexcept override;
    Result<FindServiceHandle> StartFindService(FindServiceHandler<HandleType>, InstanceIdentifier) noexcept override;
    Result<FindServiceHandle> StartFindService(FindServiceHandler<HandleType>,
                                               const EnrichedInstanceIdentifier) noexcept override;
    [[nodiscard]] ResultBlank StopFindService(const FindServiceHandle) noexcept override;
    [[nodiscard]] Result<ServiceHandleContainer<HandleType>> FindService(
        InstanceIdentifier instance_identifier) noexcept override;
    [[nodiscard]] Result<ServiceHandleContainer<HandleType>> FindService(
        InstanceSpecifier instance_specifier) noexcept override;

  private:
    Result<FindServiceHandle> StartFindServiceImpl(FindServiceHandle,
                                                   FindServiceHandler<HandleType>&,
                                                   const EnrichedInstanceIdentifier&) noexcept;
    FindServiceHandle GetNextFreeFindServiceHandle() noexcept;

    FindServiceHandler<HandleType>& StoreUserCallback(const FindServiceHandle&,
                                                      FindServiceHandler<HandleType>) noexcept;
    void StoreInstanceIdentifier(const FindServiceHandle&, const EnrichedInstanceIdentifier&) noexcept;

    IServiceDiscoveryClient& GetServiceDiscoveryClient(const InstanceIdentifier&) noexcept;

    ResultBlank BindingSpecificStartFindService(FindServiceHandle,
                                                FindServiceHandler<HandleType>&,
                                                const EnrichedInstanceIdentifier&) noexcept;

    IRuntime& runtime_;
    std::atomic<std::size_t> next_free_uid_;

    /// \brief Mutex to synchronise modification of user_callbacks_ and handle_to_instances_ in StartFindService and
    /// StopFindService.
    ///
    /// It has to be a recursive mutex as StartFindService / StopFindService can be called within the synchronous call
    /// to the user callback within StartFindService.
    std::recursive_mutex container_mutex_;
    std::unordered_map<FindServiceHandle, FindServiceHandler<HandleType>> user_callbacks_;
    std::unordered_multimap<FindServiceHandle, EnrichedInstanceIdentifier> handle_to_instances_;
};

}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_SERVICE_DISCOVERY_H
