// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#include "platform/aas/mw/com/impl/configuration/someip_service_instance_deployment.h"

#include "platform/aas/mw/com/impl/configuration/configuration_common_resources.h"

#include "platform/aas/lib/json/json_writer.h"

#include <cstdlib>
#include <exception>
#include <limits>
#include <sstream>
#include <string>

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

constexpr auto kSerializationVersionKeySerInstDepl = "serializationVersion";
constexpr auto kInstanceIdKeySerInstDepl = "instanceId";
constexpr auto kEventsKeySerInstDepl = "events";
constexpr auto kFieldsKeySerInstDepl = "fields";

}  // namespace

auto areCompatible(const SomeIpServiceInstanceDeployment& lhs, const SomeIpServiceInstanceDeployment& rhs) -> bool
{
    if ((lhs.instance_id_.has_value() == false) || (rhs.instance_id_.has_value() == false))
    {
        return true;
    }
    return lhs.instance_id_ == rhs.instance_id_;
}

bool operator==(const SomeIpServiceInstanceDeployment& lhs, const SomeIpServiceInstanceDeployment& rhs) noexcept
{
    return (lhs.instance_id_ == rhs.instance_id_);
}

SomeIpServiceInstanceDeployment::SomeIpServiceInstanceDeployment(const bmw::json::Object& json_object) noexcept
    : SomeIpServiceInstanceDeployment{
          {},
          ConvertJsonToServiceElementMap<EventInstanceMapping>(json_object, kEventsKeySerInstDepl),
          ConvertJsonToServiceElementMap<FieldInstanceMapping>(json_object, kFieldsKeySerInstDepl)}
{
    const auto instance_id_it = json_object.find(kInstanceIdKeySerInstDepl);
    if (instance_id_it != json_object.end())
    {
        instance_id_ = instance_id_it->second.As<json::Object>().value();
    }

    const auto serialization_version =
        GetValueFromJson<std::uint32_t>(json_object, kSerializationVersionKeySerInstDepl);
    if (serialization_version != serializationVersion)
    {
        std::terminate();
    }
}

bmw::json::Object SomeIpServiceInstanceDeployment::Serialize() const noexcept
{
    bmw::json::Object json_object{};
    json_object[kSerializationVersionKeySerInstDepl] = bmw::json::Any{serializationVersion};

    if (instance_id_.has_value())
    {
        json_object[kInstanceIdKeySerInstDepl] = instance_id_.value().Serialize();
    }

    json_object[kEventsKeySerInstDepl] = ConvertServiceElementMapToJson(events_);
    json_object[kFieldsKeySerInstDepl] = ConvertServiceElementMapToJson(fields_);

    return json_object;
}

}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
