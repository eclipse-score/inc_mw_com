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


#include "platform/aas/mw/com/impl/configuration/lola_event_instance_deployment.h"

#include "platform/aas/mw/com/impl/configuration/configuration_common_resources.h"

#include <amp_optional.hpp>
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

constexpr auto kSerializationVersionKey = "serializationVersion";
constexpr auto kNumberOfSampleSlotsKey = "numberOfSampleSlots";
constexpr auto kSubscribersKey = "maxSubscribers";
constexpr auto kMaxConcurrentAllocationsKey = "maxConcurrentAllocations";
constexpr auto kEnforceMaxSamplesKey = "enforceMaxSamples";

}  // namespace

LolaEventInstanceDeployment::LolaEventInstanceDeployment(amp::optional<SampleSlotCountType> number_of_sample_slots,
                                                         amp::optional<SubscriberCountType> max_subscribers,
                                                         amp::optional<std::uint8_t> max_concurrent_allocations,
                                                         const amp::optional<bool> enforce_max_samples,
                                                         bool is_tracing_enabled) noexcept
    : max_subscribers_{max_subscribers},
      max_concurrent_allocations_{max_concurrent_allocations},
      enforce_max_samples_{enforce_max_samples},
      number_of_sample_slots_{number_of_sample_slots},
      is_tracing_enabled_{is_tracing_enabled}
{
}

LolaEventInstanceDeployment::LolaEventInstanceDeployment(const bmw::json::Object& json_object) noexcept
    : LolaEventInstanceDeployment({}, {}, {}, {}, false)
{
    const auto serialization_version = GetValueFromJson<std::uint32_t>(json_object, kSerializationVersionKey);
    if (serialization_version != serializationVersion)
    {
        std::terminate();
    }

    const auto number_of_sample_slots_it = json_object.find(kNumberOfSampleSlotsKey);
    if (number_of_sample_slots_it != json_object.end())
    {
        number_of_sample_slots_ = number_of_sample_slots_it->second.As<SampleSlotCountType>();
    }

    const auto max_subscribers_it = json_object.find(kSubscribersKey);
    if (max_subscribers_it != json_object.end())
    {
        max_subscribers_ = max_subscribers_it->second.As<SubscriberCountType>();
    }

    const auto max_concurrent_allocations_it = json_object.find(kMaxConcurrentAllocationsKey);
    if (max_concurrent_allocations_it != json_object.end())
    {
        max_concurrent_allocations_ = max_concurrent_allocations_it->second.As<std::uint8_t>();
    }

    const auto enforce_max_samples_it = json_object.find(kEnforceMaxSamplesKey);
    if (enforce_max_samples_it != json_object.end())
    {
        enforce_max_samples_ = enforce_max_samples_it->second.As<bool>();
    }
}

bmw::json::Object LolaEventInstanceDeployment::Serialize() const noexcept
{
    bmw::json::Object json_object{};
    if (number_of_sample_slots_.has_value())
    {
        json_object[kNumberOfSampleSlotsKey] = bmw::json::Any{number_of_sample_slots_.value()};
    }
    if (max_subscribers_.has_value())
    {
        json_object[kSubscribersKey] = bmw::json::Any{max_subscribers_.value()};
    }
    json_object[kSerializationVersionKey] = json::Any{serializationVersion};

    if (max_concurrent_allocations_.has_value())
    {
        json_object[kMaxConcurrentAllocationsKey] = bmw::json::Any{max_concurrent_allocations_.value()};
    }

    if (enforce_max_samples_.has_value())
    {
        json_object[kEnforceMaxSamplesKey] = bmw::json::Any{enforce_max_samples_.value()};
    }

    return json_object;
}

auto LolaEventInstanceDeployment::GetNumberOfSampleSlots() const noexcept -> amp::optional<SampleSlotCountType>
{
    if (!number_of_sample_slots_.has_value())
    {
        return {};
    }
    const std::uint16_t number_of_tracing_slots = is_tracing_enabled_ ? 1U : 0U;
    return *number_of_sample_slots_ + number_of_tracing_slots;
}

auto LolaEventInstanceDeployment::GetNumberOfSampleSlotsExcludingTracingSlot() const noexcept
    -> amp::optional<SampleSlotCountType>
{
    if (!number_of_sample_slots_.has_value())
    {
        return {};
    }
    return *number_of_sample_slots_;
}

void LolaEventInstanceDeployment::SetNumberOfSampleSlots(SampleSlotCountType number_of_sample_slots) noexcept
{

    number_of_sample_slots_ = number_of_sample_slots;
}

void LolaEventInstanceDeployment::SetMaxSubscribers(SubscriberCountType max_subscribers) noexcept
{
    max_subscribers_ = max_subscribers;
}

void LolaEventInstanceDeployment::SetTracingEnabled(bool is_tracing_enabled) noexcept
{
    is_tracing_enabled_ = is_tracing_enabled;
}

bool operator==(const LolaEventInstanceDeployment& lhs, const LolaEventInstanceDeployment& rhs) noexcept
{
    const bool number_of_sample_slots_equal = (lhs.number_of_sample_slots_ == rhs.number_of_sample_slots_);
    const bool is_tracing_enabled_equal = (lhs.is_tracing_enabled_ == rhs.is_tracing_enabled_);
    const bool max_subscribers_equal = (lhs.max_subscribers_ == rhs.max_subscribers_);
    const bool max_concurrent_allocations_equal = (lhs.max_concurrent_allocations_ == rhs.max_concurrent_allocations_);
    const bool enforce_max_samples_equal = (lhs.enforce_max_samples_ == rhs.enforce_max_samples_);
    // Adding Brackets to the expression does not give additional value since only one logical operator is used which
    // is independent of the execution order
    // 
    return (number_of_sample_slots_equal && is_tracing_enabled_equal && max_subscribers_equal &&
            max_concurrent_allocations_equal && enforce_max_samples_equal);
}

}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
