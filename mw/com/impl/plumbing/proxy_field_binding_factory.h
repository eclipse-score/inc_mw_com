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


#ifndef PLATFORM_AAS_MW_COM_IMPL_PLUMBING_PROXY_FIELD_BINDING_FACTORY_H
#define PLATFORM_AAS_MW_COM_IMPL_PLUMBING_PROXY_FIELD_BINDING_FACTORY_H

#include "platform/aas/mw/com/impl/bindings/lola/proxy_event.h"
#include "platform/aas/mw/com/impl/configuration/lola_event_id.h"
#include "platform/aas/mw/com/impl/configuration/lola_service_instance_deployment.h"
#include "platform/aas/mw/com/impl/plumbing/i_proxy_field_binding_factory.h"
#include "platform/aas/mw/com/impl/plumbing/proxy_field_binding_factory_impl.h"
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

/// \brief Class that dispatches to either a real ProxyFieldBindingFactoryImpl or a mocked version
/// ProxyFieldBindingFactoryMock, if a mock is injected.
template <typename SampleType>
class ProxyFieldBindingFactory final
{
  public:
    /// \brief See documentation in IProxyFieldBindingFactory.
    static std::unique_ptr<ProxyEventBinding<SampleType>> CreateEventBinding(ProxyBase& parent,
                                                                             amp::string_view field_name) noexcept
    {
        return instance().CreateEventBinding(parent, field_name);
    }

    /// \brief Inject a mock IProxyFieldBindingFactory. If a mock is injected, then all calls on
    /// ProxyFieldBindingFactory will be dispatched to the mock.
    static void InjectMockBinding(IProxyFieldBindingFactory<SampleType>* const mock) noexcept { mock_ = mock; }

  private:
    static IProxyFieldBindingFactory<SampleType>& instance() noexcept;
    static IProxyFieldBindingFactory<SampleType>* mock_;
};

template <typename SampleType>
auto ProxyFieldBindingFactory<SampleType>::instance() noexcept -> IProxyFieldBindingFactory<SampleType>&
{
    if (mock_ != nullptr)
    {
        return *mock_;
    }

    static ProxyFieldBindingFactoryImpl<SampleType> instance{};
    return instance;
}

template <typename SampleType>
// 
IProxyFieldBindingFactory<SampleType>* ProxyFieldBindingFactory<SampleType>::mock_{nullptr};

}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_PLUMBING_PROXY_FIELD_BINDING_FACTORY_H
