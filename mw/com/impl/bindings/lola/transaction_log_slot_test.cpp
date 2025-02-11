// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#include "platform/aas/mw/com/impl/bindings/lola/transaction_log_slot.h"

#include <gtest/gtest.h>

namespace bmw
{
namespace mw
{
namespace com
{
namespace impl
{
namespace lola
{
namespace
{

TEST(TransactionLogSlotTest, TransactionsByDefaultWillBeFalse)
{
    TransactionLogSlot unit{};

    EXPECT_FALSE(unit.GetTransactionBegin());
    EXPECT_FALSE(unit.GetTransactionEnd());
}

TEST(TransactionLogSlotTest, SettingTransactionBegin)
{
    TransactionLogSlot unit{};

    unit.SetTransactionBegin(true);

    EXPECT_TRUE(unit.GetTransactionBegin());
    EXPECT_FALSE(unit.GetTransactionEnd());

    unit.SetTransactionBegin(false);

    EXPECT_FALSE(unit.GetTransactionBegin());
    EXPECT_FALSE(unit.GetTransactionEnd());
}

TEST(TransactionLogSlotTest, SettingTransactionEnd)
{
    TransactionLogSlot unit{};

    unit.SetTransactionEnd(true);

    EXPECT_FALSE(unit.GetTransactionBegin());
    EXPECT_TRUE(unit.GetTransactionEnd());

    unit.SetTransactionEnd(false);

    EXPECT_FALSE(unit.GetTransactionBegin());
    EXPECT_FALSE(unit.GetTransactionEnd());
}

}  // namespace
}  // namespace lola
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
