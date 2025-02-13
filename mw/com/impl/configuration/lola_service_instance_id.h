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


#ifndef PLATFORM_AAS_MW_COM_IMPL_CONFIGURATION_LOLA_SERVICE_INSTANCE_ID_H
#define PLATFORM_AAS_MW_COM_IMPL_CONFIGURATION_LOLA_SERVICE_INSTANCE_ID_H

#include "platform/aas/lib/json/json_parser.h"

#include <amp_string_view.hpp>

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

/// \brief Struct that wraps the type of a LoLa instance ID.
///
/// Since LolaServiceInstanceId is held in a variant within ServiceInstanceId, we use a struct so that we can
/// unambiguously differentiate between the different InstanceId types when visiting the struct.
class LolaServiceInstanceId
{
  public:
    using InstanceId = std::uint16_t;

    explicit LolaServiceInstanceId(const bmw::json::Object& json_object) noexcept;
    explicit LolaServiceInstanceId(InstanceId instance_id) noexcept;

    bmw::json::Object Serialize() const noexcept;
    amp::string_view ToHashString() const noexcept;

    InstanceId id_;

    /**
     * \brief The size of the hash string returned by ToHashString()
     *
     * The size is the amount of chars required to represent InstanceId as a hex string.
     */
    constexpr static std::size_t hashStringSize{2 * sizeof(InstanceId)};

    constexpr static std::uint32_t serializationVersion{1U};

  private:
    /**
     * \brief Stringified format of this LolaServiceInstanceId which can be used for hashing.
     */
    std::string hash_string_;
};

bool operator==(const LolaServiceInstanceId& lhs, const LolaServiceInstanceId& rhs) noexcept;
bool operator<(const LolaServiceInstanceId& lhs, const LolaServiceInstanceId& rhs) noexcept;

}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_CONFIGURATION_LOLA_SERVICE_INSTANCE_ID_H
