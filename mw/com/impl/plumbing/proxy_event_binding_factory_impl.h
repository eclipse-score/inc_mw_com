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


#ifndef PLATFORM_AAS_MW_COM_IMPL_PLUMBING_PROXY_EVENT_BINDING_FACTORY_IMPL_H
#define PLATFORM_AAS_MW_COM_IMPL_PLUMBING_PROXY_EVENT_BINDING_FACTORY_IMPL_H

#include "platform/aas/mw/com/impl/bindings/lola/generic_proxy_event.h"
#include "platform/aas/mw/com/impl/bindings/lola/proxy_event.h"
#include "platform/aas/mw/com/impl/configuration/lola_service_instance_deployment.h"
#include "platform/aas/mw/com/impl/generic_proxy_event_binding.h"
#include "platform/aas/mw/com/impl/plumbing/i_proxy_event_binding_factory.h"
#include "platform/aas/mw/com/impl/plumbing/test/proxy_event_binding_resources.h"
#include "platform/aas/mw/com/impl/proxy_base.h"
#include "platform/aas/mw/com/impl/proxy_event_binding.h"

#include <amp_overload.hpp>
#include <amp_string_view.hpp>
#include <amp_variant.hpp>

#include <limits>
#include <memory>
#include <utility>

namespace bmw
{
namespace mw
{
namespace com
{
namespace impl
{

/// \brief Factory class that dispatches calls to the appropriate binding based on binding information in the
/// deployment configuration.
template <typename SampleType>
class ProxyEventBindingFactoryImpl : public IProxyEventBindingFactory<SampleType>
{
  public:
    /// Creates instances of the binding specific implementations for a proxy event with a particular data type.
    /// \tparam SampleType Type of the data that is exchanges
    /// \param handle The handle containing the binding information.
    /// \param event_name The binding unspecific name of the event inside the proxy denoted by handle.
    /// \return An instance of ProxyEventBinding or nullptr in case of an error.
    std::unique_ptr<ProxyEventBinding<SampleType>> Create(ProxyBase& parent,
                                                          const amp::string_view event_name) noexcept override;
};

/// \brief Factory class that dispatches calls to the appropriate binding based on binding information in the
/// deployment configuration.
class GenericProxyEventBindingFactoryImpl : public IGenericProxyEventBindingFactory
{
  public:
    /// Creates instances of the binding specific implementations for a generic proxy event that has no data type.
    /// \param handle The handle containing the binding information.
    /// \param event_name The binding unspecific name of the event inside the proxy denoted by handle.
    /// \return An instance of ProxyEventBinding or nullptr in case of an error.
    std::unique_ptr<GenericProxyEventBinding> Create(ProxyBase& parent,
                                                     const amp::string_view event_name) noexcept override;
};

template <typename SampleType>
inline std::unique_ptr<ProxyEventBinding<SampleType>> ProxyEventBindingFactoryImpl<SampleType>::Create(
    ProxyBase& parent,
    const amp::string_view event_name) noexcept
{
    using ReturnType = std::unique_ptr<ProxyEventBinding<SampleType>>;

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
                return std::make_unique<lola::ProxyEvent<SampleType>>(*lola_parent, element_fq_id.value(), event_name);
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

#endif  // PLATFORM_AAS_MW_COM_IMPL_PLUMBING_PROXY_EVENT_BINDING_FACTORY_IMPL_H
