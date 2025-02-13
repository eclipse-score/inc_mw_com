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


///
/// @file
/// 
///
#include "platform/aas/mw/com/impl/generic_proxy_event.h"

#include "platform/aas/mw/com/impl/plumbing/proxy_event_binding_factory.h"

#include <amp_assert.hpp>

namespace bmw
{
namespace mw
{
namespace com
{
namespace impl
{

GenericProxyEvent::GenericProxyEvent(ProxyBase& base, const amp::string_view event_name)
    : ProxyEventBase{base, GenericProxyEventBindingFactory::Create(base, event_name), event_name}
{
}

GenericProxyEvent::GenericProxyEvent(ProxyBase& base,
                                     std::unique_ptr<GenericProxyEventBinding> proxy_binding,
                                     const amp::string_view event_name)
    : ProxyEventBase{base, std::move(proxy_binding), event_name}
{
}

std::size_t GenericProxyEvent::GetSampleSize() const noexcept
{
    auto* const proxy_event_binding = dynamic_cast<GenericProxyEventBinding*>(binding_base_.get());
    AMP_ASSERT_PRD_MESSAGE(proxy_event_binding != nullptr, "Downcast to GenericProxyEventBinding failed!");
    return proxy_event_binding->GetSampleSize();
}

bool GenericProxyEvent::HasSerializedFormat() const noexcept
{
    auto* const proxy_event_binding = dynamic_cast<GenericProxyEventBinding*>(binding_base_.get());
    AMP_ASSERT_PRD_MESSAGE(proxy_event_binding != nullptr, "Downcast to GenericProxyEventBinding failed!");
    return proxy_event_binding->HasSerializedFormat();
}

}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
