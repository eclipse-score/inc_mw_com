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


#ifndef PLATFORM_AAS_MW_COM_IMPL_TRACING_CONFIGURATION_TRACE_CONFIG_ERROR_H
#define PLATFORM_AAS_MW_COM_IMPL_TRACING_CONFIGURATION_TRACE_CONFIG_ERROR_H

#include "platform/aas/lib/result/result.h"

namespace bmw
{
namespace mw
{
namespace com
{
namespace impl
{
namespace tracing
{

/// \brief error codes, which can occur, when trying to parse a tracing filter config json and creating a
///        TracingFilterConfig from it.
enum class TraceErrorCode : bmw::result::ErrorCode
{
    JsonConfigParseError = 1,
    TraceErrorDisableAllTracePoints = 2,
    TraceErrorDisableTracePointInstance = 3
};

/// \brief See above explanation in TraceErrorCode
class TraceErrorDomain final : public bmw::result::ErrorDomain
{
  public:
    /* Gcc compiler bug leads to a compiler warning if override is not added, even if final keyword is there. */
    /// \todo Gcc compiler bug leads to a compiler warning if override is not added, even if final keyword is there
    /// (https://gcc.gnu.org/bugzilla/show_bug.cgi?id=78010). When bug is fixed, remove the override keyword from the
    /// MessageFor function signature and the AUTOSAR.MEMB.VIRTUAL.SPEC klocwork suppression.
    amp::string_view MessageFor(const bmw::result::ErrorCode& errorCode) const noexcept override final
    {
        const auto code = static_cast<TraceErrorCode>(errorCode);
        switch (code)
        {
            case TraceErrorCode::JsonConfigParseError:
                return "json config parsing error";
            case TraceErrorCode::TraceErrorDisableAllTracePoints:
                return "Tracing is completely disabled because of unrecoverable error";
            case TraceErrorCode::TraceErrorDisableTracePointInstance:
                return "Tracing for the given trace-point instance is disabled because of unrecoverable error";
            default:
                return "unknown trace error";
        }
    }
};

bmw::result::Error MakeError(const TraceErrorCode code, const std::string_view message = "");

}  // namespace tracing
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_TRACING_CONFIGURATION_TRACE_CONFIG_ERROR_H
