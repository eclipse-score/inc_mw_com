// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#include "platform/aas/mw/com/impl/skeleton_base.h"

#include "platform/aas/mw/com/impl/plumbing/skeleton_binding_factory.h"
#include "platform/aas/mw/com/impl/runtime.h"
#include "platform/aas/mw/com/impl/skeleton_binding.h"
#include "platform/aas/mw/com/impl/skeleton_event_base.h"
#include "platform/aas/mw/com/impl/skeleton_field_base.h"
#include "platform/aas/mw/com/impl/tracing/skeleton_tracing.h"

#include "platform/aas/mw/log/logging.h"

#include <amp_utility.hpp>

#include <algorithm>
#include <exception>
#include <unordered_map>
#include <utility>
#include <vector>

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

void StopOfferServiceInServiceDiscovery(const InstanceIdentifier& instance_identifier) noexcept
{
    const auto result = Runtime::getInstance().GetServiceDiscovery().StopOfferService(instance_identifier);
    if (!result.has_value())
    {
        bmw::mw::log::LogError("lola") << "SkeletonBinding::OfferService failed: service discovery could not stop offer"
                                       << result.error().Message() << ": " << result.error().UserMessage();
    }
}

SkeletonBinding::SkeletonEventBindings GetSkeletonEventBindingsMap(const SkeletonBase::SkeletonEvents& events)
{
    SkeletonBinding::SkeletonEventBindings event_bindings{};
    for (const auto& event : events)
    {
        const amp::string_view event_name = event.first;
        SkeletonEventBase& skeleton_event_base = event.second.get();

        auto skeleton_event_base_view = SkeletonEventBaseView{skeleton_event_base};
        auto* event_binding = skeleton_event_base_view.GetBinding();
        AMP_ASSERT_PRD_MESSAGE(event_binding != nullptr,
                               "Skeleton should not have been created if event binding failed to create.");
        event_bindings.insert({event_name, *event_binding});
    }
    return event_bindings;
}

SkeletonBinding::SkeletonFieldBindings GetSkeletonFieldBindingsMap(const SkeletonBase::SkeletonFields& fields)
{
    SkeletonBinding::SkeletonFieldBindings field_bindings{};
    for (const auto& field : fields)
    {
        const amp::string_view field_name = field.first;
        SkeletonFieldBase& skeleton_field_base = field.second.get();

        auto skeleton_field_base_view = SkeletonFieldBaseView{skeleton_field_base};
        auto* event_binding = skeleton_field_base_view.GetEventBinding();
        AMP_ASSERT_PRD_MESSAGE(event_binding != nullptr,
                               "Skeleton should not have been created if event binding failed to create.");
        field_bindings.insert({field_name, *event_binding});
    }
    return field_bindings;
}

}  // namespace

SkeletonBase::SkeletonBase(std::unique_ptr<SkeletonBinding> skeleton_binding,
                           const InstanceIdentifier instance_id,
                           MethodCallProcessingMode)
    : binding_{std::move(skeleton_binding)}, events_{}, fields_{}, instance_id_{instance_id}, service_offered_flag_{}
{
}

SkeletonBase::~SkeletonBase()
{
    Cleanup();
}

void SkeletonBase::Cleanup()
{
    // The SkeletonBase is responsible for calling PrepareStopOffer on the skeleton binding when the SkeletonBase is
    // destroyed. The SkeletonEventBase is responsible for calling PrepareStopOffer on the SkeletonEvent binding as the
    // SkeletonEventBases are owned by the child class of SkeletonBase and will therefore be fully destroyed before
    // ~SkeletonBase() is called.
    if (service_offered_flag_.IsSet())
    {
        StopOfferServiceInServiceDiscovery(instance_id_);
        auto tracing_handler = tracing::CreateUnregisterShmObjectCallback(instance_id_, events_, fields_, *binding_);

        binding_->PrepareStopOffer(std::move(tracing_handler));
        service_offered_flag_.Clear();
    }
}

SkeletonBase::SkeletonBase(SkeletonBase&& other) noexcept
    : binding_{std::move(other.binding_)},
      events_{std::move(other.events_)},
      fields_{std::move(other.fields_)},
      instance_id_{std::move(other.instance_id_)},
      service_offered_flag_{std::move(other.service_offered_flag_)}
{
    // Since the address of this skeleton has changed, we need update the address stored in each of the events and
    // fields belonging to the skeleton.
    for (auto& event : events_)
    {
        event.second.get().UpdateSkeletonReference(*this);
    }

    for (auto& field : fields_)
    {
        field.second.get().UpdateSkeletonReference(*this);
    }
}

SkeletonBase& SkeletonBase::operator=(SkeletonBase&& other) noexcept
{
    if (this != &other)
    {
        Cleanup();
        binding_ = std::move(other.binding_);
        events_ = std::move(other.events_);
        fields_ = std::move(other.fields_);
        instance_id_ = std::move(other.instance_id_);
        service_offered_flag_ = std::move(other.service_offered_flag_);

        // Since the address of this skeleton has changed, we need update the address stored in each of the events and
        // fields belonging to the skeleton.
        for (auto& event : events_)
        {
            event.second.get().UpdateSkeletonReference(*this);
        }

        for (auto& field : fields_)
        {
            field.second.get().UpdateSkeletonReference(*this);
        }
    }
    return *this;
}

