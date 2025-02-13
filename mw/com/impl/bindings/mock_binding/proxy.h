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


#ifndef PLATFORM_AAS_MW_COM_IMPL_BINDINGS_MOCK_BINDING_PROXY_H
#define PLATFORM_AAS_MW_COM_IMPL_BINDINGS_MOCK_BINDING_PROXY_H

#include "platform/aas/mw/com/impl/proxy_binding.h"

#include <gmock/gmock.h>

namespace bmw
{
namespace mw
{
namespace com
{
namespace impl
{
namespace mock_binding
{

/// \brief Proxy binding implementation for all mock binding proxies.
class Proxy : public ProxyBinding
{
  public:
    Proxy() = default;
    ~Proxy() = default;

    MOCK_METHOD(bool, IsEventProvided, (const amp::string_view), (const, noexcept, override));
    MOCK_METHOD(void, RegisterEventBinding, (amp::string_view, ProxyEventBindingBase&), (noexcept, override));
    MOCK_METHOD(void, UnregisterEventBinding, (amp::string_view), (noexcept, override));
};

}  // namespace mock_binding
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_BINDINGS_MOCK_BINDING_PROXY_H
