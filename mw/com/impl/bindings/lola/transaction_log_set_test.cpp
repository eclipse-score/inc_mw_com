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


#include "platform/aas/mw/com/impl/bindings/lola/transaction_log_set.h"

#include "platform/aas/mw/com/impl/bindings/lola/test/transaction_log_test_resources.h"
#include "platform/aas/mw/com/impl/com_error.h"

#include "platform/aas/lib/memory/shared/shared_memory_resource_heap_allocator_mock.h"

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

using ::testing::_;
using ::testing::MockFunction;
using ::testing::StrictMock;

const std::size_t kNumberOfLogs{5U};
const TransactionLog::MaxSampleCountType kSubscriptionMaxSampleCount{5U};
const std::size_t kDummyNumberOfSlots{5U};
const TransactionLogId kDummyTransactionLogId{10U};

class TransactionLogSetFixture : public TransactionLogSetHelperFixture
{
  protected:
    TransactionLog::DereferenceSlotCallback GetDereferenceSlotCallbackWrapper() noexcept
    {
        // Since a MockFunction doesn't fit within an amp::callback, we wrap it in a smaller lambda which only stores a
        // pointer to the MockFunction and therefore fits within the amp::callback.
        return [this](const TransactionLog::SlotIndexType slot_index) noexcept {
            dereference_slot_callback_.AsStdFunction()(slot_index);
        };
    }

    TransactionLog::UnsubscribeCallback GetUnsubscribeCallbackWrapper() noexcept
    {
        // Since a MockFunction doesn't fit within an amp::callback, we wrap it in a smaller lambda which only stores a
        // pointer to the MockFunction and therefore fits within the amp::callback.
        return [this](const TransactionLog::MaxSampleCountType subscription_max_sample_count) noexcept {
            unsubscribe_callback_.AsStdFunction()(subscription_max_sample_count);
        };
    }

    TransactionLogSet::TransactionLogIndex RegisterProxyElementWithSubscribeTransaction(
        const TransactionLogId& transaction_log_id) noexcept
    {
        const auto transaction_log_index = unit_.RegisterProxyElement(transaction_log_id).value();
        auto& transaction_log = unit_.GetTransactionLog(transaction_log_index);
        transaction_log.SubscribeTransactionBegin(kSubscriptionMaxSampleCount);
        transaction_log.SubscribeTransactionCommit();

        auto& transaction_logs = TransactionLogSetAttorney{unit_}.GetProxyTransactionLogs();
        auto& transaction_log_node = transaction_logs.at(transaction_log_index);

        EXPECT_TRUE(transaction_log_node.IsActive());
        EXPECT_FALSE(transaction_log_node.NeedsRollback());

        return transaction_log_index;
    }

    TransactionLogSet::TransactionLogIndex RegisterProxyElementWithSubscribeAndReferenceTransactions(
        const TransactionLogId& transaction_log_id,
        const TransactionLog::SlotIndexType slot_index) noexcept
    {
        const auto transaction_log_index = unit_.RegisterProxyElement(transaction_log_id).value();
        auto& transaction_log = unit_.GetTransactionLog(transaction_log_index);
        transaction_log.SubscribeTransactionBegin(kSubscriptionMaxSampleCount);
        transaction_log.SubscribeTransactionCommit();
        transaction_log.ReferenceTransactionBegin(slot_index);
        transaction_log.ReferenceTransactionCommit(slot_index);

        auto& transaction_logs = TransactionLogSetAttorney{unit_}.GetProxyTransactionLogs();
        auto& transaction_log_node = transaction_logs.at(transaction_log_index);

        EXPECT_TRUE(transaction_log_node.IsActive());
        EXPECT_FALSE(transaction_log_node.NeedsRollback());

        return transaction_log_index;
    }

    memory::shared::SharedMemoryResourceHeapAllocatorMock memory_resource_{1U};
    TransactionLogSet unit_{kNumberOfLogs, kDummyNumberOfSlots, memory_resource_.getMemoryResourceProxy()};

