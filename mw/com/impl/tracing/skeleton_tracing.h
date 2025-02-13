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


///
/// @file
/// 
///

#ifndef PLATFORM_AAS_MW_COM_IMPL_SKELETON_TRACING_H
#define PLATFORM_AAS_MW_COM_IMPL_SKELETON_TRACING_H

#include "platform/aas/mw/com/impl/instance_identifier.h"
#include "platform/aas/mw/com/impl/skeleton_binding.h"
#include "platform/aas/mw/com/impl/skeleton_event_base.h"
#include "platform/aas/mw/com/impl/skeleton_field_base.h"

#include <amp_optional.hpp>
#include <map>

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

amp::optional<SkeletonBinding::RegisterShmObjectTraceCallback> CreateRegisterShmObjectCallback(
    const InstanceIdentifier& instance_id,
    const std::map<amp::string_view, std::reference_wrapper<SkeletonEventBase>>& events,
    const std::map<amp::string_view, std::reference_wrapper<SkeletonFieldBase>>& fields,
    const SkeletonBinding& skeleton_binding) noexcept;
amp::optional<SkeletonBinding::UnregisterShmObjectTraceCallback> CreateUnregisterShmObjectCallback(
    const InstanceIdentifier& instance_id,
    const std::map<amp::string_view, std::reference_wrapper<SkeletonEventBase>>& events,
    const std::map<amp::string_view, std::reference_wrapper<SkeletonFieldBase>>& fields,
    const SkeletonBinding& skeleton_binding) noexcept;

}  // namespace tracing
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_SKELETON_TRACING_H
