// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#ifndef PLATFORM_AAS_MW_COM_IMPL_CONFIGURATION_SERVICE_INSTANCE_DEPLOYMENT_H
#define PLATFORM_AAS_MW_COM_IMPL_CONFIGURATION_SERVICE_INSTANCE_DEPLOYMENT_H

#include "platform/aas/mw/com/impl/binding_type.h"
#include "platform/aas/mw/com/impl/com_error.h"
#include "platform/aas/mw/com/impl/configuration/lola_service_instance_deployment.h"
#include "platform/aas/mw/com/impl/configuration/quality_type.h"
#include "platform/aas/mw/com/impl/configuration/service_identifier_type.h"
#include "platform/aas/mw/com/impl/configuration/someip_service_instance_deployment.h"
#include "platform/aas/mw/com/impl/instance_specifier.h"

#include "platform/aas/lib/json/json_parser.h"

#include <amp_blank.hpp>
#include <amp_string_view.hpp>
#include <amp_variant.hpp>

#include <cstdint>
#include <string>
#include <utility>

namespace bmw
{
namespace mw
{
namespace com
{
namespace impl
{

// Keep struct type even if it breaks Autosar rule A11-0-2 that says: not provide any special member functions or
// methods (such as user defined constructor, ToString(), etc). Note the struct is not compliant to POD type containing
// non-POD member and methods. But public access to members is still required by implementation, hence the best choice
// is to keep struct here. Apparently, it's a first candidate for further refactoring and switching it to a class.
// NOLINTBEGIN(bmw-struct-usage-compliance): Intended struct semantic.
// coverity [autosar_cpp14_a11_0_2_violation]
struct ServiceInstanceDeployment
// NOLINTEND(bmw-struct-usage-compliance): see above
{
  public:
    using BindingInformation = amp::variant<LolaServiceInstanceDeployment, SomeIpServiceInstanceDeployment, amp::blank>;

    explicit ServiceInstanceDeployment(const bmw::json::Object& json_object) noexcept;
    ServiceInstanceDeployment(const ServiceIdentifierType service,
                              BindingInformation binding,
                              const QualityType asil_level,
                              InstanceSpecifier instance_specifier)
        : service_{service},
          bindingInfo_{std::move(binding)},
          asilLevel_{asil_level},
          instance_specifier_{std::move(instance_specifier)}
    {
    }

    ~ServiceInstanceDeployment() noexcept = default;

    ServiceInstanceDeployment(const ServiceInstanceDeployment& other) = default;
    ServiceInstanceDeployment& operator=(const ServiceInstanceDeployment& other) = default;
    ServiceInstanceDeployment(ServiceInstanceDeployment&& other) = default;
    ServiceInstanceDeployment& operator=(ServiceInstanceDeployment&& other) = default;

    bmw::json::Object Serialize() const noexcept;

    BindingType GetBindingType() const noexcept;

    ServiceIdentifierType service_;
    BindingInformation bindingInfo_;
    QualityType asilLevel_;
    InstanceSpecifier instance_specifier_;
    constexpr static std::uint32_t serializationVersion = 1U;
};



bool operator==(const ServiceInstanceDeployment& lhs, const ServiceInstanceDeployment& rhs) noexcept;
bool operator<(const ServiceInstanceDeployment& lhs, const ServiceInstanceDeployment& rhs) noexcept;
bool areCompatible(const ServiceInstanceDeployment& lhs, const ServiceInstanceDeployment& rhs);


}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_CONFIGURATION_SERVICE_INSTANCE_DEPLOYMENT_H