    StrictMock<MockFunction<void(TransactionLog::SlotIndexType)>> dereference_slot_callback_{};
    StrictMock<MockFunction<void(TransactionLog::MaxSampleCountType)>> unsubscribe_callback_{};
};

using TransactionLogSetRollbackFixture = TransactionLogSetFixture;
TEST_F(TransactionLogSetRollbackFixture, CallingRollbackOnUnregisteredIdIdDoesNothing)
{
    const auto rollback_result = unit_.RollbackProxyTransactions(
        kDummyTransactionLogId, GetDereferenceSlotCallbackWrapper(), GetUnsubscribeCallbackWrapper());
    ASSERT_TRUE(rollback_result.has_value());
}

TEST_F(TransactionLogSetRollbackFixture, CallingRollbackOnRegisteredIdWithoutMarkingNeedsRollbackIdDoesNothing)
{
    const TransactionLog::SlotIndexType slot_index{1U};

    // Expecting that both callbacks will not be called
    EXPECT_CALL(unsubscribe_callback_, Call(kSubscriptionMaxSampleCount)).Times(0);
    EXPECT_CALL(dereference_slot_callback_, Call(slot_index)).Times(0);

    // When registering a TransactionLog
    const auto transaction_log_index = unit_.RegisterProxyElement(kDummyTransactionLogId).value();

    // and MarkTransactionLogsNeedRollback is not called

    // and RollbackProxyTransactions is called
    const auto rollback_result = unit_.RollbackProxyTransactions(
        kDummyTransactionLogId, GetDereferenceSlotCallbackWrapper(), GetUnsubscribeCallbackWrapper());
    ASSERT_TRUE(rollback_result.has_value());

    // Then the TransactionLog still remains
    const bool expect_needs_rollback{false};
    ExpectProxyTransactionLogExistsAtIndex(unit_, kDummyTransactionLogId, transaction_log_index, expect_needs_rollback);
}

TEST_F(TransactionLogSetRollbackFixture, CallingRollbackOnRegisteredIdRollsBackLogAndResetsElement)
{
    const TransactionLog::SlotIndexType slot_index{1U};

    // Expecting that both callbacks will be called once
    EXPECT_CALL(unsubscribe_callback_, Call(kSubscriptionMaxSampleCount));
    EXPECT_CALL(dereference_slot_callback_, Call(slot_index));

    // When registering a TransactionLog with successful transactions
    amp::ignore = RegisterProxyElementWithSubscribeAndReferenceTransactions(kDummyTransactionLogId, slot_index);

    // When MarkTransactionLogsNeedRollback is called
    unit_.MarkTransactionLogsNeedRollback(kDummyTransactionLogId);

    // and RollbackProxyTransactions is called
    const auto rollback_result = unit_.RollbackProxyTransactions(
        kDummyTransactionLogId, GetDereferenceSlotCallbackWrapper(), GetUnsubscribeCallbackWrapper());

    // Then no error should be returned
    ASSERT_TRUE(rollback_result.has_value());

    // And the transaction log should be cleared
    ExpectTransactionLogSetEmpty(unit_);
}

