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


#include "platform/aas/mw/com/impl/bindings/lola/service_discovery/known_instances_container.h"

#include <amp_assert.hpp>

namespace bmw
{
namespace mw
{
namespace com
{
namespace impl
{
namespace lola
{

auto KnownInstancesContainer::Insert(const EnrichedInstanceIdentifier& enriched_instance_identifier) noexcept -> bool
{
    const auto instance_id = enriched_instance_identifier.GetBindingSpecificInstanceId<LolaServiceInstanceId>();
    if (instance_id.has_value())
    {
        std::unordered_set<LolaServiceInstanceId::InstanceId> instance_ids{};
        amp::ignore = instance_ids.emplace(instance_id.value());
        const auto result = known_instances_.emplace(
            enriched_instance_identifier.GetBindingSpecificServiceId<LolaServiceTypeDeployment>().value(),
            instance_ids);
        if (!(result.second))
        {
            auto& existing_instance_ids = result.first->second;
            if (existing_instance_ids.find(instance_id.value()) != existing_instance_ids.end())
            {
                return false;
            }
            result.first->second.merge(std::move(instance_ids));
        }
        return true;
    }
    return false;
}

auto KnownInstancesContainer::Remove(const EnrichedInstanceIdentifier& enriched_instance_identifier) noexcept -> void
{
    const auto instance_id = enriched_instance_identifier.GetBindingSpecificInstanceId<LolaServiceInstanceId>();
    if (instance_id.has_value())
    {
        const auto it = known_instances_.find(
            enriched_instance_identifier.GetBindingSpecificServiceId<LolaServiceTypeDeployment>().value());
        if (it != known_instances_.end())
        {
            amp::ignore = it->second.erase(instance_id.value());
        }
    }
}

auto KnownInstancesContainer::GetKnownHandles(
    const EnrichedInstanceIdentifier& enriched_instance_identifier) const noexcept -> std::vector<HandleType>
{
    std::vector<HandleType> handles{};

    const auto it = known_instances_.find(
        enriched_instance_identifier.GetBindingSpecificServiceId<LolaServiceTypeDeployment>().value());
    if (it == known_instances_.end())
    {
        return handles;
    }

    const auto& known_service_instances = it->second;
    const auto handle_instance_id = enriched_instance_identifier.GetBindingSpecificInstanceId<LolaServiceInstanceId>();
    if (handle_instance_id.has_value())
    {
        if (known_service_instances.find(handle_instance_id.value()) != known_service_instances.cend())
        {
            handles.push_back(make_HandleType(enriched_instance_identifier.GetInstanceIdentifier(),
                                              LolaServiceInstanceId{handle_instance_id.value()}));
        }
    }
    else
    {
        for (const auto& instance_id : known_service_instances)
        {
            handles.push_back(make_HandleType(enriched_instance_identifier.GetInstanceIdentifier(),
                                              LolaServiceInstanceId{instance_id}));
        }
    }

    return handles;
}

auto KnownInstancesContainer::Merge(KnownInstancesContainer&& container_to_be_merged) noexcept -> void
{
    while (!container_to_be_merged.known_instances_.empty())
    {
        // we don't need std::move here as std::unordered_map::extract returns a node handle that owns the extracted
        // element.
        // 
        const auto to_be_merged_it = container_to_be_merged.known_instances_.begin();
        // 
        auto node = container_to_be_merged.known_instances_.extract(to_be_merged_it);

        auto base_it = known_instances_.find(node.key());
        if (base_it == known_instances_.cend())
        {
            const auto insertion_result = known_instances_.insert(std::move(node));
            AMP_ASSERT_PRD_MESSAGE(insertion_result.inserted, "Insertion failed");
        }
        else
        {
            base_it->second.merge(std::move(node.mapped()));
        }
    }
}

auto KnownInstancesContainer::Empty() const noexcept -> bool
{
    return known_instances_.empty();
}

}  // namespace lola
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw