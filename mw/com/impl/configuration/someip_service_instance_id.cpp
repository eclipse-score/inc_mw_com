// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#include "platform/aas/mw/com/impl/configuration/someip_service_instance_id.h"

#include "platform/aas/mw/com/impl/configuration/configuration_common_resources.h"

#include "platform/aas/lib/json/json_parser.h"

#include <exception>
#include <iomanip>
#include <limits>
#include <sstream>

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

constexpr auto kInstanceIdKey = "instanceId";
constexpr auto kSerializationVersionKey = "serializationVersion";

std::string ToHashStringImpl(const SomeIpServiceInstanceId::InstanceId instance_id,
                             const std::size_t hash_string_size) noexcept
{
    /// \todo When we can use C++17 features, use std::to_chars so that we can convert from an int to a hex string
    /// with no dynamic allocations.
    std::stringstream stream;
    AMP_ASSERT_PRD(hash_string_size <= std::numeric_limits<int>::max());
    stream << std::setfill('0') << std::setw(static_cast<int>(hash_string_size)) << std::hex << instance_id;
    return stream.str();
}

}  // namespace

SomeIpServiceInstanceId::SomeIpServiceInstanceId(InstanceId instance_id) noexcept
    : id_{instance_id}, hash_string_{ToHashStringImpl(id_, hashStringSize)}
{
}

SomeIpServiceInstanceId::SomeIpServiceInstanceId(const bmw::json::Object& json_object) noexcept
    : id_{GetValueFromJson<InstanceId>(json_object, kInstanceIdKey)},
      hash_string_{ToHashStringImpl(id_, hashStringSize)}
{
    const auto serialization_version = GetValueFromJson<std::uint32_t>(json_object, kSerializationVersionKey);
    if (serialization_version != serializationVersion)
    {
        std::terminate();
    }
}

bmw::json::Object SomeIpServiceInstanceId::Serialize() const noexcept
{
    bmw::json::Object json_object{};
    json_object[kInstanceIdKey] = bmw::json::Any{id_};
    json_object[kSerializationVersionKey] = json::Any{serializationVersion};
    return json_object;
}

amp::string_view SomeIpServiceInstanceId::ToHashString() const noexcept
{
    return hash_string_;
}

bool operator==(const SomeIpServiceInstanceId& lhs, const SomeIpServiceInstanceId& rhs) noexcept
{
    return lhs.id_ == rhs.id_;
}

bool operator<(const SomeIpServiceInstanceId& lhs, const SomeIpServiceInstanceId& rhs) noexcept
{
    return lhs.id_ < rhs.id_;
}

}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
