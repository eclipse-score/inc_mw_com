// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#include "platform/aas/mw/com/impl/handle_type.h"

#include "platform/aas/mw/log/logging.h"

#include <amp_overload.hpp>

#include <exception>
#include <utility>

namespace bmw
{
namespace mw
{
namespace com
{
namespace impl
{

namespace
{

ServiceInstanceId ExtractInstanceId(amp::optional<ServiceInstanceId> instance_id,
                                    const InstanceIdentifier& identifier) noexcept
{
    if (instance_id.has_value())
    {
        return instance_id.value();
    }
    else
    {
        const InstanceIdentifierView instance_id_view{identifier};
        const auto service_instance_id_result = instance_id_view.GetServiceInstanceId();
        if (!service_instance_id_result.has_value())
        {
            ::bmw::mw::log::LogFatal("lola")
                << "Service instance ID must be provided to the constructor of HandleType if it "
                   "isn't specified in the configuration. Exiting";
            std::terminate();
        }
        return *service_instance_id_result;
    }
}

}  // namespace

HandleType::HandleType(InstanceIdentifier identifier, amp::optional<ServiceInstanceId> instance_id) noexcept
    : identifier_{std::move(identifier)}, instance_id_{ExtractInstanceId(instance_id, identifier_)}
{
}

auto HandleType::GetInstanceIdentifier() const noexcept -> const InstanceIdentifier&
{
    return this->identifier_;
}

auto HandleType::GetDeploymentInformation() const noexcept -> const ServiceInstanceDeployment&
{
    const InstanceIdentifierView instance_id{GetInstanceIdentifier()};
    return instance_id.GetServiceInstanceDeployment();
}

auto operator==(const HandleType& lhs, const HandleType& rhs) noexcept -> bool
{
    return ((lhs.identifier_ == rhs.identifier_) && (lhs.instance_id_ == rhs.instance_id_));
}

auto operator<(const HandleType& lhs, const HandleType& rhs) noexcept -> bool
{
    return ((lhs.identifier_ < rhs.identifier_) || (lhs.instance_id_ < rhs.instance_id_));
}

auto make_HandleType(InstanceIdentifier identifier, amp::optional<ServiceInstanceId> instance_id) noexcept -> HandleType
{
    return HandleType(std::move(identifier), instance_id);
}

}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
