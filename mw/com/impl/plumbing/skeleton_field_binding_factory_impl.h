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


#ifndef PLATFORM_AAS_MW_COM_IMPL_PLUMBING_SKELETON_FIELD_BINDING_FACTORY_IMPL_H
#define PLATFORM_AAS_MW_COM_IMPL_PLUMBING_SKELETON_FIELD_BINDING_FACTORY_IMPL_H

#include "platform/aas/mw/com/impl/bindings/lola/element_fq_id.h"
#include "platform/aas/mw/com/impl/bindings/lola/skeleton_event.h"
#include "platform/aas/mw/com/impl/configuration/service_type_deployment.h"
#include "platform/aas/mw/com/impl/instance_identifier.h"
#include "platform/aas/mw/com/impl/plumbing/i_skeleton_field_binding_factory.h"
#include "platform/aas/mw/com/impl/skeleton_base.h"
#include "platform/aas/mw/com/impl/skeleton_event_binding.h"

#include <amp_string_view.hpp>

#include <functional>
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

/// \brief Factory class that dispatches calls to the appropriate binding based on binding information in the
/// deployment configuration.
template <typename SampleType>
class SkeletonFieldBindingFactoryImpl : public ISkeletonFieldBindingFactory<SampleType>
{
  public:
    std::unique_ptr<SkeletonEventBinding<SampleType>> CreateEventBinding(
        const InstanceIdentifier& identifier,
        SkeletonBase& parent,
        const amp::string_view field_name) noexcept override;
};

template <typename SampleType>
auto SkeletonFieldBindingFactoryImpl<SampleType>::CreateEventBinding(const InstanceIdentifier& identifier,
                                                                     SkeletonBase& parent,
                                                                     const amp::string_view field_name) noexcept
    -> std::unique_ptr<SkeletonEventBinding<SampleType>>
{
    
    /* (1) local variable
     *  (2) false positive in lambda expression evaluation */
    const InstanceIdentifierView identifier_view{identifier};

    auto visitor = amp::overload(
        [identifier_view = identifier_view, &parent, &field_name](
            const LolaServiceInstanceDeployment& shm_depl) -> std::unique_ptr<SkeletonEventBinding<SampleType>> {
            const auto& service_type_deployment = identifier_view.GetServiceTypeDeployment();
            const auto* const lola_service_type_deployment =
                amp::get_if<LolaServiceTypeDeployment>(&service_type_deployment.binding_info_);

            AMP_ASSERT_PRD_MESSAGE(lola_service_type_deployment != nullptr,
                                   "Wrong Binding! ServiceTypeDeployment doesn't contain a LoLa deployment! This "
                                   "should have already been checked when creating the parent skeleton.");

            auto* const lola_parent = dynamic_cast<lola::Skeleton*>(SkeletonBaseView{parent}.GetBinding());
            std::string field_name_string{field_name.data(), field_name.size()};
            const lola::ElementFqId event_info{static_cast<std::uint16_t>(lola_service_type_deployment->service_id_),
                                               lola_service_type_deployment->fields_.at(field_name_string),
                                               static_cast<std::uint16_t>(shm_depl.instance_id_.value().id_),
                                               lola::ElementType::FIELD};
            
            return std::make_unique<lola::SkeletonEvent<SampleType>>(
                *lola_parent, 
                event_info,
                field_name,
                lola::SkeletonEventProperties{shm_depl.fields_.at(field_name_string).GetNumberOfSampleSlots().value(),
                                              shm_depl.fields_.at(field_name_string).max_subscribers_.value(),
                                              shm_depl.fields_.at(field_name_string).enforce_max_samples_.value()});
            
        },
        [](const SomeIpServiceInstanceDeployment&) -> std::unique_ptr<SkeletonEventBinding<SampleType>> {
            return nullptr; /* not yet implemented */
        },
        [](const amp::blank&) -> std::unique_ptr<SkeletonEventBinding<SampleType>> { return nullptr; });

    return amp::visit(visitor, identifier_view.GetServiceInstanceDeployment().bindingInfo_);
    
}

}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_PLUMBING_SKELETON_FIELD_BINDING_FACTORY_IMPL_H
