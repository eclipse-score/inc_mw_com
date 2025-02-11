// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#include "platform/aas/mw/com/impl/configuration/service_identifier_type.h"

#include "platform/aas/mw/com/impl/configuration/configuration_common_resources.h"

#include "platform/aas/lib/json/json_writer.h"

#include <amp_string_view.hpp>

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

constexpr auto kSerializationVersionKeyServIdentType = "serializationVersion";
constexpr auto kServiceTypeKeyServIdentType = "serviceType";
constexpr auto kVersionKeyServIdentType = "version";

std::string ToStringImpl(const json::Object& serialized_json_object) noexcept
{
    bmw::json::JsonWriter writer{};
    const std::string serialized_form = writer.ToBuffer(serialized_json_object).value();
    return serialized_form;
}

}  // namespace

ServiceIdentifierType::ServiceIdentifierType(std::string serviceTypeName, ServiceVersionType version)
    : serviceTypeName_{std::move(serviceTypeName)}, version_{version}, serialized_string_{ToStringImpl(Serialize())}
{
}

ServiceIdentifierType::ServiceIdentifierType(std::string serviceTypeName,
                                             const std::uint32_t major_version_number,
                                             const std::uint32_t minor_version_number)
    : ServiceIdentifierType(std::move(serviceTypeName),
                            make_ServiceVersionType(major_version_number, minor_version_number))
{
}

ServiceIdentifierType::ServiceIdentifierType(const bmw::json::Object& json_object) noexcept
    : ServiceIdentifierType(
          GetValueFromJson<amp::string_view>(json_object, kServiceTypeKeyServIdentType).data(),
          static_cast<ServiceVersionType>(GetValueFromJson<json::Object>(json_object, kVersionKeyServIdentType)))
{
    const auto serialization_version =
        GetValueFromJson<std::uint32_t>(json_object, kSerializationVersionKeyServIdentType);
    if (serialization_version != serializationVersion)
    {
        std::terminate();
    }
}

auto ServiceIdentifierType::ToString() const noexcept -> amp::string_view
{
    return serviceTypeName_;
}

auto ServiceIdentifierType::Serialize() const noexcept -> bmw::json::Object
{
    json::Object json_object{};
    json_object[kServiceTypeKeyServIdentType] = bmw::json::Any{std::string{serviceTypeName_}};
    json_object[kVersionKeyServIdentType] = bmw::json::Any{version_.Serialize()};
    json_object[kSerializationVersionKeyServIdentType] = json::Any{serializationVersion};
    return json_object;
}


auto operator==(const ServiceIdentifierType& lhs, const ServiceIdentifierType& rhs) noexcept -> bool
{
    return (((amp::string_view{lhs.serviceTypeName_} == amp::string_view{rhs.serviceTypeName_}) &&
             (lhs.version_ == rhs.version_)));
}


auto operator<(const ServiceIdentifierType& lhs, const ServiceIdentifierType& rhs) noexcept -> bool
{
    if (amp::string_view{lhs.serviceTypeName_} == amp::string_view{rhs.serviceTypeName_})
    {
        return lhs.version_ < rhs.version_;
    }
    return amp::string_view{lhs.serviceTypeName_} < amp::string_view{rhs.serviceTypeName_};
}

}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
