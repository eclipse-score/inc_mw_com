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


///
/// @file
/// 
///
#include "platform/aas/mw/com/impl/generic_proxy.h"

#include "platform/aas/mw/com/impl/com_error.h"
#include "platform/aas/mw/com/impl/plumbing/proxy_binding_factory.h"

#include "platform/aas/mw/log/logging.h"

#include <amp_assert.hpp>

#include <algorithm>
#include <utility>
#include <vector>

namespace bmw::mw::com::impl
{

namespace
{

std::vector<amp::string_view> GetEventNameList(const InstanceIdentifier& identifier) noexcept
{
    using ReturnType = std::vector<amp::string_view>;

    const auto& service_type_deployment = InstanceIdentifierView{identifier}.GetServiceTypeDeployment();
    auto visitor = amp::overload(
        [](const LolaServiceTypeDeployment& deployment) -> ReturnType {
            ReturnType event_names;
            for (const auto& event : deployment.events_)
            {
                const auto event_name = amp::string_view{event.first.data(), event.first.size()};
                event_names.push_back(event_name);
            }
            return event_names;
        },
        [](const amp::blank&) -> ReturnType { return {}; });
    return amp::visit(visitor, service_type_deployment.binding_info_);
}

}  // namespace

Result<GenericProxy> GenericProxy::Create(HandleType instance_handle) noexcept
{
    auto proxy_binding = ProxyBindingFactory::Create(instance_handle);
    if (proxy_binding == nullptr)
    {
        ::bmw::mw::log::LogError("lola") << "Could not create GenericProxy as binding could not be created.";
        return MakeUnexpected(ComErrc::kBindingFailure);
    }

    GenericProxy generic_proxy{std::move(proxy_binding), std::move(instance_handle)};
    if (!generic_proxy.AreBindingsValid())
    {
        ::bmw::mw::log::LogError("lola") << "Could not create GenericProxy as binding is invalid.";
        return MakeUnexpected(ComErrc::kBindingFailure);
    }

    const auto& instance_identifier = generic_proxy.handle_.GetInstanceIdentifier();
    const auto event_names = GetEventNameList(instance_identifier);
    generic_proxy.FillEventMap(event_names);
    if (!generic_proxy.IsEventMapValid())
    {
        ::bmw::mw::log::LogError("lola")
            << "Could not create GenericProxy at least one event in the EventMap is invalid.";
        return MakeUnexpected(ComErrc::kBindingFailure);
    }
    return generic_proxy;
}

GenericProxy::GenericProxy(HandleType instance_handle)
    : ProxyBase{ProxyBindingFactory::Create(instance_handle), std::move(instance_handle)}
{
}

GenericProxy::GenericProxy(std::unique_ptr<ProxyBinding> proxy_binding, HandleType instance_handle)
    : ProxyBase{std::move(proxy_binding), std::move(instance_handle)}
{
}

void GenericProxy::FillEventMap(const std::vector<amp::string_view>& event_names) noexcept
{
    for (const auto event_name : event_names)
    {
        if (proxy_binding_->IsEventProvided(event_name))
        {
            const auto emplace_result = events_.emplace(
                std::piecewise_construct, std::forward_as_tuple(event_name), std::forward_as_tuple(*this, event_name));
            AMP_ASSERT_PRD_MESSAGE(emplace_result.second, "Could not emplace GenericProxyEvent in map.");
        }
        else
        {
            ::bmw::mw::log::LogError("lola")
                << "GenericProxy: Event provided in the ServiceTypeDeployment could not be "
                   "found in shared memory. This is likely a configuration error.";
        }
    }
}

bool GenericProxy::IsEventMapValid() const noexcept
{
    bool is_event_map_valid{true};
    std::for_each(events_.cbegin(), events_.cend(), [&is_event_map_valid](const EventMap::value_type& value) {
        if (!value.second.IsBindingValid())
        {
            is_event_map_valid = false;
        }
    });
    return is_event_map_valid;
}

GenericProxy::EventMap& GenericProxy::GetEvents() noexcept
{
    // The signature of this function is part of the public API of the GenericProxy, specified in this requirement:
    // 
    // 
    return events_;
}

}  // namespace bmw::mw::com::impl
