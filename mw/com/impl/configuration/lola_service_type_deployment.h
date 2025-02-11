// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#ifndef PLATFORM_AAS_MW_COM_IMPL_CONFIGURATION_LOLA_SERVICE_TYPE_DEPLOYMENT_H
#define PLATFORM_AAS_MW_COM_IMPL_CONFIGURATION_LOLA_SERVICE_TYPE_DEPLOYMENT_H

#include "platform/aas/mw/com/impl/configuration/binding_service_type_deployment.h"
#include "platform/aas/mw/com/impl/configuration/lola_event_id.h"
#include "platform/aas/mw/com/impl/configuration/lola_field_id.h"
#include "platform/aas/mw/com/impl/configuration/lola_service_id.h"

namespace bmw
{
namespace mw
{
namespace com
{
namespace impl
{

using LolaServiceTypeDeployment = BindingServiceTypeDeployment<LolaEventId, LolaFieldId, LolaServiceId>;

}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_CONFIGURATION_LOLA_SERVICE_TYPE_DEPLOYMENT_H
