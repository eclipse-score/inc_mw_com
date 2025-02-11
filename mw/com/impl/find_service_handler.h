// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#ifndef PLATFORM_AAS_MW_COM_IMPL_FIND_SERVICE_HANDLER_H
#define PLATFORM_AAS_MW_COM_IMPL_FIND_SERVICE_HANDLER_H

#include "platform/aas/mw/com/impl/find_service_handle.h"

#include <amp_callback.hpp>

#include <vector>

namespace bmw
{
namespace mw
{
namespace com
{
namespace impl
{

/// \brief The container holds a list of service handles and is used as a return value of the FindService methods.
///
/// \requirement 
template <typename T>
using ServiceHandleContainer = std::vector<T>;

/// \brief A function wrapper for the handler function that gets called by the Communication Management software in case
/// the service availability changes.
///
/// It takes as input parameter a handle container containing handles for all matching service instances and a
/// FindServiceHandle which can be used to invoke StopFindService (see []) from within the
/// FindServiceHandler.
///
/// \requirement 
template <typename T>
using FindServiceHandler = amp::callback<void(ServiceHandleContainer<T>, FindServiceHandle)>;

}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_FIND_SERVICE_HANDLER_H
