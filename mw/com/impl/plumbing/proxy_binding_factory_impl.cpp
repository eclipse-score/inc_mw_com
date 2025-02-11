// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#include "platform/aas/mw/com/impl/plumbing/proxy_binding_factory_impl.h"

#include "platform/aas/mw/com/impl/bindings/lola/proxy.h"
#include "platform/aas/mw/com/impl/com_error.h"
#include "platform/aas/mw/com/impl/instance_identifier.h"

#include "platform/aas/mw/log/logging.h"

#include <amp_overload.hpp>
#include <amp_variant.hpp>

#include <exception>
#include <memory>
#include <utility>
#include <vector>

namespace bmw
{
namespace mw
{
namespace com
{
namespace impl
{

std::unique_ptr<ProxyBinding> ProxyBindingFactoryImpl::Create(const HandleType& handle) noexcept
{
    using ReturnType = std::unique_ptr<ProxyBinding>;
    auto visitor = amp::overload(
        [handle](const LolaServiceInstanceDeployment&) -> ReturnType { return lola::Proxy::Create(handle); },
        // 
        [](const SomeIpServiceInstanceDeployment&) -> ReturnType { return nullptr; },
        // 
        [](const amp::blank&) -> ReturnType { return nullptr; });
    return amp::visit(visitor, handle.GetDeploymentInformation().bindingInfo_);
}

}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
