// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#ifndef PLATFORM_AAS_MW_COM_IMPL_CONFIGURATION_CONFIGURATION_ERROR_H
#define PLATFORM_AAS_MW_COM_IMPL_CONFIGURATION_CONFIGURATION_ERROR_H

#include "platform/aas/lib/result/result.h"

namespace bmw
{
namespace mw
{
namespace com
{
namespace impl
{

/// \brief error codes, which can occur, when trying to create an InstanceIdentifier from a string representation
/// \details These error codes and the corresponding error domain are a preparation for a later/upcoming implementation
///          of static bmw::Result<InstanceIdentifier> InstanceIdentifier::Create(amp_string_view serializedFormat).
///          Right now, it isn't used from core functionality.
enum class configuration_errc : bmw::result::ErrorCode
{
    serialization_deploymentinformation_invalid = 0,
    serialization_no_shmbindinginformation = 1,
    serialization_shmbindinginformation_invalid = 2,
    serialization_someipbindinginformation_invalid = 3,
    serialization_no_someipbindinginformation = 4,
};

/// \brief See above explanation in configuration_errc
class ConfigurationErrorDomain final : public bmw::result::ErrorDomain
{

  public:
    /* Gcc compiler bug leads to a compiler warning if override is not added, even if final keyword is there. */
    // \todo Gcc compiler bug leads to a compiler warning if override is not added, even if final keyword is there
    // (https://gcc.gnu.org/bugzilla/show_bug.cgi?id=78010). When bug is fixed, remove the override keyword from the
    // MessageFor function signature and the coverity suppression.
    // 
    amp::string_view MessageFor(const bmw::result::ErrorCode& errorCode) const noexcept override final
    {
        switch (errorCode)
        {
            case static_cast<bmw::result::ErrorCode>(configuration_errc::serialization_deploymentinformation_invalid):
                return "serialization of <DeploymentInformation> is invalid";
            case static_cast<bmw::result::ErrorCode>(configuration_errc::serialization_no_shmbindinginformation):
                return "no serialization of <LoLaShmBindingInfo>";
            case static_cast<bmw::result::ErrorCode>(configuration_errc::serialization_shmbindinginformation_invalid):
                return "serialization of <LoLaShmBindingInfo> is invalid";
            case static_cast<bmw::result::ErrorCode>(configuration_errc::serialization_no_someipbindinginformation):
                return "no serialization of <SomeIpBindingInfo>";
            case static_cast<bmw::result::ErrorCode>(
                configuration_errc::serialization_someipbindinginformation_invalid):
                return "serialization of <SomeIpBindingInfo> is invalid";
            default:
                return "unknown configuration error";
        }
    }
};

bmw::result::Error MakeError(const configuration_errc code, const std::string_view message = "");

}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_CONFIGURATION_CONFIGURATION_ERROR_H
