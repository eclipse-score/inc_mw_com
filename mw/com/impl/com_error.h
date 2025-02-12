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


#ifndef PLATFORM_AAS_MW_COM_IMPL_COMERROR_H
#define PLATFORM_AAS_MW_COM_IMPL_COMERROR_H

#include "platform/aas/lib/result/result.h"

namespace bmw
{
namespace mw
{
namespace com
{
namespace impl
{

/**
 * @brief error codes of ara::com API as standardized
 *
 * \requirement 
 */
enum class ComErrc : bmw::result::ErrorCode
{
    kServiceNotAvailable = 1,
    kMaxSamplesReached,
    kBindingFailure,
    kGrantEnforcementError,
    kPeerIsUnreachable,
    kFieldValueIsNotValid,
    kSetHandlerNotSet,
    kUnsetFailure,
    kSampleAllocationFailure,
    kIllegalUseOfAllocate,
    kServiceNotOffered,
    kCommunicationLinkError,
    kNoClients,
    kCommunicationStackError,
    kMaxSampleCountNotRealizable,
    kMaxSubscribersExceeded,
    kWrongMethodCallProcessingMode,
    kErroneousFileHandle,
    kCouldNotExecute,
    kInvalidInstanceIdentifierString,
    kInvalidBindingInformation,
    kEventNotExisting,
    kNotSubscribed,
    kInvalidConfiguration,
    kInvalidMetaModelShortname,
    kServiceInstanceAlreadyOffered,
    kCouldNotRestartProxy,
    kNotOffered,
    kInstanceIDCouldNotBeResolved,
    kFindServiceHandlerFailure,
    kInvalidHandle,
};

/**
 * @brief Error domain for ara::com (CommunicationManagement)
 *
 * \requirement 
 */
// This switch-case statement implements a "mapping table". Length of the mapping table does not add complexity!
class ComErrorDomain final : public bmw::result::ErrorDomain
{
  public:
    
    /* Gcc compiler bug leads to a compiler warning if override is not added, even if final keyword is there. */
    /// \todo Gcc compiler bug leads to a compiler warning if override is not added, even if final keyword is there
    /// (https://gcc.gnu.org/bugzilla/show_bug.cgi?id=78010). When bug is fixed, remove the override keyword from the
    /// MessageFor function signature and the AUTOSAR.MEMB.VIRTUAL.SPEC klocwork suppression.
    amp::string_view MessageFor(const bmw::result::ErrorCode& errorCode) const noexcept override final
    
