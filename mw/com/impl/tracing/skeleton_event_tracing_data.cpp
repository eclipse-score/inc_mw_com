// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#include "platform/aas/mw/com/impl/tracing/skeleton_event_tracing_data.h"

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

void DisableAllTracePoints(SkeletonEventTracingData& skeleton_event_tracing_data) noexcept
{
    skeleton_event_tracing_data.enable_send = false;
    skeleton_event_tracing_data.enable_send_with_allocate = false;
}

}  // namespace tracing
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
