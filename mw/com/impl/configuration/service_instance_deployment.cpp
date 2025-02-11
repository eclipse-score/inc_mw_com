// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#include "platform/aas/mw/com/impl/configuration/service_instance_deployment.h"

#include "platform/aas/mw/com/impl/configuration/lola_service_instance_deployment.h"

#include "platform/aas/mw/com/impl/configuration/configuration_common_resources.h"

#include "platform/aas/lib/json/json_parser.h"

#include <amp_overload.hpp>
#include <amp_variant.hpp>

#include <exception>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>

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
constexpr auto kAsilLevelKey = "asilLevel";
constexpr auto kInstanceSpecifierKey = "instanceSpecifier";
constexpr auto kServiceKey = "service";

QualityType GetQualityTypeFromJson(const bmw::json::Object& json_object, amp::string_view key) noexcept
{
    const auto it = json_object.find(key);
    AMP_ASSERT(it != json_object.end());
    return FromString(it->second.As<std::string>().value());
}

ServiceInstanceDeployment::BindingInformation GetBindingInfoFromJson(const bmw::json::Object& json_object) noexcept
{
    const auto variant_index = GetValueFromJson<std::ptrdiff_t>(json_object, kBindingInfoIndexKey);

    const auto binding_information = DeserializeVariant<0, ServiceInstanceDeployment::BindingInformation>(
        json_object, variant_index, kBindingInfoKey);

    return binding_information;
}

}  // namespace

auto operator==(const ServiceInstanceDeployment& lhs, const ServiceInstanceDeployment& rhs) noexcept -> bool
{
    return (((lhs.asilLevel_ == rhs.asilLevel_) && (lhs.bindingInfo_ == rhs.bindingInfo_)));
}

auto operator<(const ServiceInstanceDeployment& lhs, const ServiceInstanceDeployment& rhs) noexcept -> bool
{
    bool bindingLess{false};

    const auto* const lhsShmBindingInfo = amp::get_if<LolaServiceInstanceDeployment>(&lhs.bindingInfo_);
    const auto* const rhsShmBindingInfo = amp::get_if<LolaServiceInstanceDeployment>(&rhs.bindingInfo_);
    if ((lhsShmBindingInfo != nullptr) && (rhsShmBindingInfo != nullptr))
    {
        bindingLess = lhsShmBindingInfo->instance_id_ < rhsShmBindingInfo->instance_id_;
    }
    else
    {
        const auto* const lhsSomeIpBindingInfo = amp::get_if<SomeIpServiceInstanceDeployment>(&lhs.bindingInfo_);
        const auto* const rhsSomeIpBindingInfo = amp::get_if<SomeIpServiceInstanceDeployment>(&rhs.bindingInfo_);
        if ((lhsSomeIpBindingInfo != nullptr) && (rhsSomeIpBindingInfo != nullptr))
        {
            bindingLess = lhsSomeIpBindingInfo->instance_id_ == rhsSomeIpBindingInfo->instance_id_;
        }
    }
    return (((lhs.asilLevel_ < rhs.asilLevel_) && bindingLess));
}

auto areCompatible(const ServiceInstanceDeployment& lhs, const ServiceInstanceDeployment& rhs) -> bool
{
    bool bindingCompatible{false};
    const auto* const lhsShmBindingInfo = amp::get_if<LolaServiceInstanceDeployment>(&lhs.bindingInfo_);
    const auto* const rhsShmBindingInfo = amp::get_if<LolaServiceInstanceDeployment>(&rhs.bindingInfo_);
    if ((lhsShmBindingInfo != nullptr) && (rhsShmBindingInfo != nullptr))
    {
        bindingCompatible = areCompatible(*lhsShmBindingInfo, *rhsShmBindingInfo);
    }
    else
    {
        const auto* const lhsSomeIpBindingInfo = amp::get_if<SomeIpServiceInstanceDeployment>(&lhs.bindingInfo_);
        const auto* const rhsSomeIpBindingInfo = amp::get_if<SomeIpServiceInstanceDeployment>(&rhs.bindingInfo_);
        if ((lhsSomeIpBindingInfo != nullptr) && (rhsSomeIpBindingInfo != nullptr))
        {
            bindingCompatible = areCompatible(*lhsSomeIpBindingInfo, *rhsSomeIpBindingInfo);
        }
    }
    return areCompatible(lhs.asilLevel_, rhs.asilLevel_) && bindingCompatible;
}

ServiceInstanceDeployment::ServiceInstanceDeployment(const bmw::json::Object& json_object) noexcept
    : ServiceInstanceDeployment(
          ServiceIdentifierType{GetValueFromJson<json::Object>(json_object, kServiceKey)},
          GetBindingInfoFromJson(json_object),
          GetQualityTypeFromJson(json_object, kAsilLevelKey),
          InstanceSpecifier::Create(GetValueFromJson<std::string>(json_object, kInstanceSpecifierKey)).value())
{
    const auto serialization_version = GetValueFromJson<std::uint32_t>(json_object, kSerializationVersionKey);
    if (serialization_version != serializationVersion)
    {
        std::terminate();
    }
}

bmw::json::Object ServiceInstanceDeployment::Serialize() const noexcept
{
    bmw::json::Object json_object{};
    json_object[kBindingInfoIndexKey] = bmw::json::Any{bindingInfo_.index()};

    auto visitor = amp::overload(
        [&json_object](const LolaServiceInstanceDeployment& deployment) {
            json_object[kBindingInfoKey] = deployment.Serialize();
        },
        [&json_object](const SomeIpServiceInstanceDeployment& deployment) {
            json_object[kBindingInfoKey] = deployment.Serialize();
        },
        [](const amp::blank&) {});
    amp::visit(visitor, bindingInfo_);

    json_object[kAsilLevelKey] = json::Any{ToString(asilLevel_)};
    json_object[kServiceKey] = service_.Serialize();

    const auto instance_specifier_view = instance_specifier_.ToString();
    json_object[kInstanceSpecifierKey] =
        json::Any{std::string{instance_specifier_view.data(), instance_specifier_view.size()}};
    json_object[kSerializationVersionKey] = json::Any{serializationVersion};

    return json_object;
}

BindingType ServiceInstanceDeployment::GetBindingType() const noexcept
{
    // FP: only one statement in this line
    // 
    auto visitor = amp::overload([](const LolaServiceInstanceDeployment&) { return BindingType::kLoLa; },
                                 // FP: only one statement in this line
                                 // 
                                 [](const SomeIpServiceInstanceDeployment&) { return BindingType::kSomeIp; },
                                 [](const amp::blank&) { return BindingType::kFake; });
    return amp::visit(visitor, bindingInfo_);
}

}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
