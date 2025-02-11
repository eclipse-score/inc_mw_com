// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#include "platform/aas/mw/com/impl/instance_identifier.h"

#include "platform/aas/mw/com/impl/com_error.h"
#include "platform/aas/mw/com/impl/configuration/configuration_common_resources.h"

#include "platform/aas/lib/json/json_writer.h"
#include "platform/aas/mw/log/logging.h"

#include <amp_overload.hpp>

#include <exception>
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

constexpr auto kServiceInstanceDeploymentKey = "serviceInstanceDeployment";
constexpr auto kServiceTypeDeploymentKey = "serviceTypeDeployment";
constexpr auto kSerializationVersionKey = "serializationVersion";

std::string ToStringImpl(const json::Object& serialized_json_object) noexcept
{
    bmw::json::JsonWriter writer{};
    const std::string serialized_form = writer.ToBuffer(serialized_json_object).value();
    return serialized_form;
}

}  // namespace

Configuration* InstanceIdentifier::configuration_{nullptr};

bmw::Result<InstanceIdentifier> InstanceIdentifier::Create(amp::string_view serialized_format) noexcept
{
    if (configuration_ == nullptr)
    {
        ::bmw::mw::log::LogFatal("lola") << "InstanceIdentifier configuration pointer hasn't been set. Exiting";
        return MakeUnexpected(ComErrc::kInvalidConfiguration);
    }
    json::JsonParser json_parser{};
    auto json_result = json_parser.FromBuffer(serialized_format.data());
    if (!json_result.has_value())
    {
        ::bmw::mw::log::LogFatal("lola") << "InstanceIdentifier serialized string is invalid. Exiting";
        return MakeUnexpected(ComErrc::kInvalidInstanceIdentifierString);
    }
    const auto& json_object = std::move(json_result).value().As<json::Object>().value().get();
    std::string serialized_string{serialized_format.data(), serialized_format.size()};
    InstanceIdentifier instance_identifier{json_object, std::move(serialized_string)};
    return instance_identifier;
}

InstanceIdentifier::InstanceIdentifier(const json::Object& json_object, std::string serialized_string) noexcept
    : instance_deployment_{nullptr}, type_deployment_{nullptr}, serialized_string_{std::move(serialized_string)}
{
    const auto serialization_version = GetValueFromJson<std::uint32_t>(json_object, kSerializationVersionKey);
    if (serialization_version != serializationVersion)
    {
        ::bmw::mw::log::LogFatal("lola") << "InstanceIdentifier serialization versions don't match. "
                                         << serialization_version << "!=" << serializationVersion << ". Terminating.";
        std::terminate();
    }

    ServiceInstanceDeployment instance_deployment{
        GetValueFromJson<json::Object>(json_object, kServiceInstanceDeploymentKey)};
    ServiceTypeDeployment type_deployment{GetValueFromJson<json::Object>(json_object, kServiceTypeDeploymentKey)};

    auto service_identifier_type = instance_deployment.service_;
    const auto* const type_deployment_ptr =
        configuration_->AddServiceTypeDeployment(std::move(service_identifier_type), std::move(type_deployment));
    if (type_deployment_ptr == nullptr)
    {
        ::bmw::mw::log::LogFatal("lola")
            << "Could not insert service type deployment into configuration map. Terminating.";
        std::terminate();
    }
    type_deployment_ = type_deployment_ptr;

    auto instance_specifier = instance_deployment.instance_specifier_;
    const auto* const service_instance_deployment_ptr =
        configuration_->AddServiceInstanceDeployments(std::move(instance_specifier), std::move(instance_deployment));
    if (service_instance_deployment_ptr == nullptr)
    {
        ::bmw::mw::log::LogFatal("lola") << "Could not insert instance deployment into configuration map. Terminating.";
        std::terminate();
    }
    instance_deployment_ = service_instance_deployment_ptr;
}

InstanceIdentifier::InstanceIdentifier(const ServiceInstanceDeployment& deployment,
                                       const ServiceTypeDeployment& type_deployment) noexcept
    : instance_deployment_{&deployment},
      type_deployment_{&type_deployment},
      serialized_string_{ToStringImpl(Serialize())}
{
}

auto InstanceIdentifier::Serialize() const noexcept -> json::Object
{
    json::Object json_object{};
    json_object[kSerializationVersionKey] = bmw::json::Any{serializationVersion};

    json_object[kServiceInstanceDeploymentKey] = instance_deployment_->Serialize();
    json_object[kServiceTypeDeploymentKey] = type_deployment_->Serialize();

    return json_object;
}

auto InstanceIdentifier::ToString() const noexcept -> amp::string_view
{
    return serialized_string_;
}

auto operator==(const InstanceIdentifier& lhs, const InstanceIdentifier& rhs) noexcept -> bool
{
    return (((lhs.instance_deployment_->service_ == rhs.instance_deployment_->service_) &&
             (*lhs.instance_deployment_ == *rhs.instance_deployment_)));
}

auto operator<(const InstanceIdentifier& lhs, const InstanceIdentifier& rhs) noexcept -> bool
{
    return (((lhs.instance_deployment_->service_ < rhs.instance_deployment_->service_) ||
             (*lhs.instance_deployment_ < *rhs.instance_deployment_)));
}

InstanceIdentifierView::InstanceIdentifierView(const InstanceIdentifier& identifier) : identifier_{identifier} {}

auto InstanceIdentifierView::GetServiceInstanceId() const noexcept -> amp::optional<ServiceInstanceId>
{
    auto visitor = amp::overload(
        [](const LolaServiceInstanceDeployment& deployment) -> amp::optional<ServiceInstanceId> {
            if (!deployment.instance_id_.has_value())
            {
                return {};
            }
            return ServiceInstanceId{*deployment.instance_id_};
        },
        [](const SomeIpServiceInstanceDeployment& deployment) -> amp::optional<ServiceInstanceId> {
            if (!deployment.instance_id_.has_value())
            {
                return {};
            }
            return ServiceInstanceId{*deployment.instance_id_};
        },
        [](const amp::blank&) -> amp::optional<ServiceInstanceId> { return amp::optional<ServiceInstanceId>{}; });
    return amp::visit(visitor, GetServiceInstanceDeployment().bindingInfo_);
}

auto InstanceIdentifierView::GetServiceInstanceDeployment() const -> const ServiceInstanceDeployment&
{
    return *identifier_.instance_deployment_;
}

auto InstanceIdentifierView::GetServiceTypeDeployment() const -> const ServiceTypeDeployment&
{
    return *identifier_.type_deployment_;
}

auto InstanceIdentifierView::isCompatibleWith(const InstanceIdentifier& rhs) const -> bool
{
    return areCompatible(*identifier_.instance_deployment_, *rhs.instance_deployment_);
}

auto InstanceIdentifierView::isCompatibleWith(const InstanceIdentifierView& rhs) const -> bool
{
    return areCompatible(*identifier_.instance_deployment_, *rhs.identifier_.instance_deployment_);
}

}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

auto std::hash<bmw::mw::com::impl::InstanceIdentifier>::operator()(
    const bmw::mw::com::impl::InstanceIdentifier& instance_identifier) const noexcept -> std::size_t
{
    return std::hash<amp::string_view>()(instance_identifier.ToString());
}
