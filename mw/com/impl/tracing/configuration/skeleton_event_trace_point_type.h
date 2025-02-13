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


#ifndef PLATFORM_AAS_MW_COM_IMPL_TRACING_CONFIGURATION_SKELETON_EVENT_TRACE_POINT_TYPE_H
#define PLATFORM_AAS_MW_COM_IMPL_TRACING_CONFIGURATION_SKELETON_EVENT_TRACE_POINT_TYPE_H

#include <cstdint>

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

enum class SkeletonEventTracePointType : std::uint8_t
{
    INVALID = 0U,
    SEND,
    SEND_WITH_ALLOCATE,
};

}  // namespace tracing
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_TRACING_CONFIGURATION_SKELETON_EVENT_TRACE_POINT_TYPE_H
