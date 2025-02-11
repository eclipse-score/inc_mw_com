// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#ifndef PLATFORM_AAS_MW_COM_IMPL_CONFIGURATION_SERVICE_TYPE_DEPLOYMENT_H
#define PLATFORM_AAS_MW_COM_IMPL_CONFIGURATION_SERVICE_TYPE_DEPLOYMENT_H

#include "platform/aas/mw/com/impl/configuration/lola_service_type_deployment.h"

#include "platform/aas/lib/json/json_parser.h"

#include <amp_string_view.hpp>
#include <amp_variant.hpp>

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

class ServiceTypeDeployment
{
  public:
    using BindingInformation = amp::variant<LolaServiceTypeDeployment, amp::blank>;

    explicit ServiceTypeDeployment(const bmw::json::Object& json_object) noexcept;

    explicit ServiceTypeDeployment(const BindingInformation binding) noexcept;

    bmw::json::Object Serialize() const noexcept;
    amp::string_view ToHashString() const noexcept;

    BindingInformation binding_info_;

    /**
     * \brief The size of the hash string returned by ToHashString()
     *
     * The size is the max size of the hash string returned by ToHashString() from all the bindings in
     * BindingInformation plus 1 for the index of the binding type in the variant, BindingInformation.
     */
    // Variable is used in a test case -> so this line is tested and prepared for easier reuse
    // 
    constexpr static std::size_t hashStringSize{LolaServiceTypeDeployment::hashStringSize + 1U};

    constexpr static std::uint32_t serializationVersion = 1U;

  private:
    /**
     * \brief Stringified format of this ServiceTypeDeployment which can be used for hashing.
     */
    std::string hash_string_;
};

}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_CONFIGURATION_SERVICE_TYPE_DEPLOYMENT_H
