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


#ifndef PLATFORM_AAS_MW_COM_IMPL_ENRICHED_INSTANCE_IDENTIFIER_H
#define PLATFORM_AAS_MW_COM_IMPL_ENRICHED_INSTANCE_IDENTIFIER_H

#include "platform/aas/mw/com/impl/configuration/service_instance_id.h"
#include "platform/aas/mw/com/impl/handle_type.h"
#include "platform/aas/mw/com/impl/instance_identifier.h"

#include <amp_assert.hpp>
#include <amp_optional.hpp>

namespace bmw
{
namespace mw
{
namespace com
{
namespace impl
{

/// \brief Mutable wrapper class around an InstanceIdentifier which allows to modify different attributes
///
/// Difference between EnrichedInstanceIdentifier, InstanceIdentifier and HandleType:
///     - InstanceIdentifier: Non-mutable object which is generated purely from the configuration. It contains an
///     optional ServiceInstanceId which is set in the general case and is not set when it is used for a FindAny search.
///     - HandleType: Contains an InstanceIdentifier. Also contains a ServiceInstanceId which is filled on construction
///     by the ServiceInstanceId from the InstanceIdentifier if it has one, otherwise, by a ServiceInstanceId that is
///     passed into the constructor (which would be found in the FindAny search). A HandleType must always contain a
///     valid ServiceInstanceId.
///     - EnrichedInstanceIdentifier: Allows overwriting of some internal attributes instance identifiers.
class EnrichedInstanceIdentifier final
{
  public:
    explicit EnrichedInstanceIdentifier(InstanceIdentifier instance_identifier) noexcept
        : instance_identifier_{std::move(instance_identifier)},
          instance_id_{InstanceIdentifierView{instance_identifier_}.GetServiceInstanceId()},
          quality_type_{InstanceIdentifierView{instance_identifier_}.GetServiceInstanceDeployment().asilLevel_}
    {
    }

    EnrichedInstanceIdentifier(InstanceIdentifier instance_identifier, const ServiceInstanceId instance_id) noexcept
        : instance_identifier_{std::move(instance_identifier)},
          instance_id_{instance_id},
          quality_type_{InstanceIdentifierView{instance_identifier_}.GetServiceInstanceDeployment().asilLevel_}
    {
        const bool config_contains_instance_id =
            InstanceIdentifierView{instance_identifier_}.GetServiceInstanceId().has_value();
        AMP_PRECONDITION_PRD_MESSAGE(!config_contains_instance_id,
                                     "An ServiceInstanceId should only be provided to EnrichedInstanceIdentifier if "
                                     "one doesn't exist in the config.");
    }

    EnrichedInstanceIdentifier(EnrichedInstanceIdentifier instance_identifier, const QualityType quality_type) noexcept
        : instance_identifier_{instance_identifier.GetInstanceIdentifier()},
          instance_id_{instance_identifier.GetInstanceId()},
          quality_type_{quality_type}
    {
    }

    explicit EnrichedInstanceIdentifier(const HandleType handle) noexcept
        : instance_identifier_{handle.GetInstanceIdentifier()},
          instance_id_{handle.GetInstanceId()},
          quality_type_{InstanceIdentifierView{instance_identifier_}.GetServiceInstanceDeployment().asilLevel_}
    {
    }

    const InstanceIdentifier& GetInstanceIdentifier() const noexcept { return instance_identifier_; }

    template <typename ServiceTypeDeployment>

    amp::optional<typename ServiceTypeDeployment::ServiceId> GetBindingSpecificServiceId() const noexcept
    {
        const InstanceIdentifierView instance_identifier_view{instance_identifier_};
        const auto* service_deployment =
            amp::get_if<ServiceTypeDeployment>(&(instance_identifier_view.GetServiceTypeDeployment().binding_info_));
        if (service_deployment == nullptr)
        {
            return amp::nullopt;
        }
        return service_deployment->service_id_;
    }

    const amp::optional<ServiceInstanceId>& GetInstanceId() const noexcept { return instance_id_; }

    template <typename ServiceInstanceId>
    amp::optional<typename ServiceInstanceId::InstanceId> GetBindingSpecificInstanceId() const noexcept
    {
        if (!instance_id_.has_value())
        {
            return amp::nullopt;
        }

        const auto* instance_id = amp::get_if<ServiceInstanceId>(&(instance_id_->binding_info_));
        if (instance_id == nullptr)
        {
            return amp::nullopt;
        }
        return instance_id->id_;
    }

    QualityType GetQualityType() const noexcept { return quality_type_; }

  private:
    InstanceIdentifier instance_identifier_;
    amp::optional<ServiceInstanceId> instance_id_;
    QualityType quality_type_;
};

bool operator==(const EnrichedInstanceIdentifier& lhs, const EnrichedInstanceIdentifier& rhs) noexcept;

}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_ENRICHED_INSTANCE_IDENTIFIER_H