TEST_F(TransactionLogSetRollbackFixture, CallingRollbackOnRegisteredIdRollsBackFirstLogWithProvidedId)
{
    const TransactionLog::SlotIndexType slot_index{1U};

    // Expecting that the unsubscribe callback will be called for both instances
    EXPECT_CALL(unsubscribe_callback_, Call(kSubscriptionMaxSampleCount)).Times(2);

    // and expecting that the dereference callback will only be called for the instance with reference transactions
    // recorded
    EXPECT_CALL(dereference_slot_callback_, Call(slot_index)).Times(1);

    // When registering TransactionLogs with successful transactions
    amp::ignore = RegisterProxyElementWithSubscribeAndReferenceTransactions(kDummyTransactionLogId, slot_index);
    const auto transaction_log_index_2 = RegisterProxyElementWithSubscribeTransaction(kDummyTransactionLogId);

    // When MarkTransactionLogsNeedRollback is called
    unit_.MarkTransactionLogsNeedRollback(kDummyTransactionLogId);

    // and RollbackProxyTransactions is called
    const auto rollback_result = unit_.RollbackProxyTransactions(
        kDummyTransactionLogId, GetDereferenceSlotCallbackWrapper(), GetUnsubscribeCallbackWrapper());

    // Then no error should be returned
    ASSERT_TRUE(rollback_result.has_value());

    // And the only the second transaction log should remain
    const bool expect_needs_rollback{true};
    ExpectProxyTransactionLogExistsAtIndex(
        unit_, kDummyTransactionLogId, transaction_log_index_2, expect_needs_rollback);

    // and when RollbackProxyTransactions is called again
    const auto rollback_result_2 = unit_.RollbackProxyTransactions(
        kDummyTransactionLogId, GetDereferenceSlotCallbackWrapper(), GetUnsubscribeCallbackWrapper());

    // Then no error should be returned
    ASSERT_TRUE(rollback_result_2.has_value());

    // And the transaction log should be cleared
    ExpectTransactionLogSetEmpty(unit_);
}

TEST_F(TransactionLogSetRollbackFixture, CallingRollbackOnRegisteredIdOnlyRollsBackLogsMarkedForRollback)
{
    const TransactionLog::SlotIndexType slot_index{1U};

    // Expecting that the unsubscribe callback will be called once
    EXPECT_CALL(unsubscribe_callback_, Call(kSubscriptionMaxSampleCount));

    // and expecting that the dereference callback will never be called
    EXPECT_CALL(dereference_slot_callback_, Call(slot_index)).Times(0);

    // When registering a TransactionLog with a successful subscribe transaction
    amp::ignore = RegisterProxyElementWithSubscribeTransaction(kDummyTransactionLogId);

    // When MarkTransactionLogsNeedRollback is called
    unit_.MarkTransactionLogsNeedRollback(kDummyTransactionLogId);

    // and then a second TransactionLog is registered with successful transactions
    const auto transaction_log_index_2 =
        RegisterProxyElementWithSubscribeAndReferenceTransactions(kDummyTransactionLogId, slot_index);

    // When RollbackProxyTransactions is called
    const auto rollback_result = unit_.RollbackProxyTransactions(
        kDummyTransactionLogId, GetDereferenceSlotCallbackWrapper(), GetUnsubscribeCallbackWrapper());

    // Then no error should be returned
    ASSERT_TRUE(rollback_result.has_value());

    // And the second transaction log should remain
    const bool expect_needs_rollback{false};
    ExpectProxyTransactionLogExistsAtIndex(
        unit_, kDummyTransactionLogId, transaction_log_index_2, expect_needs_rollback);
}

TEST_F(TransactionLogSetRollbackFixture,
       CallingRollbackOnRegisteredProxyTransactionLogIdPropagatesErrorFromTransactionLog)
{
    const std::size_t slot_index{1U};

    // Expecting that both callbacks are never called
    EXPECT_CALL(unsubscribe_callback_, Call(_)).Times(0);
    EXPECT_CALL(dereference_slot_callback_, Call(slot_index)).Times(0);

    // When registering a TransactionLog
    const auto transaction_log_index = unit_.RegisterProxyElement(kDummyTransactionLogId).value();

    // and a subscribe transaction is begun but never finished, indicating a crash
    auto& transaction_log = unit_.GetTransactionLog(transaction_log_index);
    transaction_log.SubscribeTransactionBegin(kSubscriptionMaxSampleCount);

    // When MarkTransactionLogsNeedRollback is called
    unit_.MarkTransactionLogsNeedRollback(kDummyTransactionLogId);

    // and RollbackProxyTransactions is called
    const auto rollback_result = unit_.RollbackProxyTransactions(
        kDummyTransactionLogId, GetDereferenceSlotCallbackWrapper(), GetUnsubscribeCallbackWrapper());

    // Then an error should be returned
    ASSERT_FALSE(rollback_result.has_value());
    EXPECT_EQ(rollback_result.error(), ComErrc::kCouldNotRestartProxy);

    // And the transaction log should not be cleared
    const bool expect_needs_rollback{true};
    ExpectProxyTransactionLogExistsAtIndex(unit_, kDummyTransactionLogId, transaction_log_index, expect_needs_rollback);
}

