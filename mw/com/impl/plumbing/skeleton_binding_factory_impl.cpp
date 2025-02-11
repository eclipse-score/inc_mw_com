// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#include "platform/aas/mw/com/impl/plumbing/skeleton_binding_factory_impl.h"

#include "platform/aas/mw/com/impl/bindings/lola/partial_restart_path_builder.h"
#include "platform/aas/mw/com/impl/bindings/lola/shm_path_builder.h"

#include "platform/aas/mw/com/impl/bindings/lola/skeleton.h"

#include "platform/aas/lib/filesystem/filesystem.h"

#include <amp_overload.hpp>
#include <amp_variant.hpp>

#include <memory>

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

template <typename LolaInstanceMapping>
bool IsServiceElementConfigurationValid(const LolaInstanceMapping& mapping, const amp::span<const char* const> names)
{
    for (const auto event_name : names)
    {
        auto search = mapping.find(event_name);
        if (search == mapping.end())
        {
            return false;
        }

        if (search->second.max_concurrent_allocations_.has_value() == false)
        {
            return false;
        }

        if ((!search->second.GetNumberOfSampleSlots().has_value()) ||
            (search->second.GetNumberOfSampleSlots().value() == 0) || (!search->second.max_subscribers_.has_value()) ||
            (search->second.max_subscribers_.value() == 0U) ||
            (search->second.max_concurrent_allocations_.value() == 0U))
        {
            return false;
        }
    }
    return true;
}

const LolaServiceTypeDeployment& GetLolaServiceTypeDeploymentFromInstanceIdentifier(
    const InstanceIdentifier& identifier) noexcept
{
    const auto& service_type_depl_info = InstanceIdentifierView{identifier}.GetServiceTypeDeployment();
    AMP_ASSERT_PRD_MESSAGE(amp::holds_alternative<LolaServiceTypeDeployment>(service_type_depl_info.binding_info_),
                           "Wrong Binding! ServiceTypeDeployment doesn't contain a LoLa deployment!");
    return amp::get<LolaServiceTypeDeployment>(service_type_depl_info.binding_info_);
}

}  // namespace

auto SkeletonBindingFactoryImpl::Create(const InstanceIdentifier& identifier) noexcept
    -> std::unique_ptr<SkeletonBinding>
{
    const InstanceIdentifierView identifier_view{identifier};

    auto visitor = amp::overload(
        [&identifier](const LolaServiceInstanceDeployment&) -> std::unique_ptr<SkeletonBinding> {
            bmw::filesystem::Filesystem filesystem = filesystem::FilesystemFactory{}.CreateInstance();
            auto shm_path_builder = std::make_unique<lola::ShmPathBuilder>(
                GetLolaServiceTypeDeploymentFromInstanceIdentifier(identifier).service_id_);
            auto partial_restart_path_builder = std::make_unique<lola::PartialRestartPathBuilder>(
                GetLolaServiceTypeDeploymentFromInstanceIdentifier(identifier).service_id_);
            return lola::Skeleton::Create(
                identifier, filesystem, std::move(shm_path_builder), std::move(partial_restart_path_builder));
        },
        [](const SomeIpServiceInstanceDeployment&) -> std::unique_ptr<SkeletonBinding> {
            return nullptr; /* not yet implemented */
        },
        [](const amp::blank&) -> std::unique_ptr<SkeletonBinding> { return nullptr; });
    return amp::visit(visitor, identifier_view.GetServiceInstanceDeployment().bindingInfo_);
}

}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
