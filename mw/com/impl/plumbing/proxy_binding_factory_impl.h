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


#ifndef PLATFORM_AAS_MW_COM_IMPL_PLUMBING_PROXY_BINDING_FACTORY_IMPL_H
#define PLATFORM_AAS_MW_COM_IMPL_PLUMBING_PROXY_BINDING_FACTORY_IMPL_H

#include "platform/aas/mw/com/impl/plumbing/i_proxy_binding_factory.h"

#include <memory>
#include <vector>

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
class ProxyBindingFactoryImpl final : public IProxyBindingFactory
{
  public:
    /// \brief See documentation in IProxyBindingFactory.
    std::unique_ptr<ProxyBinding> Create(const HandleType& handle) noexcept override;
};

}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_PLUMBING_PROXY_BINDING_FACTORY_IMPL_H
