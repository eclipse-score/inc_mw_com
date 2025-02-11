// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#include "platform/aas/mw/com/impl/find_service_handle.h"

#include <gtest/gtest.h>

namespace bmw
{
namespace mw
{
namespace com
{
namespace impl
{
namespace
{

TEST(FindServiceHandle, CanBeCopiedAndEqualCompared)
{
    RecordProperty("Verifies", "2");
    RecordProperty("Description", "Checks CopyAssignment operator and EqualComparableOperator");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("Priority", "1");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    const auto unit = make_FindServiceHandle(1U);
    const auto unitCopy = unit;

    ASSERT_EQ(unit, unitCopy);
}

TEST(FindServiceHandle, LessCompareable)
{
    RecordProperty("Verifies", "2");
    RecordProperty("Description", "Checks LessComparableOperator");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("Priority", "1");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    const auto unit = make_FindServiceHandle(2U);
    const auto less = make_FindServiceHandle(1U);

    ASSERT_LT(less, unit);
}

TEST(FindServiceHandleView, GetUid)
{
    const auto unit = make_FindServiceHandle(42U);

    ASSERT_EQ(FindServiceHandleView{unit}.getUid(), 42U);
}

}  // namespace
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
