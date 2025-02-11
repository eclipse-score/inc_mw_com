// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#ifndef PLATFORM_AAS_MW_COM_IMPL_CONFIGURATION_CONFIGURATION_COMMON_RESOURCES_H
#define PLATFORM_AAS_MW_COM_IMPL_CONFIGURATION_CONFIGURATION_COMMON_RESOURCES_H

#include "platform/aas/lib/json/json_parser.h"

#include <amp_assert.hpp>
#include <amp_string_view.hpp>

#include <type_traits>

namespace bmw
{
namespace mw
{
namespace com
{
namespace impl
{

/// \brief Helper function to get a value from a json object.
///
/// Since json::Object::As can either return a value (e.g. for integer types, amp::string_view etc.) or a
/// reference_wrapper (e.g. for json::Object), we need to return the same from this function.
template <typename T,
          typename = std::enable_if_t<!std::is_arithmetic<T>::value>,
          typename = std::enable_if_t<!std::is_same<T, amp::string_view>::value, bool>>
auto GetValueFromJson(const bmw::json::Object& json_object, amp::string_view key) noexcept -> const T&
{
    const auto it = json_object.find(key);
    AMP_ASSERT(it != json_object.end());
    return it->second.As<T>().value();
}

template <typename T, typename = std::enable_if_t<std::is_arithmetic<T>::value, bool>>
auto GetValueFromJson(const bmw::json::Object& json_object, amp::string_view key) noexcept -> T
{
    const auto it = json_object.find(key);
    AMP_ASSERT(it != json_object.end());
    return it->second.As<T>().value();
}

template <typename T, typename = std::enable_if_t<std::is_same<T, amp::string_view>::value, bool>>
auto GetValueFromJson(const bmw::json::Object& json_object, amp::string_view key) noexcept -> amp::string_view
{
    const auto it = json_object.find(key);
    AMP_ASSERT(it != json_object.end());
    return it->second.As<T>().value();
}

template <typename VariantHeldType>
class DoConstructVariant
{
  public:
    static VariantHeldType Do(const bmw::json::Object& json_object, amp::string_view json_variant_key) noexcept
    {
        const auto& binding_info_json_object = GetValueFromJson<json::Object>(json_object, json_variant_key);
        return VariantHeldType{binding_info_json_object};
    }
};

template <>
class DoConstructVariant<amp::blank>
{
  public:
    static amp::blank Do(const bmw::json::Object&, amp::string_view) noexcept { return amp::blank{}; }
};

/// ConstructVariant dispatches to DoConstructVariant in a template class to avoid function template specialization
template <typename VariantHeldType>
auto ConstructVariant(const bmw::json::Object& json_object, amp::string_view json_variant_key) noexcept
    -> VariantHeldType
{
    return DoConstructVariant<VariantHeldType>::Do(json_object, json_variant_key);
}

/// \brief Helper function to deserialize a variant from a json Object
///
/// Accessing the type of a variant using an index can only be done at compile time. This class generates template
/// functions for each possible type within a variant such that a variant type can be created at runtime using an index.
///
/// \param json_object The json object containing the serialized variant
/// \param variant_index The index of the variant (taken from amp::variant::index() which should be serialized /
/// deserialized along with the variant itself)
/// \param json_variant_key The key for the variant in the json object
template <std::size_t VariantIndex,
          typename VariantType,
          typename std::enable_if<VariantIndex == amp::variant_size<VariantType>::value>::type* = nullptr>
auto DeserializeVariant(const bmw::json::Object& /* json_object */,
                        std::ptrdiff_t /* variant_index */,
                        amp::string_view /* json_variant_key */) noexcept -> VariantType
{
    std::terminate();
}

template <std::size_t VariantIndex,
          typename VariantType,
          typename std::enable_if<VariantIndex<amp::variant_size<VariantType>::value>::type* = nullptr> auto
              DeserializeVariant(const bmw::json::Object& json_object,
                                 std::ptrdiff_t variant_index,
                                 amp::string_view json_variant_key) noexcept -> VariantType
{
    if (variant_index == VariantIndex)
    {
        using VariantHeldType = typename amp::variant_alternative<VariantIndex, VariantType>::type;
        VariantHeldType deployment{ConstructVariant<VariantHeldType>(json_object, json_variant_key)};
        return VariantType{std::move(deployment)};
    }
    else
    {
        return DeserializeVariant<VariantIndex + 1, VariantType>(json_object, variant_index, json_variant_key);
    }
}

template <typename ServiceElementInstanceMapping>
json::Object ConvertServiceElementMapToJson(const ServiceElementInstanceMapping& input_map) noexcept
{
    json::Object service_element_instance_mapping_object{};
    for (auto it : input_map)
    {
        const auto insert_result =
            service_element_instance_mapping_object.insert(std::make_pair(it.first, it.second.Serialize()));
        AMP_ASSERT_PRD_MESSAGE(insert_result.second, "Could not insert element in map");
    }
    return service_element_instance_mapping_object;
}

template <typename ServiceElementInstanceMapping>
auto ConvertJsonToServiceElementMap(const json::Object& json_object, amp::string_view key) noexcept
    -> ServiceElementInstanceMapping
{
    const auto& service_element_json = GetValueFromJson<json::Object>(json_object, key);

    ServiceElementInstanceMapping service_element_map{};
    for (auto& it : service_element_json)
    {
        const amp::string_view event_name_view{it.first.GetAsStringView()};
        const std::string event_name{event_name_view.data(), event_name_view.size()};
        auto event_instance_deployment_json = it.second.As<bmw::json::Object>().value();
        const auto insert_result =
            service_element_map.insert(std::make_pair(event_name, event_instance_deployment_json));
        AMP_ASSERT_PRD_MESSAGE(insert_result.second, "Could not insert element in map");
    }
    return service_element_map;
}

}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_CONFIGURATION_CONFIGURATION_COMMON_RESOURCES_H
