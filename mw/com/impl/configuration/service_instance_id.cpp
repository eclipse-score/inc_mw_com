// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#include "platform/aas/mw/com/impl/configuration/service_instance_id.h"

#include "platform/aas/mw/com/impl/configuration/configuration_common_resources.h"

#include "platform/aas/lib/json/json_parser.h"

#include <amp_overload.hpp>

#include <exception>
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

constexpr auto kBindingInfoKeySerInstID = "bindingInfo";
constexpr auto kBindingInfoIndexKeySerInstID = "bindingInfoIndex";
constexpr auto kSerializationVersionKeySerInstID = "serializationVersion";

ServiceInstanceId::BindingInformation GetBindingInfoFromJson(const bmw::json::Object& json_object) noexcept
{
    const auto variant_index = GetValueFromJson<std::ptrdiff_t>(json_object, kBindingInfoIndexKeySerInstID);

    const auto binding_information = DeserializeVariant<0, ServiceInstanceId::BindingInformation>(
        json_object, variant_index, kBindingInfoKeySerInstID);

    return binding_information;
}

std::string ToHashStringImpl(const ServiceInstanceId::BindingInformation& binding_info) noexcept
{
    // The conversion to hex string below does not work with a std::uint8_t, so we cast it to an int. However, we
    // ensure that the value is less than the max value of a std::uint8_t to ensure it will fit with a single char
    // in the string representation.
    static_assert(
        amp::variant_size<ServiceInstanceId::BindingInformation>::value <= std::numeric_limits<std::uint8_t>::max(),
        "BindingInformation variant size should be less than 256");
    auto binding_info_index = static_cast<int>(binding_info.index());

    /// \todo When we can use C++17 features, use std::to_chars so that we can convert from an int to a hex string
    /// with no dynamic allocations.
    std::stringstream binding_index_string_stream;
    binding_index_string_stream << std::hex << binding_info_index;

    auto visitor = amp::overload(
        [](const LolaServiceInstanceId& instance_id) noexcept -> amp::string_view {
            return instance_id.ToHashString();
        },
        [](const SomeIpServiceInstanceId& instance_id) noexcept -> amp::string_view {
            return instance_id.ToHashString();
        },
        [](const amp::blank&) noexcept -> amp::string_view { return ""; });
    const auto binding_instance_id_hash_string_view = amp::visit(visitor, binding_info);
    std::string binding_instance_id_hash_string{binding_instance_id_hash_string_view.data(),
                                                binding_instance_id_hash_string_view.size()};

    std::string combined_hash_string = binding_index_string_stream.str() + binding_instance_id_hash_string;
    return combined_hash_string;
}

}  // namespace

ServiceInstanceId::ServiceInstanceId(BindingInformation binding_info) noexcept
    : binding_info_{std::move(binding_info)}, hash_string_{ToHashStringImpl(binding_info_)}
{
}

ServiceInstanceId::ServiceInstanceId(const bmw::json::Object& json_object) noexcept
    : binding_info_{GetBindingInfoFromJson(json_object)}, hash_string_{ToHashStringImpl(binding_info_)}
{
    const auto serialization_version = GetValueFromJson<std::uint32_t>(json_object, kSerializationVersionKeySerInstID);
    if (serialization_version != serializationVersion)
    {
        std::terminate();
    }
}

bmw::json::Object ServiceInstanceId::Serialize() const noexcept
{
    bmw::json::Object json_object{};
    json_object[kBindingInfoIndexKeySerInstID] = bmw::json::Any{binding_info_.index()};
    json_object[kSerializationVersionKeySerInstID] = json::Any{serializationVersion};

    auto visitor = amp::overload(
        [&json_object](const LolaServiceInstanceId& instance_id) {
            json_object[kBindingInfoKeySerInstID] = instance_id.Serialize();
        },
        [&json_object](const SomeIpServiceInstanceId& instance_id) {
            json_object[kBindingInfoKeySerInstID] = instance_id.Serialize();
        },
        [](const amp::blank&) {});
    amp::visit(visitor, binding_info_);
    return json_object;
}

amp::string_view ServiceInstanceId::ToHashString() const noexcept
{
    return hash_string_;
}

bool operator==(const ServiceInstanceId& lhs, const ServiceInstanceId& rhs) noexcept
{
    auto visitor = amp::overload(
        [&rhs](const LolaServiceInstanceId& lhs_lola) -> bool {
            const auto* const rhs_lola = amp::get_if<LolaServiceInstanceId>(&rhs.binding_info_);
            if (rhs_lola == nullptr)
            {
                return false;
            }
            return lhs_lola == *rhs_lola;
        },
        [lhs, rhs](const SomeIpServiceInstanceId& lhs_someip) -> bool {
            const auto* const rhs_someip = amp::get_if<SomeIpServiceInstanceId>(&rhs.binding_info_);
            if (rhs_someip == nullptr)
            {
                return false;
            }
            return lhs_someip == *rhs_someip;
        },
        [](const amp::blank&) -> bool { return true; });
    return amp::visit(visitor, lhs.binding_info_);
}

bool operator<(const ServiceInstanceId& lhs, const ServiceInstanceId& rhs) noexcept
{
    auto visitor = amp::overload(
        [&rhs](const LolaServiceInstanceId& lhs_lola) -> bool {
            const auto* const rhs_lola = amp::get_if<LolaServiceInstanceId>(&rhs.binding_info_);
            if (rhs_lola == nullptr)
            {
                return false;
            }
            return lhs_lola < *rhs_lola;
        },
        [lhs, rhs](const SomeIpServiceInstanceId& lhs_someip) -> bool {
            const auto* const rhs_someip = amp::get_if<SomeIpServiceInstanceId>(&rhs.binding_info_);
            if (rhs_someip == nullptr)
            {
                return false;
            }
            return lhs_someip < *rhs_someip;
        },
        // FP: only one statement in this line
        // 
        [](const amp::blank&) -> bool { return true; });
    return amp::visit(visitor, lhs.binding_info_);
}

}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