    {
        /* Note: This lengthy switch-case lead to a klocwork CYCLOMATICCOMPLEXITY and LINESOFCODEMETHOD warnings. We set
         * status to "defer" for both with the following argument/comment within the klocwork UI:
         * Switch-case statement is used for a simple error-code->error-message lookup. It does not add real complexity.
         * There is no other good solution for such a mapping, which is constexpr/compile time capable!
         * Anything, which is runtime, would need a complete refactoring of the public API or even the introduction of
         * synchronization, which is a no-go. */
        switch (errorCode)
        {
            case static_cast<bmw::result::ErrorCode>(ComErrc::kServiceNotAvailable):
                return "Service is not available.";
            case static_cast<bmw::result::ErrorCode>(ComErrc::kMaxSamplesReached):
                return "Application holds more SamplePtrs than commited in Subscribe().";
            case static_cast<bmw::result::ErrorCode>(ComErrc::kBindingFailure):
                return "Local failure has been detected by the binding.";
            case static_cast<bmw::result::ErrorCode>(ComErrc::kGrantEnforcementError):
                return "Request was refused by Grant enforcement layer.";
            case static_cast<bmw::result::ErrorCode>(ComErrc::kPeerIsUnreachable):
                return "TLS handshake fail.";
            case static_cast<bmw::result::ErrorCode>(ComErrc::kFieldValueIsNotValid):
                return "Field Value is not valid.";
            case static_cast<bmw::result::ErrorCode>(ComErrc::kSetHandlerNotSet):
                return "SetHandler has not been registered.";
            case static_cast<bmw::result::ErrorCode>(ComErrc::kUnsetFailure):
                return "Failure has been detected by unset operation.";
            case static_cast<bmw::result::ErrorCode>(ComErrc::kSampleAllocationFailure):
                return "Not Sufficient memory resources can be allocated.";
            case static_cast<bmw::result::ErrorCode>(ComErrc::kIllegalUseOfAllocate):
                return "The allocation was illegally done via custom allocator (i.e., not via shared memory "
                       "allocation).";
            case static_cast<bmw::result::ErrorCode>(ComErrc::kServiceNotOffered):
                return "Service not offered.";
            case static_cast<bmw::result::ErrorCode>(ComErrc::kCommunicationLinkError):
                return "Communication link is broken.";
            case static_cast<bmw::result::ErrorCode>(ComErrc::kNoClients):
                return "No clients connected.";
            case static_cast<bmw::result::ErrorCode>(ComErrc::kCommunicationStackError):
                return "Communication Stack Error, e.g. network stack, network binding, or communication framework "
                       "reports an error";
            case static_cast<bmw::result::ErrorCode>(ComErrc::kMaxSampleCountNotRealizable):
                return "Provided maxSampleCount not realizable.";
            case static_cast<bmw::result::ErrorCode>(ComErrc::kMaxSubscribersExceeded):
                return "Subscriber count exceeded";
            case static_cast<bmw::result::ErrorCode>(ComErrc::kWrongMethodCallProcessingMode):
                return "Wrong processing mode passed to constructor method call.";
            case static_cast<bmw::result::ErrorCode>(ComErrc::kErroneousFileHandle):
                return "The FileHandle returned from FindServce is corrupt/service not available.";
            case static_cast<bmw::result::ErrorCode>(ComErrc::kCouldNotExecute):
                return "Command could not be executed in provided Execution Context.";
            case static_cast<bmw::result::ErrorCode>(ComErrc::kInvalidInstanceIdentifierString):
                return "Invalid instance identifier format of string.";
            case static_cast<bmw::result::ErrorCode>(ComErrc::kInvalidBindingInformation):
                return "Internal error: Binding information invalid.";
            case static_cast<bmw::result::ErrorCode>(ComErrc::kEventNotExisting):
                return "Requested event does not exist on sender side.";
            case static_cast<bmw::result::ErrorCode>(ComErrc::kNotSubscribed):
                return "Request invalid: event proxy is not subscribed to the event.";
            case static_cast<bmw::result::ErrorCode>(ComErrc::kInvalidConfiguration):
                return "Invalid configuration.";
            case static_cast<bmw::result::ErrorCode>(ComErrc::kInvalidMetaModelShortname):
                return "Meta model short name does not adhere to naming requirements.";
            case static_cast<bmw::result::ErrorCode>(ComErrc::kServiceInstanceAlreadyOffered):
                return "Service instance is already offered";
            case static_cast<bmw::result::ErrorCode>(ComErrc::kCouldNotRestartProxy):
                return "Could not recreate proxy after previous crash.";
            case static_cast<bmw::result::ErrorCode>(ComErrc::kNotOffered):
                return "Skeleton Event / Field has not been offered yet.";
            case static_cast<bmw::result::ErrorCode>(ComErrc::kInstanceIDCouldNotBeResolved):
                return "Runtime could not resolve a valid InstanceIdentifier from the provided InstanceSpecifier.";
            case static_cast<bmw::result::ErrorCode>(ComErrc::kFindServiceHandlerFailure):
                return "StartFindService failed to register handler.";
            case static_cast<bmw::result::ErrorCode>(ComErrc::kInvalidHandle):
                return "StopFindService was called with invalid FindServiceHandle.";
            default:
                return "unknown future error";
        }
    }
};

bmw::result::Error MakeError(const ComErrc code, const std::string_view message = "");


}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_COMERROR_H
