// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#include "platform/aas/mw/com/impl/tracing/configuration/proxy_field_trace_point_type.h"

#include <gtest/gtest.h>

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
namespace
{

TEST(ProxyFieldTracePointTypeTest, DefaultConstructedEnumValueIsInvalid)
{
    // Given a default constructed ProxyFieldTracePointType
    ProxyFieldTracePointType proxy_field_trace_point_type{};

    // Then the value of the enum should be invalid
    EXPECT_EQ(proxy_field_trace_point_type, ProxyFieldTracePointType::INVALID);
}

}  // namespace
}  // namespace tracing
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
