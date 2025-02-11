// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#include "platform/aas/mw/com/impl/service_discovery.h"
#include "platform/aas/mw/com/impl/i_runtime_binding.h"

#include <algorithm>

namespace bmw
{
namespace mw
{
namespace com
{
namespace impl
{

ServiceDiscovery::ServiceDiscovery(bmw::mw::com::impl::IRuntime& runtime)
    : IServiceDiscovery{},
      runtime_{runtime},
      next_free_uid_{0},
      container_mutex_{},
      user_callbacks_{},
      handle_to_instances_{}
{
}

ServiceDiscovery::~ServiceDiscovery()
{
    const auto copy_of_handles = handle_to_instances_;
    for (const auto& handle : copy_of_handles)
    {
        const auto result = StopFindService(handle.first);
        if (!result.has_value())
        {
            mw::log::LogError("lola") << "FindService for (" << FindServiceHandleView{handle.first}.getUid() << ","
                                      << handle.second.GetInstanceIdentifier().ToString() << ") could not be stopped"
                                      << result.error();
        }
    }
}

auto ServiceDiscovery::OfferService(bmw::mw::com::impl::InstanceIdentifier instance_identifier) noexcept -> ResultBlank
{
    auto& service_discovery_client = GetServiceDiscoveryClient(instance_identifier);

    return service_discovery_client.OfferService(std::move(instance_identifier));
}

auto ServiceDiscovery::StopOfferService(bmw::mw::com::impl::InstanceIdentifier instance_identifier) noexcept
    -> ResultBlank
{
    return StopOfferService(instance_identifier, QualityTypeSelector::kBoth);
}

auto ServiceDiscovery::StopOfferService(bmw::mw::com::impl::InstanceIdentifier instance_identifier,
                                        QualityTypeSelector quality_type) noexcept -> ResultBlank
{
    auto& service_discovery_client = GetServiceDiscoveryClient(instance_identifier);

    return service_discovery_client.StopOfferService(std::move(instance_identifier), quality_type);
}

auto ServiceDiscovery::StartFindService(FindServiceHandler<HandleType> handler,
                                        const InstanceSpecifier instance_specifier) noexcept
    -> Result<FindServiceHandle>
{
    const auto find_service_handle = GetNextFreeFindServiceHandle();
    auto& handler_reference = StoreUserCallback(find_service_handle, std::move(handler));

    const auto instance_identifiers = runtime_.resolve(instance_specifier);
    for (auto& instance_identifier : instance_identifiers)
    {
        EnrichedInstanceIdentifier enriched_instance_identifier{instance_identifier};
        auto result = StartFindServiceImpl(find_service_handle, handler_reference, enriched_instance_identifier);
        if (!(result.has_value()))
        {
            return result;
        }
    }

    return find_service_handle;
}

auto ServiceDiscovery::StartFindService(FindServiceHandler<HandleType> handler,
                                        InstanceIdentifier instance_identifier) noexcept -> Result<FindServiceHandle>
{
    EnrichedInstanceIdentifier enriched_instance_identifier{std::move(instance_identifier)};
    return StartFindService(std::move(handler), std::move(enriched_instance_identifier));
}

auto ServiceDiscovery::StartFindService(FindServiceHandler<HandleType> handler,
                                        const EnrichedInstanceIdentifier enriched_instance_identifier) noexcept
    -> Result<FindServiceHandle>
{
    std::lock_guard lock{container_mutex_};
    auto find_service_handle = GetNextFreeFindServiceHandle();
    auto& handler_reference = StoreUserCallback(find_service_handle, std::move(handler));

    return StartFindServiceImpl(find_service_handle, handler_reference, enriched_instance_identifier);
}

[[nodiscard]] auto ServiceDiscovery::StopFindService(const FindServiceHandle find_service_handle) noexcept
    -> ResultBlank
{
    std::lock_guard lock{container_mutex_};
    auto iterators = handle_to_instances_.equal_range(find_service_handle);

    ResultBlank result{};
    std::for_each(iterators.first, iterators.second, [this, &result, &find_service_handle](const auto& entry) {
        const auto& enriched_instance_identifier = entry.second;

        auto& service_discovery_client =
            GetServiceDiscoveryClient(enriched_instance_identifier.GetInstanceIdentifier());
        auto specific_result = service_discovery_client.StopFindService(find_service_handle);
        if (!specific_result.has_value())
        {
            result = specific_result;
        }
    });

    user_callbacks_.erase(find_service_handle);
    handle_to_instances_.erase(find_service_handle);

    return result;
}

auto ServiceDiscovery::StartFindServiceImpl(FindServiceHandle find_service_handle,
                                            FindServiceHandler<HandleType>& handler_reference,
                                            const EnrichedInstanceIdentifier& enriched_instance_identifier) noexcept
    -> Result<FindServiceHandle>
{
    StoreInstanceIdentifier(find_service_handle, enriched_instance_identifier);
    auto result = BindingSpecificStartFindService(find_service_handle, handler_reference, enriched_instance_identifier);
    if (!result.has_value())
    {
        amp::ignore = StopFindService(find_service_handle);
        return Unexpected{result.error()};
    }

    return find_service_handle;
}

FindServiceHandle ServiceDiscovery::GetNextFreeFindServiceHandle() noexcept
{
    // This is an atomic value. Incrementing and assignement has to take place on the same line.
    // Since there are no other operations besides increment ans assignement, it is quite clear what is happening.
    // 
    const auto free_uid = next_free_uid_++;
    return make_FindServiceHandle(free_uid);
}

auto ServiceDiscovery::StoreUserCallback(const FindServiceHandle& find_service_handle,
                                         FindServiceHandler<HandleType> handler) noexcept
    -> FindServiceHandler<HandleType>&
{
    auto entry = user_callbacks_.emplace(std::piecewise_construct,
                                         std::forward_as_tuple(find_service_handle),
                                         std::forward_as_tuple(std::move(handler)));

    return entry.first->second;
}

auto ServiceDiscovery::StoreInstanceIdentifier(const FindServiceHandle& find_service_handle,
                                               const EnrichedInstanceIdentifier& enriched_instance_identifier) noexcept
    -> void
{
    handle_to_instances_.emplace(std::piecewise_construct,
                                 std::forward_as_tuple(find_service_handle),
                                 std::forward_as_tuple(enriched_instance_identifier));
}

auto ServiceDiscovery::GetServiceDiscoveryClient(const InstanceIdentifier& instance_identifier) noexcept
    -> IServiceDiscoveryClient&
{
    InstanceIdentifierView instance_identifier_view{instance_identifier};
    auto binding_type = instance_identifier_view.GetServiceInstanceDeployment().GetBindingType();
    auto binding_runtime = runtime_.GetBindingRuntime(binding_type);

    if (binding_runtime == nullptr)
    {
        mw::log::LogFatal("lola") << "Service discovery failed to find fitting binding for"
                                  << instance_identifier.ToString();
    }
    AMP_ASSERT_PRD_MESSAGE(binding_runtime != nullptr, "Unsupported binding");

    return binding_runtime->GetServiceDiscoveryClient();
}

auto ServiceDiscovery::BindingSpecificStartFindService(
    FindServiceHandle search_handle,
    FindServiceHandler<HandleType>& handler,
    const EnrichedInstanceIdentifier& enriched_instance_identifier) noexcept -> ResultBlank
{
    auto& service_discovery_client = GetServiceDiscoveryClient(enriched_instance_identifier.GetInstanceIdentifier());

    return service_discovery_client.StartFindService(
        search_handle,
        [&handler](auto container, auto handle) noexcept { handler(container, handle); },
        enriched_instance_identifier);
}

Result<ServiceHandleContainer<HandleType>> ServiceDiscovery::FindService(
    InstanceIdentifier instance_identifier) noexcept
{
    EnrichedInstanceIdentifier enriched_instance_identifier{std::move(instance_identifier)};
    auto& service_discovery_client = GetServiceDiscoveryClient(enriched_instance_identifier.GetInstanceIdentifier());
    return service_discovery_client.FindService(std::move(enriched_instance_identifier));
}

Result<ServiceHandleContainer<HandleType>> ServiceDiscovery::FindService(InstanceSpecifier instance_specifier) noexcept
{
    ServiceHandleContainer<HandleType> handles;
    const auto instance_identifiers = runtime_.resolve(instance_specifier);
    if (instance_identifiers.size() == std::size_t{0U})
    {
        bmw::mw::log::LogFatal("lola") << "Failed to resolve instance identifier from instance specifier";
        std::terminate();
    }

    bool are_only_errors_received{true};
    for (auto& instance_identifier : instance_identifiers)
    {
        auto result = FindService(instance_identifier);
        if (result.has_value())
        {
            are_only_errors_received = false;
            for (const auto& handle : result.value())
            {
                handles.push_back(handle);
            }
        }
    }

    if (are_only_errors_received)
    {
        return MakeUnexpected(ComErrc::kBindingFailure);
    }
    return handles;
}

}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