bmw::ResultBlank SkeletonBase::OfferServiceEvents() const noexcept
{
    for (auto& event : events_)
    {
        const auto event_name = event.first;
        auto& skeleton_event = event.second.get();
        const auto offer_result = skeleton_event.PrepareOffer();
        if (!offer_result.has_value())
        {
            bmw::mw::log::LogError("lola")
                << "SkeletonBinding::OfferService failed for event" << event_name
                << ": Reason:" << offer_result.error().Message() << ": " << offer_result.error().UserMessage();
            return MakeUnexpected(ComErrc::kBindingFailure);
        }
    }
    return {};
}

bmw::ResultBlank SkeletonBase::OfferServiceFields() const noexcept
{
    for (auto& field : fields_)
    {
        const auto field_name = field.first;
        auto& skeleton_field = field.second.get();
        const auto offer_result = skeleton_field.PrepareOffer();
        if (!offer_result.has_value())
        {
            bmw::mw::log::LogError("lola")
                << "SkeletonBinding::OfferService failed for field" << field_name
                << ": Reason:" << offer_result.error().Message() << ": " << offer_result.error().UserMessage();
            if (offer_result.error() == ComErrc::kFieldValueIsNotValid)
            {
                return MakeUnexpected(ComErrc::kFieldValueIsNotValid);
            }
            return MakeUnexpected(ComErrc::kBindingFailure);
        }
    }
    return {};
}

auto SkeletonBase::OfferService() noexcept -> ResultBlank
{
    if (binding_ != nullptr)
    {
        auto event_bindings = GetSkeletonEventBindingsMap(events_);
        auto field_bindings = GetSkeletonFieldBindingsMap(fields_);

        auto register_shm_object_callback =
            tracing::CreateRegisterShmObjectCallback(instance_id_, events_, fields_, *binding_);

        const auto result =
            binding_->PrepareOffer(event_bindings, field_bindings, std::move(register_shm_object_callback));
        if (!result.has_value())
        {
            bmw::mw::log::LogError("lola") << "SkeletonBinding::OfferService failed: " << result.error().Message()
                                           << ": " << result.error().UserMessage();
            return MakeUnexpected(ComErrc::kBindingFailure);
        }

        const auto event_verification_result = OfferServiceEvents();
        if (!event_verification_result.has_value())
        {
            return event_verification_result;
        }

        const auto fields_verification_result = OfferServiceFields();
        if (!fields_verification_result.has_value())
        {
            return fields_verification_result;
        }

        service_offered_flag_.Set();
        const auto finalize_offer_result = binding_->FinalizeOffer();
        if (!finalize_offer_result.has_value())
        {
            bmw::mw::log::LogError("lola")
                << "SkeletonBinding::OfferService failed: Could not finalize offer"
                << finalize_offer_result.error().Message() << ": " << finalize_offer_result.error().UserMessage();
            return MakeUnexpected(ComErrc::kBindingFailure);
        }

        const auto service_discovery_offer_result =
            Runtime::getInstance().GetServiceDiscovery().OfferService(instance_id_);
        if (!service_discovery_offer_result.has_value())
        {
            bmw::mw::log::LogError("lola")
                << "SkeletonBinding::OfferService failed: service discovery could not start offer"
                << service_discovery_offer_result.error().Message() << ": "
                << service_discovery_offer_result.error().UserMessage();
            return MakeUnexpected(ComErrc::kBindingFailure);
        }
    }
    else
    {
        bmw::mw::log::LogFatal("lola") << "Trying to call OfferService() on a skeleton WITHOUT a binding!";
        std::terminate();
    }
    return {};
}

auto SkeletonBase::StopOfferService() noexcept -> void
{
    if (binding_ != nullptr && service_offered_flag_.IsSet())
    {
        StopOfferServiceInServiceDiscovery(instance_id_);

        for (auto& event : events_)
        {
            event.second.get().PrepareStopOffer();
        }
        for (auto& field : fields_)
        {
            field.second.get().PrepareStopOffer();
        }

        auto tracing_handler = tracing::CreateUnregisterShmObjectCallback(instance_id_, events_, fields_, *binding_);
        binding_->PrepareStopOffer(std::move(tracing_handler));
        service_offered_flag_.Clear();
        mw::log::LogInfo("lola") << "Service was stop offered successfully";
    }
}

auto SkeletonBase::AreBindingsValid() const noexcept -> bool
{
    const bool is_skeleton_binding_valid{binding_ != nullptr};
    bool are_service_element_bindings_valid{true};

    std::for_each(events_.begin(), events_.end(), [&are_service_element_bindings_valid](const auto& element) {
        if (SkeletonEventBaseView{element.second.get()}.GetBinding() == nullptr)
        {
            are_service_element_bindings_valid = false;
        }
    });
    std::for_each(fields_.begin(), fields_.end(), [&are_service_element_bindings_valid](const auto& element) {
        if (SkeletonFieldBaseView{element.second.get()}.GetEventBinding() == nullptr)
        {
            are_service_element_bindings_valid = false;
        }
    });
    return is_skeleton_binding_valid && are_service_element_bindings_valid;
}

amp::optional<InstanceIdentifier> GetInstanceIdentifier(const InstanceSpecifier& specifier)
{
    const auto instance_identifiers = Runtime::getInstance().resolve(specifier);
    if (instance_identifiers.size() != std::size_t{1U})
    {
        return {};
    }
    const auto instance_identifier = instance_identifiers.front();
    return instance_identifier;
}

}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
