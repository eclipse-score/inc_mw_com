// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#ifndef PLATFORM_AAS_MW_COM_IMPL_PLUMBING_I_PROXY_EVENT_BINDING_FACTORY_H
#define PLATFORM_AAS_MW_COM_IMPL_PLUMBING_I_PROXY_EVENT_BINDING_FACTORY_H

#include "platform/aas/mw/com/impl/generic_proxy_event_binding.h"
#include "platform/aas/mw/com/impl/handle_type.h"
#include "platform/aas/mw/com/impl/instance_identifier.h"
#include "platform/aas/mw/com/impl/proxy_base.h"
#include "platform/aas/mw/com/impl/proxy_event_binding.h"

#include <amp_string_view.hpp>

#include <memory>

namespace bmw
{
namespace mw
{
namespace com
{
namespace impl
{

/// \brief Interface for a factory class that dispatches calls to the appropriate binding based on binding
/// information in the deployment configuration.
template <typename SampleType>
class IProxyEventBindingFactory
{
  public:
    IProxyEventBindingFactory() noexcept = default;

    virtual ~IProxyEventBindingFactory() noexcept = default;

    IProxyEventBindingFactory(IProxyEventBindingFactory&&) = delete;
    IProxyEventBindingFactory& operator=(IProxyEventBindingFactory&&) = delete;
    IProxyEventBindingFactory(const IProxyEventBindingFactory&) = delete;
    IProxyEventBindingFactory& operator=(const IProxyEventBindingFactory&) = delete;

    /// Creates instances of the binding specific implementations for a proxy event with a particular data type.
    /// \tparam SampleType Type of the data that is exchanges
    /// \param handle The handle containing the binding information.
    /// \param event_name The binding unspecific name of the event inside the proxy denoted by handle.
    /// \return An instance of ProxyEventBinding or nullptr in case of an error.
    virtual auto Create(ProxyBase& parent, const amp::string_view event_name) noexcept
        -> std::unique_ptr<ProxyEventBinding<SampleType>> = 0;
};

/// \brief Interface for a factory class that dispatches calls to the appropriate binding based on
/// binding information in the deployment configuration.
class IGenericProxyEventBindingFactory
{
  public:
    IGenericProxyEventBindingFactory() noexcept = default;

    virtual ~IGenericProxyEventBindingFactory() noexcept = default;

    IGenericProxyEventBindingFactory(IGenericProxyEventBindingFactory&&) = delete;
    IGenericProxyEventBindingFactory& operator=(IGenericProxyEventBindingFactory&&) = delete;
    IGenericProxyEventBindingFactory(const IGenericProxyEventBindingFactory&) = delete;
    IGenericProxyEventBindingFactory& operator=(const IGenericProxyEventBindingFactory&) = delete;

    /// Creates instances of the binding specific implementations for a generic proxy event that has no data type.
    /// \param handle The handle containing the binding information.
    /// \param event_name The binding unspecific name of the event inside the proxy denoted by handle.
    /// \return An instance of ProxyEventBinding or nullptr in case of an error.
    virtual std::unique_ptr<GenericProxyEventBinding> Create(ProxyBase& parent,
                                                             const amp::string_view event_name) noexcept = 0;
};

}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_PLUMBING_I_PROXY_EVENT_BINDING_FACTORY_H
