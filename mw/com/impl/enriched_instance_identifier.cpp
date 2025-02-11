// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#include "platform/aas/mw/com/impl/enriched_instance_identifier.h"

namespace bmw
{
namespace mw
{
namespace com
{
namespace impl
{

bool operator==(const EnrichedInstanceIdentifier& lhs, const EnrichedInstanceIdentifier& rhs) noexcept
{
    return ((lhs.GetInstanceIdentifier() == rhs.GetInstanceIdentifier()) &&
            (lhs.GetInstanceId() == rhs.GetInstanceId()) && (lhs.GetQualityType() == rhs.GetQualityType()));
}

}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
