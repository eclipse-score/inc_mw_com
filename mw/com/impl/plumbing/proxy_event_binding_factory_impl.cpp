// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#include "platform/aas/mw/com/impl/plumbing/proxy_event_binding_factory_impl.h"

namespace bmw
{
namespace mw
{
namespace com
{
namespace impl
{

std::unique_ptr<GenericProxyEventBinding> GenericProxyEventBindingFactoryImpl::Create(
    ProxyBase& parent,
    const amp::string_view event_name) noexcept
{
    using ReturnType = std::unique_ptr<GenericProxyEventBinding>;

    const HandleType& handle = parent.GetHandle();
    const ServiceInstanceDeployment& deployment = handle.GetDeploymentInformation();
    const auto& service_deployment = InstanceIdentifierView{handle.GetInstanceIdentifier()}.GetServiceTypeDeployment();

    auto deployment_info_visitor = amp::overload(
        [&parent, event_name, &handle, &service_deployment](const LolaServiceInstanceDeployment&) -> ReturnType {
            auto* const lola_parent = dynamic_cast<lola::Proxy*>(ProxyBaseView{parent}.GetBinding());

            const auto lola_service_type_deployment =
                GetLolaServiceTypeDeploymentFromServiceTypeDeployment(service_deployment);
            std::string event_name_string{event_name.data(), event_name.size()};
            const auto event_id = lola_service_type_deployment.events_.at(event_name_string);
            const Result<lola::ElementFqId> element_fq_id =
                EventConfigToElementFqId(handle.GetInstanceId(), service_deployment, event_id);

            if ((element_fq_id.has_value()) && (lola_parent != nullptr))
            {
                return std::make_unique<lola::GenericProxyEvent>(*lola_parent, element_fq_id.value(), event_name);
            }
            else
            {
                // LCOV_EXCL_LINE defensive programming: EventConfigToElementFqId only returns no value, when the
                // binding instance id is greater than uint16_t::max() or event_id is greater than uint8_t::max(), or
                // the deployment type is not lola. The binding instance id is uint16_t and the event id must match the
                // event ID registered with the skeleton using an ElementFqId which is also uint8_t. The deployment type
                // must be lola in order to select this overload. Therefore, this case can currently not be reached.
                return nullptr;
            }
        },
        [](const amp::blank&) -> ReturnType {
            // LCOV_EXCL_LINE defensive programming: ProxyBase currently cannot be instantiated with a SomeIp or blank
            // binding
            return nullptr;
        },
        [](const SomeIpServiceInstanceDeployment&) -> ReturnType {
            // LCOV_EXCL_LINE defensive programming: ProxyBase currently cannot be instantiated with a SomeIp or blank
            // binding
            return nullptr;
        });

    return amp::visit(deployment_info_visitor, deployment.bindingInfo_);
}

}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
