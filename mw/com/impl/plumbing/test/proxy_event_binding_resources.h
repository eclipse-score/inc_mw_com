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


#ifndef PLATFORM_AAS_MW_COM_IMPL_PLUMBING_PROXY_EVENT_BINDING_TEST_RESOURCES_H
#define PLATFORM_AAS_MW_COM_IMPL_PLUMBING_PROXY_EVENT_BINDING_TEST_RESOURCES_H

#include "platform/aas/mw/com/impl/bindings/lola/element_fq_id.h"
#include "platform/aas/mw/com/impl/configuration/lola_event_id.h"
#include "platform/aas/mw/com/impl/configuration/lola_field_id.h"
#include "platform/aas/mw/com/impl/configuration/lola_service_instance_deployment.h"
#include "platform/aas/mw/com/impl/configuration/service_instance_id.h"
#include "platform/aas/mw/com/impl/configuration/service_type_deployment.h"
#include "platform/aas/mw/com/impl/proxy_event_binding.h"

#include <amp_overload.hpp>
#include <amp_string_view.hpp>
#include <amp_variant.hpp>

#include <limits>
#include <memory>
#include <utility>

namespace bmw
{
namespace mw
{
namespace com
{
namespace impl
{

LolaServiceTypeDeployment GetLolaServiceTypeDeploymentFromServiceTypeDeployment(
    const ServiceTypeDeployment& type_deployment);

/// Converts a LolaServiceInstanceDeployment to the internal representation of an event.
///
/// Due to limitations of Lola, the service and the instance ID may not be larger than 0xFFFF, and the event ID
/// may not be larger than 0xFF. If those limitations are violated, an error is returned. Furthermore, the
/// instanceId inside the binding information must not be empty.
///
/// \param shm_binding The service instand ID obtained from service discovery.
/// \param type_deployment The mapping of the required type information onto binding specific information.
/// \param event_id The event ID of the event within the interface identified by the binding's service ID.
/// \return The fully qualified event ID, or an error.
Result<lola::ElementFqId> EventConfigToElementFqId(const ServiceInstanceId& service_instance_id,
                                                   const ServiceTypeDeployment& type_deployment,
                                                   const LolaEventId event_id) noexcept;

/// Converts a LolaServiceInstanceDeployment to the internal representation of a field.
///
/// Due to limitations of Lola, the service and the instance ID may not be larger than 0xFFFF, and the event ID
/// may not be larger than 0xFF. If those limitations are violated, an error is returned. Furthermore, the
/// instanceId inside the binding information must not be empty.
///
/// \param shm_binding The service instand ID obtained from service discovery.
/// \param type_deployment The mapping of the required type information onto binding specific information.
/// \param field_id The field ID of the field within the interface identified by the binding's service ID.
/// \return The fully qualified event ID, or an error.
Result<lola::ElementFqId> FieldConfigToElementFqId(const ServiceInstanceId& service_instance_id,
                                                   const ServiceTypeDeployment& type_deployment,
                                                   const LolaFieldId field_id) noexcept;

}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_PLUMBING_PROXY_EVENT_BINDING_TEST_RESOURCES_H
