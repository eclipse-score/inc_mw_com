// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#include "platform/aas/mw/com/impl/plumbing/test/proxy_event_binding_resources.h"

#include <amp_assert.hpp>

namespace bmw
{
namespace mw
{
namespace com
{
namespace impl
{

namespace
{

template <typename IdType>
Result<lola::ElementFqId> ElementConfigToElementFqId(const ServiceInstanceId& service_instance_id,
                                                     const ServiceTypeDeployment& type_deployment,
                                                     const IdType element_id,
                                                     lola::ElementType element_type) noexcept
{
    const auto* const lola_service_instance_id = amp::get_if<LolaServiceInstanceId>(&service_instance_id.binding_info_);
    if (lola_service_instance_id == nullptr)
    {
        return MakeUnexpected(ComErrc::kInvalidBindingInformation, "No lola service instance id available.");
    }

    const auto* const lola_service_type_deployment =
        amp::get_if<LolaServiceTypeDeployment>(&type_deployment.binding_info_);
    if (lola_service_type_deployment == nullptr)
    {
        // LCOV_EXCL_LINE defensive programming: BindingToElementFqId is currently only called from
        // ProxyEventBindingFactory::Create after checking that the deployment type is lola. Therefore, this case can
        // currently not be reached.
        return MakeUnexpected(ComErrc::kInvalidBindingInformation, "No lola type deployment available.");
    }

    return lola::ElementFqId{
        lola_service_type_deployment->service_id_, element_id, lola_service_instance_id->id_, element_type};
}

}  // namespace

LolaServiceTypeDeployment GetLolaServiceTypeDeploymentFromServiceTypeDeployment(
    const ServiceTypeDeployment& type_deployment)
{
    const auto* const lola_service_type_deployment =
        amp::get_if<LolaServiceTypeDeployment>(&type_deployment.binding_info_);
    AMP_ASSERT_PRD_MESSAGE(lola_service_type_deployment != nullptr,
                           "Service type deployment should contain a Lola binding!");
    return *lola_service_type_deployment;
}

Result<lola::ElementFqId> EventConfigToElementFqId(const ServiceInstanceId& service_instance_id,
                                                   const ServiceTypeDeployment& type_deployment,
                                                   const LolaEventId event_id) noexcept
{
    return ElementConfigToElementFqId<LolaEventId>(
        service_instance_id, type_deployment, event_id, lola::ElementType::EVENT);
}

Result<lola::ElementFqId> FieldConfigToElementFqId(const ServiceInstanceId& service_instance_id,
                                                   const ServiceTypeDeployment& type_deployment,
                                                   const LolaFieldId field_id) noexcept
{
    return ElementConfigToElementFqId<LolaFieldId>(
        service_instance_id, type_deployment, field_id, lola::ElementType::FIELD);
}

}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
