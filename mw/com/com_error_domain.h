// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#ifndef PLATFORM_AAS_MW_COM_COM_ERROR_DOMAIN_H
#define PLATFORM_AAS_MW_COM_COM_ERROR_DOMAIN_H

#include "platform/aas/mw/com/impl/com_error.h"

/**
 * \brief The com error domain header file includes the error related definitions which are specific for the
 * ara::com API.
 *
 * \requirement , , , 
 */
namespace bmw
{
namespace mw
{
namespace com
{

using ComErrc = bmw::mw::com::impl::ComErrc;
using ComErrorDomain = bmw::mw::com::impl::ComErrorDomain;

}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_COM_ERROR_DOMAIN_H
