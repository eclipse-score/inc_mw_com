// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#ifndef PLATFORM_AAS_MW_COM_IMPL_PROXY_BASE_H
#define PLATFORM_AAS_MW_COM_IMPL_PROXY_BASE_H

#include "platform/aas/mw/com/impl/find_service_handle.h"
#include "platform/aas/mw/com/impl/find_service_handler.h"
#include "platform/aas/mw/com/impl/handle_type.h"
#include "platform/aas/mw/com/impl/instance_identifier.h"
#include "platform/aas/mw/com/impl/instance_specifier.h"
#include "platform/aas/mw/com/impl/proxy_binding.h"
#include "platform/aas/mw/com/impl/proxy_event_binding.h"

#include "platform/aas/lib/memory/string_literal.h"
#include "platform/aas/lib/result/result.h"

#include <amp_span.hpp>

#include <memory>

namespace bmw::mw::com::impl
{

/// \brief Base class for all binding-unspecific proxies that are generated from the IDL.
class ProxyBase
{

    friend class ProxyBaseView;

  public:
    /// \brief Identifier for a specific instance of a specific service
    using HandleType = ::bmw::mw::com::impl::HandleType;
    using EventNameList = amp::span<const bmw::StringLiteral>;

    /// \brief Creation of ProxyBase which should be called by parent class (generated Proxy or GenericProxy)
    ProxyBase(std::unique_ptr<ProxyBinding> proxy_binding, HandleType handle);

    virtual ~ProxyBase() = default;

    /// \brief A Proxy shall not be copyable
    /// \requirement 
    ProxyBase(const ProxyBase&) = delete;
    ProxyBase& operator=(const ProxyBase&) = delete;

    /// \brief A Proxy shall be movable
    /// \requirement 
    ProxyBase(ProxyBase&& other) = default;
    ProxyBase& operator=(ProxyBase&& other) = default;

    /// \brief Tries to find a service that matches the given specifier synchronously.
    /// \details Does a synchronous one-shot lookup/find, which service instance(s) matching the specifier are there.
    ///
    /// \requirement 
    ///
    /// \param specifier The instance specifier of the service.
    /// \return A result which on success contains a list of found handles that can be used to create a proxy. On
    /// failure, returns an error code.
    static Result<ServiceHandleContainer<HandleType>> FindService(InstanceSpecifier specifier) noexcept;

    /// \brief Tries to find a service that matches the given instance identifier synchronously.
    /// \details Does a synchronous one-shot lookup/find, which service instance(s) matching the specifier are there.
    ///
    /// \param specifier The instance_identifier of the service.
    /// \return A result which on success contains a list of found handles that can be used to create a proxy. On
    /// failure, returns an error code.
    static Result<ServiceHandleContainer<HandleType>> FindService(InstanceIdentifier instance_identifier) noexcept;

    static Result<FindServiceHandle> StartFindService(FindServiceHandler<HandleType> handler,
                                                      InstanceIdentifier instance_identifier) noexcept;

    static Result<FindServiceHandle> StartFindService(FindServiceHandler<HandleType> handler,
                                                      InstanceSpecifier instance_specifier) noexcept;

    static bmw::ResultBlank StopFindService(const FindServiceHandle handle) noexcept;

    /// Returns the handle that was used to instantiate this proxy.
    ///
    /// \return Handle identifying the service that this proxy is connected to.
    const HandleType& GetHandle() const noexcept;

  protected:
    bool AreBindingsValid() const noexcept
    {
        const bool is_proxy_binding_valid{proxy_binding_ != nullptr};
        return is_proxy_binding_valid && are_service_element_bindings_valid_;
    }

    std::unique_ptr<ProxyBinding> proxy_binding_;
    HandleType handle_;
    bool are_service_element_bindings_valid_;
};

class ProxyBaseView final
{
  public:
    /// \brief Create a view on the ProxyBase instance to allow for additional methods on the ProxyBase.
    ///
    /// \param proxy_base The ProxyBase to create the view on.
    explicit ProxyBaseView(ProxyBase& proxy_base) noexcept;

    /// \brief Return a reference to the underlying implementation provided by the binding.
    ///
    /// \return Pointer to the proxy binding.
    ProxyBinding* GetBinding() noexcept;

    const HandleType& GetAssociatedHandleType() const noexcept { return proxy_base_.handle_; }

    void MarkServiceElementBindingInvalid() noexcept { proxy_base_.are_service_element_bindings_valid_ = false; }

  private:
    ProxyBase& proxy_base_;
};

}  // namespace bmw::mw::com::impl

#endif  // PLATFORM_AAS_MW_COM_IMPL_PROXY_BASE_H
