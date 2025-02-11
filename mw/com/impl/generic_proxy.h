// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



///
/// @file
/// 
///

#ifndef PLATFORM_AAS_MW_COM_IMPL_GENERIC_PROXY_H
#define PLATFORM_AAS_MW_COM_IMPL_GENERIC_PROXY_H

#include "platform/aas/mw/com/impl/generic_proxy_event.h"
#include "platform/aas/mw/com/impl/handle_type.h"
#include "platform/aas/mw/com/impl/proxy_base.h"
#include "platform/aas/mw/com/impl/proxy_binding.h"
#include "platform/aas/mw/com/impl/service_element_map.h"

#include "platform/aas/lib/result/result.h"

#include <memory>

namespace bmw::mw::com::impl
{
namespace test
{
class GenericProxyAttorney;
}

/// \brief GenericProxy is a non-binding specific Proxy class which doesn't require any type information for its events.
/// This means that it can connect to a service providing instance (skeleton) just based on deployment information
/// specified at runtime.
///
/// It contains a map of events which can access strongly-typed events in a type-erased way i.e. by accessing a raw
/// memory buffer.
///
/// It is the generic analogue of a Proxy, which contains strongly-typed events. While the Proxy is usually generated
/// from the IDL, a GenericProxy can be manually instantiated at runtime based on deployment information.
/// \todo The EventMap, events, needs to be populated with actual GenericProxyEvents.
class GenericProxy : public ProxyBase
{
    friend class test::GenericProxyAttorney;

  public:
    using EventMap = ServiceElementMap<GenericProxyEvent>;

    /// \brief Exception-less GenericProxy constructor
    static Result<GenericProxy> Create(HandleType instance_handle) noexcept;

    EventMap& GetEvents() noexcept;

  private:
    /// \brief Constructs ProxyBase
    explicit GenericProxy(HandleType instance_handle);

    GenericProxy(std::unique_ptr<ProxyBinding> proxy_binding, HandleType instance_handle);

    void FillEventMap(const std::vector<amp::string_view>& event_names) noexcept;
    bool IsEventMapValid() const noexcept;

    EventMap events_;
};

}  // namespace bmw::mw::com::impl

#endif  // PLATFORM_AAS_MW_COM_IMPL_GENERIC_PROXY_H
