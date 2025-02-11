// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#ifndef PLATFORM_AAS_MW_COM_IMPL_CONFIGURATION_LOLA_EVENT_INSTANCE_DEPLOYMENT_H
#define PLATFORM_AAS_MW_COM_IMPL_CONFIGURATION_LOLA_EVENT_INSTANCE_DEPLOYMENT_H

#include "platform/aas/lib/json/json_parser.h"

#include <amp_optional.hpp>

#include <cstdint>
#include <string>

namespace bmw
{
namespace mw
{
namespace com
{
namespace impl
{

class LolaEventInstanceDeployment
{
  public:
    using SampleSlotCountType = std::uint16_t;
    using SubscriberCountType = std::uint8_t;

    explicit LolaEventInstanceDeployment(amp::optional<SampleSlotCountType> number_of_sample_slots = {},
                                         amp::optional<SubscriberCountType> max_subscribers = {},
                                         amp::optional<std::uint8_t> max_concurrent_allocations = 1U,
                                         const amp::optional<bool> enforce_max_samples = true,
                                         bool is_tracing_enabled = false) noexcept;
    explicit LolaEventInstanceDeployment(const bmw::json::Object& json_object) noexcept;

    bmw::json::Object Serialize() const noexcept;

    void SetNumberOfSampleSlots(SampleSlotCountType number_of_sample_slots) noexcept;
    void SetMaxSubscribers(SubscriberCountType max_subscribers) noexcept;
    void SetTracingEnabled(bool is_tracing_enabled) noexcept;

    amp::optional<SampleSlotCountType> GetNumberOfSampleSlots() const noexcept;
    amp::optional<SampleSlotCountType> GetNumberOfSampleSlotsExcludingTracingSlot() const noexcept;

    /// \brief max subscribers slots is only relevant/required on skeleton side. On the proxy side it is irrelevant.
    ///         Therefore it is optional!
    amp::optional<SubscriberCountType> max_subscribers_;
    amp::optional<std::uint8_t> max_concurrent_allocations_;
    amp::optional<bool> enforce_max_samples_;

    constexpr static std::uint32_t serializationVersion = 1U;

    friend bool operator==(const LolaEventInstanceDeployment& lhs, const LolaEventInstanceDeployment& rhs) noexcept;

  private:
    /// \brief number of sample slots is only relevant/required on skeleton side, where slots get allocated. On the
    ///         proxy side it is irrelevant. Therefore it is optional!
    amp::optional<SampleSlotCountType> number_of_sample_slots_;
    bool is_tracing_enabled_;
};

bool operator==(const LolaEventInstanceDeployment& lhs, const LolaEventInstanceDeployment& rhs) noexcept;

}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_CONFIGURATION_LOLA_EVENT_INSTANCE_DEPLOYMENT_H
