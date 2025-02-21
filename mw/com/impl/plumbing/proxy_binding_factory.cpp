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


#include "platform/aas/mw/com/impl/plumbing/proxy_binding_factory.h"

#include "platform/aas/mw/com/impl/plumbing/proxy_binding_factory_impl.h"

namespace bmw
{
namespace mw
{
namespace com
{
namespace impl
{

IProxyBindingFactory* ProxyBindingFactory::mock_{nullptr};

IProxyBindingFactory& ProxyBindingFactory::instance() noexcept
{
    if (mock_ != nullptr)
    {
        return *mock_;
    }

    static ProxyBindingFactoryImpl instance{};
    return instance;
}

}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