TEST_F(TransactionLogSetRollbackFixture, CallingRollbackOnUnregisteredSkeletonTransactionLogIdDoesNothing)
{
    const auto rollback_result = unit_.RollbackSkeletonTracingTransactions(GetDereferenceSlotCallbackWrapper());
    ASSERT_TRUE(rollback_result.has_value());
}

TEST_F(TransactionLogSetRollbackFixture,
       CallingRollbackOnRegisteredSkeletonTransactionLogIdRollsBackLogAndResetsElement)
{
    const std::size_t slot_index{kDummyNumberOfSlots - 1U};

    // Expecting that the dereference callback will be called once
    EXPECT_CALL(dereference_slot_callback_, Call(slot_index));

    // When registering a TransactionLog
    const auto transaction_log_index = unit_.RegisterSkeletonTracingElement();

    auto& transaction_log = unit_.GetTransactionLog(transaction_log_index);

    // and a successful reference transaction is recorded
    transaction_log.ReferenceTransactionBegin(slot_index);
    transaction_log.ReferenceTransactionCommit(slot_index);

    // When RollbackSkeletonTracingTransactions is called
    const auto rollback_result = unit_.RollbackSkeletonTracingTransactions(GetDereferenceSlotCallbackWrapper());

    // Then no error should be returned
    ASSERT_TRUE(rollback_result.has_value());

    // And the transaction log should be cleared
    EXPECT_FALSE(TransactionLogSetAttorney{unit_}.GetSkeletonTransactionLog().has_value());
}

TEST_F(TransactionLogSetRollbackFixture,
       CallingRollbackOnRegisteredSkeletonTransactionLogIdPropagatesErrorFromTransactionLog)
{
    const std::size_t slot_index{1U};

    // Expecting that the dereference callback is never called
    EXPECT_CALL(dereference_slot_callback_, Call(slot_index)).Times(0);

    // When registering a TransactionLog
    const auto transaction_log_index = unit_.RegisterSkeletonTracingElement();

    // and a reference transaction is begun but never finished, indicating a crash
    auto& transaction_log = unit_.GetTransactionLog(transaction_log_index);
    transaction_log.ReferenceTransactionBegin(slot_index);

    // When RollbackProxyTransactions is called
    const auto rollback_result = unit_.RollbackSkeletonTracingTransactions(GetDereferenceSlotCallbackWrapper());

    // Then an error should be returned
    ASSERT_FALSE(rollback_result.has_value());
    EXPECT_EQ(rollback_result.error(), ComErrc::kCouldNotRestartProxy);

    // And the transaction log should not be cleared
    EXPECT_TRUE(TransactionLogSetAttorney{unit_}.GetSkeletonTransactionLog().has_value());
}

using TransactionLogSetRegisterFixture = TransactionLogSetFixture;
TEST_F(TransactionLogSetRegisterFixture, RegisteringLessThanTheMaxNumberPassedToConstructorReturnsValidIndexes)
{
    for (std::size_t i = 0; i < kNumberOfLogs; ++i)
    {
        const TransactionLogId transaction_log_id{static_cast<uid_t>(i)};
        const auto transaction_log_index_result = unit_.RegisterProxyElement(transaction_log_id);
        EXPECT_TRUE(transaction_log_index_result.has_value());
    }
}

TEST_F(TransactionLogSetRegisterFixture, RegisteringMoreThanTheMaxNumberPassedToConstructorReturnsError)
{
    for (std::size_t i = 0; i < kNumberOfLogs; ++i)
    {
        const TransactionLogId transaction_log_id{static_cast<uid_t>(i)};
        const auto transaction_log_index_result = unit_.RegisterProxyElement(transaction_log_id);
        EXPECT_TRUE(transaction_log_index_result.has_value());
    }

    const TransactionLogId transaction_log_id{static_cast<uid_t>(kNumberOfLogs)};
    const auto transaction_log_index_result = unit_.RegisterProxyElement(transaction_log_id);
    EXPECT_FALSE(transaction_log_index_result.has_value());
}

