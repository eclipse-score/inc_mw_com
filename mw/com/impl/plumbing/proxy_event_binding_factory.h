// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#ifndef PLATFORM_AAS_MW_COM_IMPL_PLUMBING_PROXY_EVENT_BINDING_FACTORY_H
#define PLATFORM_AAS_MW_COM_IMPL_PLUMBING_PROXY_EVENT_BINDING_FACTORY_H

#include "platform/aas/mw/com/impl/bindings/lola/generic_proxy_event.h"
#include "platform/aas/mw/com/impl/bindings/lola/proxy_event.h"
#include "platform/aas/mw/com/impl/configuration/lola_service_instance_deployment.h"
#include "platform/aas/mw/com/impl/generic_proxy_event_binding.h"
#include "platform/aas/mw/com/impl/plumbing/i_proxy_event_binding_factory.h"
#include "platform/aas/mw/com/impl/plumbing/proxy_event_binding_factory_impl.h"
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

/// \brief Class that dispatches to either a real ProxyEventBindingFactoryImpl or a mocked version
/// ProxyEventBindingFactoryMock, if a mock is injected.
template <typename SampleType>
class ProxyEventBindingFactory final
{
  public:
    /// \brief See documentation in IProxyEventBindingFactory.
    static std::unique_ptr<ProxyEventBinding<SampleType>> Create(ProxyBase& parent, amp::string_view event_name)
    {
        return instance().Create(parent, event_name);
    }

    /// \brief Inject a mock IProxyEventBindingFactory. If a mock is injected, then all calls on
    /// ProxyEventBindingFactory will be dispatched to the mock.
    static void InjectMockBinding(IProxyEventBindingFactory<SampleType>* const mock) noexcept { mock_ = mock; }

  private:
    static IProxyEventBindingFactory<SampleType>& instance() noexcept;
    static IProxyEventBindingFactory<SampleType>* mock_;
};

/// \brief Class that dispatches to either a real GenericProxyEventBindingFactoryImpl or a mocked version
/// GenericProxyEventBindingFactoryMock, if a mock is injected.
class GenericProxyEventBindingFactory final
{
  public:
    /// \brief See documentation in IGenericProxyEventBindingFactory.
    static std::unique_ptr<GenericProxyEventBinding> Create(ProxyBase& parent, amp::string_view event_name)
    {
        return instance().Create(parent, event_name);
    }

    /// \brief Inject a mock IGenericProxyEventBindingFactory. If a mock is injected, then all calls on
    /// GenericProxyEventBindingFactory will be dispatched to the mock.
    static void InjectMockBinding(IGenericProxyEventBindingFactory* const mock) noexcept { mock_ = mock; }

  private:
    static IGenericProxyEventBindingFactory& instance() noexcept;
    static IGenericProxyEventBindingFactory* mock_;
};

template <typename SampleType>
auto ProxyEventBindingFactory<SampleType>::instance() noexcept -> IProxyEventBindingFactory<SampleType>&
{
    if (mock_ != nullptr)
    {
        return *mock_;
    }

    static ProxyEventBindingFactoryImpl<SampleType> instance{};
    return instance;
}

// Suppress "AUTOSAR_C++14_A3-1-1" rule finding. This rule states:" It shall be possible to include any header file
// in multiple translation units without violating the One Definition Rule".
template <typename SampleType>
//  This is false-positive, "mock_" is defined once
IProxyEventBindingFactory<SampleType>* ProxyEventBindingFactory<SampleType>::mock_{nullptr};

}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_PLUMBING_PROXY_EVENT_BINDING_FACTORY_H
