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


#include "platform/aas/mw/com/impl/configuration/lola_service_instance_deployment.h"

#include "platform/aas/mw/com/impl/configuration/configuration_common_resources.h"

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

constexpr auto kSerializationVersionKeyInstDepl = "serializationVersion";
constexpr auto kInstanceIdKeyInstDepl = "instanceId";
constexpr auto kSharedMemorySizeKeyInstDepl = "sharedMemorySize";
constexpr auto kEventsKeyInstDepl = "events";
constexpr auto kFieldsKeyInstDepl = "fields";
constexpr auto kStrictKeyInstDepl = "strict";
constexpr auto kAllowedConsumerKeyInstDepl = "allowedConsumer";
constexpr auto kAllowedProviderKeyInstDepl = "allowedProvider";

std::unordered_map<QualityType, std::vector<uid_t>> ConvertJsonToUidMap(const json::Object& json_object,
                                                                        amp::string_view key) noexcept
{
    const auto& uid_map_json = GetValueFromJson<json::Object>(json_object, key);

    std::unordered_map<QualityType, std::vector<uid_t>> uid_map{};
    for (auto& it : uid_map_json)
    {
        std::string quality_string{it.first.GetAsStringView().data(), it.first.GetAsStringView().size()};
        const QualityType quality_type{FromString(std::move(quality_string))};
        const auto& uids_json = it.second.As<bmw::json::List>().value().get();

        std::vector<uid_t> uids{};
        for (auto& uid_json : uids_json)
        {
            uids.push_back(uid_json.As<uid_t>().value());
        }

        const auto insert_result = uid_map.insert(std::make_pair(quality_type, std::move(uids)));
        AMP_ASSERT_PRD_MESSAGE(insert_result.second, "Could not insert element in map");
    }
    return uid_map;
}

json::Object ConvertUidMapToJson(const std::unordered_map<QualityType, std::vector<uid_t>>& input_map) noexcept
{
    json::Object json_object{};
    for (auto it : input_map)
    {
        const QualityType& quality_type{it.first};
        auto quality_type_string = ToString(quality_type);

        const std::vector<uid_t>& uids{it.second};
        json::List uids_json{uids.begin(), uids.end()};
        const auto insert_result =
            json_object.insert(std::make_pair(std::move(quality_type_string), std::move(uids_json)));
        AMP_ASSERT_PRD_MESSAGE(insert_result.second, "Could not insert element in map");
    }
    return json_object;
}

}  // namespace

auto areCompatible(const LolaServiceInstanceDeployment& lhs, const LolaServiceInstanceDeployment& rhs) noexcept -> bool
{
    if (((lhs.instance_id_.has_value()) == false) || (rhs.instance_id_.has_value() == false))
    {
        return true;
    }
    return lhs.instance_id_ == rhs.instance_id_;
}

bool operator==(const LolaServiceInstanceDeployment& lhs, const LolaServiceInstanceDeployment& rhs) noexcept
{
    return lhs.instance_id_ == rhs.instance_id_;
}

// In this case the constructor delegation does not provide additional code structuring because of the amp::optional
// By adding a third private constructor additional complexity would be introduced
// 
LolaServiceInstanceDeployment::LolaServiceInstanceDeployment(const bmw::json::Object& json_object) noexcept
    : instance_id_{},
      shared_memory_size_{},
      events_{ConvertJsonToServiceElementMap<EventInstanceMapping>(json_object, kEventsKeyInstDepl)},
      fields_{ConvertJsonToServiceElementMap<FieldInstanceMapping>(json_object, kFieldsKeyInstDepl)},
      strict_permissions_{GetValueFromJson<bool>(json_object, kStrictKeyInstDepl)},
      allowed_consumer_{ConvertJsonToUidMap(json_object, kAllowedConsumerKeyInstDepl)},
      allowed_provider_{ConvertJsonToUidMap(json_object, kAllowedProviderKeyInstDepl)}
{
    const auto serialization_version = GetValueFromJson<std::uint32_t>(json_object, kSerializationVersionKeyInstDepl);
    if (serialization_version != serializationVersion)
    {
        std::terminate();
    }

    const auto instance_id_it = json_object.find(kInstanceIdKeyInstDepl);
    if (instance_id_it != json_object.end())
    {
        instance_id_ = instance_id_it->second.As<json::Object>().value();
    }

    const auto shared_memory_size_it = json_object.find(kSharedMemorySizeKeyInstDepl);
    if (shared_memory_size_it != json_object.end())
    {
        shared_memory_size_ = shared_memory_size_it->second.As<std::size_t>().value();
    }
}

LolaServiceInstanceDeployment::LolaServiceInstanceDeployment(const LolaServiceInstanceId instance_id,
                                                             EventInstanceMapping events,
                                                             FieldInstanceMapping fields,
                                                             const bool strict_permission) noexcept
    : instance_id_{instance_id},
      shared_memory_size_{},
      events_{std::move(events)},
      fields_{std::move(fields)},
      strict_permissions_{strict_permission},
      allowed_consumer_{},
      allowed_provider_{}
{
}

bmw::json::Object LolaServiceInstanceDeployment::Serialize() const noexcept
{
    json::Object json_object{};
    json_object[kSerializationVersionKeyInstDepl] = bmw::json::Any{serializationVersion};

    if (instance_id_.has_value())
    {
        json_object[kInstanceIdKeyInstDepl] = instance_id_.value().Serialize();
    }

    if (shared_memory_size_.has_value())
    {
        json_object[kSharedMemorySizeKeyInstDepl] = bmw::json::Any{shared_memory_size_.value()};
    }

    json_object[kEventsKeyInstDepl] = ConvertServiceElementMapToJson(events_);
    json_object[kFieldsKeyInstDepl] = ConvertServiceElementMapToJson(fields_);

    json_object[kStrictKeyInstDepl] = strict_permissions_;

    json_object[kAllowedConsumerKeyInstDepl] = ConvertUidMapToJson(allowed_consumer_);
    json_object[kAllowedProviderKeyInstDepl] = ConvertUidMapToJson(allowed_provider_);

    return json_object;
}

bool LolaServiceInstanceDeployment::ContainsEvent(const std::string& event_name) const noexcept
{
    return (events_.find(event_name) != events_.end());
}

bool LolaServiceInstanceDeployment::ContainsField(const std::string& field_name) const noexcept
{
    return (fields_.find(field_name) != fields_.end());
}

}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