TEST_F(TransactionLogSetRegisterFixture, CallingRegisterProxyWillCreateATransactionLogAndReturnTheIndex)
{
    const bool expect_needs_rollback{false};

    const auto transaction_log_index = unit_.RegisterProxyElement(kDummyTransactionLogId).value();
    ExpectProxyTransactionLogExistsAtIndex(unit_, kDummyTransactionLogId, transaction_log_index, expect_needs_rollback);
}

TEST_F(TransactionLogSetRegisterFixture, RegisterWithSameIdCanBeReCalledAfterUnregistering)
{
    const bool expect_needs_rollback{false};

    const auto transaction_log_index = unit_.RegisterProxyElement(kDummyTransactionLogId).value();
    unit_.Unregister(transaction_log_index);
    const auto transaction_log_index_2 = unit_.RegisterProxyElement(kDummyTransactionLogId).value();
    ExpectProxyTransactionLogExistsAtIndex(
        unit_, kDummyTransactionLogId, transaction_log_index_2, expect_needs_rollback);
}

TEST_F(TransactionLogSetRegisterFixture, CallingRegisterWithSameIdWillReturnDifferentIndices)
{
    const bool expect_needs_rollback{false};

    const auto transaction_log_index = unit_.RegisterProxyElement(kDummyTransactionLogId).value();
    ExpectProxyTransactionLogExistsAtIndex(unit_, kDummyTransactionLogId, transaction_log_index, expect_needs_rollback);

    const auto transaction_log_index_2 = unit_.RegisterProxyElement(kDummyTransactionLogId).value();
    EXPECT_NE(transaction_log_index, transaction_log_index_2);

    const bool expect_other_slots_empty{false};
    ExpectProxyTransactionLogExistsAtIndex(
        unit_, kDummyTransactionLogId, transaction_log_index_2, expect_needs_rollback, expect_other_slots_empty);
}

TEST_F(TransactionLogSetRegisterFixture, CallingRegisterSkeletonWillCreateATransactionLogAndReturnTheIndex)
{
    const auto transaction_log_index = unit_.RegisterSkeletonTracingElement();
    EXPECT_EQ(transaction_log_index, TransactionLogSet::kSkeletonIndexSentinel);
    EXPECT_TRUE(TransactionLogSetAttorney{unit_}.GetSkeletonTransactionLog().has_value());
}

TEST_F(TransactionLogSetRegisterFixture, CallingUnRegisterWillRemoveProxyTransactionLog)
{
    const auto transaction_log_index = unit_.RegisterProxyElement(kDummyTransactionLogId).value();
    unit_.Unregister(transaction_log_index);
    ExpectTransactionLogSetEmpty(unit_);
}

TEST_F(TransactionLogSetRegisterFixture, CallingUnRegisterAfterRegisteringTwiceWithSameIdOnlyUnregistersOneAtATime)
{
    const bool expect_needs_rollback{false};

    const auto transaction_log_index = unit_.RegisterProxyElement(kDummyTransactionLogId).value();
    const auto transaction_log_index_2 = unit_.RegisterProxyElement(kDummyTransactionLogId).value();

    unit_.Unregister(transaction_log_index);
    ExpectProxyTransactionLogExistsAtIndex(
        unit_, kDummyTransactionLogId, transaction_log_index_2, expect_needs_rollback);

    unit_.Unregister(transaction_log_index_2);
    ExpectTransactionLogSetEmpty(unit_);
}

TEST_F(TransactionLogSetRegisterFixture, CallingUnRegisterWillRemoveSkeletonTransactionLog)
{
    const auto transaction_log_index = unit_.RegisterSkeletonTracingElement();
    unit_.Unregister(transaction_log_index);
    EXPECT_FALSE(TransactionLogSetAttorney{unit_}.GetSkeletonTransactionLog().has_value());
}

}  // namespace
}  // namespace lola
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
