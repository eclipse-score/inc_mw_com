// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#ifndef PLATFORM_AAS_MW_COM_IMPL_BINDING_EVENT_RECEIVE_HANDLER_H
#define PLATFORM_AAS_MW_COM_IMPL_BINDING_EVENT_RECEIVE_HANDLER_H

#include "platform/aas/language/safecpp/scoped_function/move_only_scoped_function.h"

namespace bmw::mw::com::impl
{

/// \brief Callback used on the binding level for event notifications on the proxy side.
///
/// We use a ScopedFunction so that the binding independent ProxyEvent controls the scope during which the callback can
/// be called. i.e. When UnsetReceiveHandler is called on the ProxyEvent, then the callable should no longer be called.
/// \todo: Note: A ScopedFunction requires dynamic memory allocation. In the future, we should use a custom allocator
/// which allocates memory in a memory pool which is allocated during startup.
using BindingEventReceiveHandler = safecpp::MoveOnlyScopedFunction<void()>;

}  // namespace bmw::mw::com::impl

#endif  // PLATFORM_AAS_MW_COM_IMPL_BINDING_EVENT_RECEIVE_HANDLER_H
