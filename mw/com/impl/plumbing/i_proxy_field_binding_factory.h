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


#ifndef PLATFORM_AAS_MW_COM_IMPL_PLUMBING_I_PROXY_FIELD_BINDING_FACTORY_H
#define PLATFORM_AAS_MW_COM_IMPL_PLUMBING_I_PROXY_FIELD_BINDING_FACTORY_H

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

/// \brief Interface for a factory class that dispatches calls to the appropriate binding based on
/// binding information in the deployment configuration.
template <typename SampleType>
class IProxyFieldBindingFactory
{
  public:
    IProxyFieldBindingFactory() noexcept = default;

    virtual ~IProxyFieldBindingFactory() noexcept = default;

    IProxyFieldBindingFactory(IProxyFieldBindingFactory&&) = delete;
    IProxyFieldBindingFactory& operator=(IProxyFieldBindingFactory&&) = delete;
    IProxyFieldBindingFactory(const IProxyFieldBindingFactory&) = delete;
    IProxyFieldBindingFactory& operator=(const IProxyFieldBindingFactory&) = delete;

    /// Creates instances of the event binding of a proxy field with a particular data type.
    /// \tparam SampleType Type of the data that is exchanges
    /// \param handle The handle containing the binding information.
    /// \param field_name The binding unspecific name of the event inside the proxy denoted by handle.
    /// \return An instance of ProxyEventBinding or nullptr in case of an error.
    virtual auto CreateEventBinding(ProxyBase& parent, const amp::string_view field_name) noexcept
        -> std::unique_ptr<ProxyEventBinding<SampleType>> = 0;
};
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_PLUMBING_I_PROXY_FIELD_BINDING_FACTORY_H
