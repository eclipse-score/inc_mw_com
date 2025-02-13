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


#include "platform/aas/mw/com/impl/configuration/service_type_deployment.h"

#include "platform/aas/mw/com/impl/configuration/configuration_common_resources.h"

#include "platform/aas/lib/json/json_parser.h"

#include <amp_overload.hpp>
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

constexpr auto kSerializationVersionKey = "serializationVersion";
constexpr auto kBindingInfoKey = "bindingInfo";
constexpr auto kBindingInfoIndexKey = "bindingInfoIndex";

ServiceTypeDeployment::BindingInformation GetBindingInfoFromJson(const bmw::json::Object& json_object) noexcept
{
    const auto variant_index = GetValueFromJson<std::ptrdiff_t>(json_object, kBindingInfoIndexKey);

    const auto binding_information =
        DeserializeVariant<0, ServiceTypeDeployment::BindingInformation>(json_object, variant_index, kBindingInfoKey);

    return binding_information;
}

std::string ToHashStringImpl(const ServiceTypeDeployment::BindingInformation& binding_info) noexcept
{
    // The conversion to hex string below does not work with a std::uint8_t, so we cast it to an int. However, we
    // ensure that the value is less than the max value of a std::uint8_t to ensure it will fit with a single char
    // in the string representation.
    static_assert(
        amp::variant_size<ServiceTypeDeployment::BindingInformation>::value <= std::numeric_limits<std::uint8_t>::max(),
        "BindingInformation variant size should be less than 256");
    auto binding_info_index = static_cast<int>(binding_info.index());

    /// \todo When we can use C++17 features, use std::to_chars so that we can convert from an int to a hex string
    /// with no dynamic allocations.
    std::stringstream binding_index_string_stream;
    binding_index_string_stream << std::hex << binding_info_index;

    auto visitor = amp::overload(
        [](const LolaServiceTypeDeployment& service_type_deployment) noexcept -> amp::string_view {
            return service_type_deployment.ToHashString();
        },
        // FP: only one statement in this line
        // 
        [](const amp::blank&) noexcept -> amp::string_view { return ""; });
    const auto binding_service_type_deployment_hash_string_view = amp::visit(visitor, binding_info);
    std::string binding_service_type_deployment_hash_string{binding_service_type_deployment_hash_string_view.data(),
                                                            binding_service_type_deployment_hash_string_view.size()};

    std::string combined_hash_string = binding_index_string_stream.str() + binding_service_type_deployment_hash_string;
    return combined_hash_string;
}

}  // namespace

ServiceTypeDeployment::ServiceTypeDeployment(const BindingInformation binding) noexcept
    : binding_info_{binding}, hash_string_{ToHashStringImpl(binding_info_)}
{
}

ServiceTypeDeployment::ServiceTypeDeployment(const bmw::json::Object& json_object) noexcept
    : binding_info_{GetBindingInfoFromJson(json_object)}, hash_string_{ToHashStringImpl(binding_info_)}
{
    const auto serialization_version = GetValueFromJson<std::uint32_t>(json_object, kSerializationVersionKey);
    if (serialization_version != serializationVersion)
    {
        std::terminate();
    }
}

bmw::json::Object ServiceTypeDeployment::Serialize() const noexcept
{
    bmw::json::Object json_object{};
    json_object[kBindingInfoIndexKey] = bmw::json::Any{binding_info_.index()};
    json_object[kSerializationVersionKey] = json::Any{serializationVersion};

    auto visitor = amp::overload(
        [&json_object](const LolaServiceTypeDeployment& deployment) {
            json_object[kBindingInfoKey] = deployment.Serialize();
        },
        [](const amp::blank&) {});
    amp::visit(visitor, binding_info_);

    return json_object;
}

amp::string_view ServiceTypeDeployment::ToHashString() const noexcept
{
    return hash_string_;
}

}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
