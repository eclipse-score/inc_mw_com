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


#include "platform/aas/mw/com/impl/configuration/service_version_type.h"

#include "platform/aas/mw/com/impl/configuration/configuration_common_resources.h"

#include "platform/aas/lib/json/json_writer.h"

#include <amp_assert.hpp>

#include <exception>

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

constexpr auto kSerializationVersionKeySerVerType = "serializationVersion";
constexpr auto kMajorVersionKeySerVerType = "majorVersion";
constexpr auto kMinorVersionKeySerVerType = "minorVersion";

std::string ToStringImpl(const json::Object& serialized_json_object) noexcept
{
    bmw::json::JsonWriter writer{};
    const std::string serialized_form = writer.ToBuffer(serialized_json_object).value();
    return serialized_form;
}

}  // namespace

ServiceVersionType::ServiceVersionType(const std::uint32_t major_version_number,
                                       const std::uint32_t minor_version_number)
    : major_{major_version_number}, minor_{minor_version_number}, serialized_string_{ToStringImpl(Serialize())}
{
}

ServiceVersionType::ServiceVersionType(const bmw::json::Object& json_object) noexcept
    : ServiceVersionType{GetValueFromJson<std::uint32_t>(json_object, kMajorVersionKeySerVerType),
                         GetValueFromJson<std::uint32_t>(json_object, kMinorVersionKeySerVerType)}
{
    const auto serialization_version = GetValueFromJson<std::uint32_t>(json_object, kSerializationVersionKeySerVerType);
    serialized_string_ = ToStringImpl(json_object);
    if (serialization_version != serializationVersion)
    {
        std::terminate();
    }
}

auto ServiceVersionType::ToString() const noexcept -> amp::string_view
{
    return serialized_string_;
}

auto ServiceVersionType::Serialize() const noexcept -> bmw::json::Object
{
    json::Object json_object{};
    json_object[kSerializationVersionKeySerVerType] = json::Any{serializationVersion};
    json_object[kMajorVersionKeySerVerType] = bmw::json::Any{major_};
    json_object[kMinorVersionKeySerVerType] = bmw::json::Any{minor_};

    return json_object;
}

auto operator==(const ServiceVersionType& lhs, const ServiceVersionType& rhs) noexcept -> bool
{
    return (((lhs.major_ == rhs.major_) && (lhs.minor_ == rhs.minor_)));
}

auto operator<(const ServiceVersionType& lhs, const ServiceVersionType& rhs) noexcept -> bool
{
    if (lhs.major_ == rhs.major_)
    {
        return lhs.minor_ < rhs.minor_;
    }
    return lhs.major_ < rhs.major_;
}

}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
