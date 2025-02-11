// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#include "platform/aas/mw/com/impl/util/copyable_atomic.h"

#include <gtest/gtest.h>

namespace bmw::mw::com::impl
{

namespace
{
TEST(CopyableAtomicTest, Construction)
{
    bool value{true};

    auto unit = CopyableAtomic<bool>(value);
    EXPECT_EQ(unit, value);
}

TEST(CopyableAtomicTest, CopyConstruct)
{
    auto unit = CopyableAtomic<bool>(true);
    CopyableAtomic<bool> unit2{unit};
    EXPECT_EQ(unit2, true);
}

TEST(CopyableAtomicTest, CopyAssign)
{
    auto unit = CopyableAtomic<bool>(true);
    auto unit2 = CopyableAtomic<bool>(false);
    EXPECT_EQ(unit2, false);
    unit2 = unit;
    EXPECT_EQ(unit2, true);
}

}  // namespace

}  // namespace bmw::mw::com::impl
