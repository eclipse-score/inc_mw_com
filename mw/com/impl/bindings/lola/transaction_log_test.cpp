// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#include "platform/aas/mw/com/impl/bindings/lola/transaction_log.h"

#include "platform/aas/lib/memory/shared/shared_memory_resource_heap_allocator_mock.h"

#include <gmock/gmock.h>
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

const std::size_t kNumberOfSlots{5U};
const TransactionLog::MaxSampleCountType kSubscriptionMaxSampleCount{5U};

class TransactionLogFixture : public ::testing::Test
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

    memory::shared::SharedMemoryResourceHeapAllocatorMock memory_resource_{1U};
    TransactionLog unit_{kNumberOfSlots, memory_resource_.getMemoryResourceProxy()};

    StrictMock<MockFunction<void(TransactionLog::SlotIndexType)>> dereference_slot_callback_{};
    StrictMock<MockFunction<void(TransactionLog::MaxSampleCountType)>> unsubscribe_callback_{};
};

using TransactionLogProxyElementFixture = TransactionLogFixture;
TEST_F(TransactionLogProxyElementFixture, RollbackWillNotCallCallbackWhenNoTransactionsRecorded)
{
    // Given a valid TransactionLog

    // Expecting that neither callback will be called
    EXPECT_CALL(dereference_slot_callback_, Call(_)).Times(0);
    EXPECT_CALL(unsubscribe_callback_, Call(_)).Times(0);

    // When no transactions are recorded

    // and when rollback is called
    const auto rollback_result =
        unit_.RollbackProxyElementLog(GetDereferenceSlotCallbackWrapper(), GetUnsubscribeCallbackWrapper());

    // Then the result will not contain an error
    EXPECT_TRUE(rollback_result.has_value());
}

TEST_F(TransactionLogProxyElementFixture, RollbackWillNotCallCallbackWhenOnlySubscribeAndUnsubscribeRecorded)
{
    // Given a valid TransactionLog

    // Expecting that neither callback will be called
    EXPECT_CALL(dereference_slot_callback_, Call(_)).Times(0);
    EXPECT_CALL(unsubscribe_callback_, Call(_)).Times(0);

    // When Subscribe is recorded
    unit_.SubscribeTransactionBegin(kSubscriptionMaxSampleCount);
    unit_.SubscribeTransactionCommit();

    // and no transactions are recorded

    // and then Unsubscribe is recorded
    unit_.UnsubscribeTransactionBegin();
    unit_.UnsubscribeTransactionCommit();

    // and when rollback is called
    const auto rollback_result =
        unit_.RollbackProxyElementLog(GetDereferenceSlotCallbackWrapper(), GetUnsubscribeCallbackWrapper());

    // Then the result will not contain an error
    EXPECT_TRUE(rollback_result.has_value());
}

TEST_F(TransactionLogProxyElementFixture, RollbackWillNotCallCallbackAfterDereferencingAndUnsubscribingCompleted)
{
    const std::size_t slot_index_0{0U};
    const std::size_t slot_index_1{1U};

    // Given a valid TransactionLog

    // Expecting that neither callback will be called
    EXPECT_CALL(dereference_slot_callback_, Call(_)).Times(0);
    EXPECT_CALL(unsubscribe_callback_, Call(_)).Times(0);

    // When Subscribe is recorded
    unit_.SubscribeTransactionBegin(kSubscriptionMaxSampleCount);
    unit_.SubscribeTransactionCommit();

    // and 2 slots are succesfully referenced and dereferenced
    unit_.ReferenceTransactionBegin(slot_index_0);
    unit_.ReferenceTransactionCommit(slot_index_0);
    unit_.ReferenceTransactionBegin(slot_index_1);
    unit_.ReferenceTransactionCommit(slot_index_1);

    unit_.DereferenceTransactionBegin(slot_index_0);
    unit_.DereferenceTransactionCommit(slot_index_0);
    unit_.DereferenceTransactionBegin(slot_index_1);
    unit_.DereferenceTransactionCommit(slot_index_1);

    // and then Unsubscribe is recorded
    unit_.UnsubscribeTransactionBegin();
    unit_.UnsubscribeTransactionCommit();

    // and when rollback is called
    const auto rollback_result =
        unit_.RollbackProxyElementLog(GetDereferenceSlotCallbackWrapper(), GetUnsubscribeCallbackWrapper());

    // Then the result will not contain an error
    EXPECT_TRUE(rollback_result.has_value());
}

TEST_F(TransactionLogProxyElementFixture, RollbackWillNotCallCallbackIfReferencingAborted)
{
    const std::size_t slot_index_0{0U};
    const std::size_t slot_index_1{1U};

    // Given a valid TransactionLog

    // Expecting that neither callback will be called
    EXPECT_CALL(dereference_slot_callback_, Call(_)).Times(0);
    EXPECT_CALL(unsubscribe_callback_, Call(_)).Times(0);

    // When Subscribe is recorded
    unit_.SubscribeTransactionBegin(kSubscriptionMaxSampleCount);
    unit_.SubscribeTransactionCommit();

    // and 2 slots are referenced and aborted
    unit_.ReferenceTransactionBegin(slot_index_0);
    unit_.ReferenceTransactionAbort(slot_index_0);
    unit_.ReferenceTransactionBegin(slot_index_1);
    unit_.ReferenceTransactionAbort(slot_index_1);

    // and then Unsubscribe is recorded
    unit_.UnsubscribeTransactionBegin();
    unit_.UnsubscribeTransactionCommit();

    // and when rollback is called
    const auto rollback_result =
        unit_.RollbackProxyElementLog(GetDereferenceSlotCallbackWrapper(), GetUnsubscribeCallbackWrapper());

    // Then the result will not contain an error
    EXPECT_TRUE(rollback_result.has_value());
}

TEST_F(TransactionLogProxyElementFixture, RollbackWillNotCallCallbackIfSubscribeAborted)
{
    // Given a valid TransactionLog

    // Expecting that neither callback will be called
    EXPECT_CALL(dereference_slot_callback_, Call(_)).Times(0);
    EXPECT_CALL(unsubscribe_callback_, Call(_)).Times(0);

    // When Subscribe is recorded but then aborted
    unit_.SubscribeTransactionBegin(kSubscriptionMaxSampleCount);
    unit_.SubscribeTransactionAbort();

    // and when rollback is called
    const auto rollback_result =
        unit_.RollbackProxyElementLog(GetDereferenceSlotCallbackWrapper(), GetUnsubscribeCallbackWrapper());

    // Then the result will not contain an error
    EXPECT_TRUE(rollback_result.has_value());
}

TEST_F(TransactionLogProxyElementFixture, RollbackWillCallBothCallbacksAfterReferencingCompleted)
{
    const std::size_t slot_index_0{0U};
    const std::size_t slot_index_1{1U};

    // Given a valid TransactionLog

    // Expecting that the decrement slot callback will be called once for 2 slots
    EXPECT_CALL(dereference_slot_callback_, Call(slot_index_0));
    EXPECT_CALL(dereference_slot_callback_, Call(slot_index_1));

    // and the unsubscribe callback will be called
    EXPECT_CALL(unsubscribe_callback_, Call(kSubscriptionMaxSampleCount));

    // When Subscribe is recorded
    unit_.SubscribeTransactionBegin(kSubscriptionMaxSampleCount);
    unit_.SubscribeTransactionCommit();

    // and both slots are referenced but never dereferenced
    unit_.ReferenceTransactionBegin(slot_index_0);
    unit_.ReferenceTransactionCommit(slot_index_0);

    unit_.ReferenceTransactionBegin(slot_index_1);
    unit_.ReferenceTransactionCommit(slot_index_1);

    // and when rollback is called
    const auto rollback_result =
        unit_.RollbackProxyElementLog(GetDereferenceSlotCallbackWrapper(), GetUnsubscribeCallbackWrapper());

    // Then the result will not contain an error
    EXPECT_TRUE(rollback_result.has_value());

    // and when rollback is called again, then the slots should have already been derefenced so it should do nothing
    const auto rollback_result_2 =
        unit_.RollbackProxyElementLog(GetDereferenceSlotCallbackWrapper(), GetUnsubscribeCallbackWrapper());
    EXPECT_TRUE(rollback_result_2.has_value());
}

TEST_F(TransactionLogProxyElementFixture, RollbackWillCallUnsubscribeCallbackAfterDereferencingButNotUnsubscribing)
{
    const std::size_t slot_index_0{0U};
    const std::size_t slot_index_1{1U};

    // Given a valid TransactionLog

    // Expecting that the decrement slot callback will never be called
    EXPECT_CALL(dereference_slot_callback_, Call(_)).Times(0);

    // But the unsubscribe callback will be called
    EXPECT_CALL(unsubscribe_callback_, Call(kSubscriptionMaxSampleCount));

    // When Subscribe is recorded
    unit_.SubscribeTransactionBegin(kSubscriptionMaxSampleCount);
    unit_.SubscribeTransactionCommit();

    // and 2 slots are succesfully referenced and dereferenced
    unit_.ReferenceTransactionBegin(slot_index_0);
    unit_.ReferenceTransactionCommit(slot_index_0);
    unit_.ReferenceTransactionBegin(slot_index_1);
    unit_.ReferenceTransactionCommit(slot_index_1);

    unit_.DereferenceTransactionBegin(slot_index_0);
    unit_.DereferenceTransactionCommit(slot_index_0);
    unit_.DereferenceTransactionBegin(slot_index_1);
    unit_.DereferenceTransactionCommit(slot_index_1);

    // and when rollback is called
    const auto rollback_result =
        unit_.RollbackProxyElementLog(GetDereferenceSlotCallbackWrapper(), GetUnsubscribeCallbackWrapper());

    // Then the result will not contain an error
    EXPECT_TRUE(rollback_result.has_value());
}

TEST_F(TransactionLogProxyElementFixture, RollbackWillCallUnsubscribeCallbackWithMostRecentSubscriptionMaxSampleCount)
{
    const std::size_t first_subscription_max_sample_count{5U};
    const std::size_t second_subscription_max_sample_count{10U};

    // Given a valid TransactionLog

    // Expecting that the decrement slot callback will never be called
    EXPECT_CALL(dereference_slot_callback_, Call(_)).Times(0);

    // But the unsubscribe callback will be called with the max sample count from the second subscription
    EXPECT_CALL(unsubscribe_callback_, Call(second_subscription_max_sample_count));

    // When Subscribe is recorded
    unit_.SubscribeTransactionBegin(first_subscription_max_sample_count);
    unit_.SubscribeTransactionCommit();

    // and then Unsubscribe is recorded
    unit_.UnsubscribeTransactionBegin();
    unit_.UnsubscribeTransactionCommit();

    // and a second Subscribe is recorded with a different max sample count
    unit_.SubscribeTransactionBegin(second_subscription_max_sample_count);
    unit_.SubscribeTransactionCommit();

    // and when rollback is called
    const auto rollback_result =
        unit_.RollbackProxyElementLog(GetDereferenceSlotCallbackWrapper(), GetUnsubscribeCallbackWrapper());

    // Then the result will not contain an error
    EXPECT_TRUE(rollback_result.has_value());
}

TEST_F(TransactionLogProxyElementFixture, RollbackWillReturnErrorIfReferenceTransactionDidNotComplete)
{
    const std::size_t slot_index_0{0U};
    const std::size_t slot_index_1{1U};

    // Given a valid TransactionLog

    // Expecting that neither callback will be called
    EXPECT_CALL(dereference_slot_callback_, Call(_)).Times(0);
    EXPECT_CALL(unsubscribe_callback_, Call(_)).Times(0);

    // When Subscribe is recorded
    unit_.SubscribeTransactionBegin(kSubscriptionMaxSampleCount);
    unit_.SubscribeTransactionCommit();

    // and both slots are referenced but the referencing never finished
    unit_.ReferenceTransactionBegin(slot_index_0);

    unit_.ReferenceTransactionBegin(slot_index_1);

    // and when rollback is called
    const auto rollback_result =
        unit_.RollbackProxyElementLog(GetDereferenceSlotCallbackWrapper(), GetUnsubscribeCallbackWrapper());

    // Then the result will contain an error
    EXPECT_FALSE(rollback_result.has_value());

    // and when rollback is called again, then an error should still be returned
    const auto rollback_result_2 =
        unit_.RollbackProxyElementLog(GetDereferenceSlotCallbackWrapper(), GetUnsubscribeCallbackWrapper());
    EXPECT_FALSE(rollback_result_2.has_value());
}

TEST_F(TransactionLogProxyElementFixture, RollbackWillReturnErrorIfDereferenceTransactionDidNotComplete)
{
    const std::size_t slot_index_0{0U};
    const std::size_t slot_index_1{1U};

    // Given a valid TransactionLog

    // Expecting that neither callback will be called
    EXPECT_CALL(dereference_slot_callback_, Call(_)).Times(0);
    EXPECT_CALL(unsubscribe_callback_, Call(_)).Times(0);

    // When Subscribe is recorded
    unit_.SubscribeTransactionBegin(kSubscriptionMaxSampleCount);
    unit_.SubscribeTransactionCommit();

    // and 2 slots are succesfully referenced
    unit_.ReferenceTransactionBegin(slot_index_0);
    unit_.ReferenceTransactionCommit(slot_index_0);
    unit_.ReferenceTransactionBegin(slot_index_1);
    unit_.ReferenceTransactionCommit(slot_index_1);

    // but dereferencing both slots never finished
    unit_.DereferenceTransactionBegin(slot_index_0);
    unit_.DereferenceTransactionBegin(slot_index_1);

    // and when rollback is called
    const auto rollback_result =
        unit_.RollbackProxyElementLog(GetDereferenceSlotCallbackWrapper(), GetUnsubscribeCallbackWrapper());

    // Then the result will contain an error
    EXPECT_FALSE(rollback_result.has_value());

    // and when rollback is called again, then an error should still be returned
    const auto rollback_result_2 =
        unit_.RollbackProxyElementLog(GetDereferenceSlotCallbackWrapper(), GetUnsubscribeCallbackWrapper());
    EXPECT_FALSE(rollback_result_2.has_value());
}

TEST_F(TransactionLogProxyElementFixture, RollbackWillReturnErrorIfSubscribeTransactionDidNotComplete)
{
    const std::size_t slot_index_0{0U};
    const std::size_t slot_index_1{1U};

    // Given a valid TransactionLog

    // Expecting that the decrement slot callback will be called once for 2 slots
    EXPECT_CALL(dereference_slot_callback_, Call(slot_index_0));
    EXPECT_CALL(dereference_slot_callback_, Call(slot_index_1));

    // Expecting that the unsubscribe callback will never be called
    EXPECT_CALL(unsubscribe_callback_, Call(_)).Times(0);

    // When subscribe is begun but never finished
    unit_.SubscribeTransactionBegin(kSubscriptionMaxSampleCount);

    // and 2 slots are succesfully referenced
    unit_.ReferenceTransactionBegin(slot_index_0);
    unit_.ReferenceTransactionCommit(slot_index_0);
    unit_.ReferenceTransactionBegin(slot_index_1);
    unit_.ReferenceTransactionCommit(slot_index_1);

    // and when rollback is called
    const auto rollback_result =
        unit_.RollbackProxyElementLog(GetDereferenceSlotCallbackWrapper(), GetUnsubscribeCallbackWrapper());

    // Then the result will contain an error
    EXPECT_FALSE(rollback_result.has_value());

    // and when rollback is called again, then an error should still be returned
    const auto rollback_result_2 =
        unit_.RollbackProxyElementLog(GetDereferenceSlotCallbackWrapper(), GetUnsubscribeCallbackWrapper());
    EXPECT_FALSE(rollback_result_2.has_value());
}

TEST_F(TransactionLogProxyElementFixture, RollbackWillReturnErrorIfUnsubscribeTransactionDidNotComplete)
{
    const std::size_t slot_index_0{0U};
    const std::size_t slot_index_1{1U};

    // Given a valid TransactionLog

    // Expecting that the decrement slot callback will be called once for 2 slots
    EXPECT_CALL(dereference_slot_callback_, Call(slot_index_0));
    EXPECT_CALL(dereference_slot_callback_, Call(slot_index_1));

    // Expecting that the unsubscribe callback will never be called
    EXPECT_CALL(unsubscribe_callback_, Call(_)).Times(0);

    // When subscribe is called
    unit_.SubscribeTransactionBegin(kSubscriptionMaxSampleCount);
    unit_.SubscribeTransactionCommit();

    // and 2 slots are succesfully referenced
    unit_.ReferenceTransactionBegin(slot_index_0);
    unit_.ReferenceTransactionCommit(slot_index_0);
    unit_.ReferenceTransactionBegin(slot_index_1);
    unit_.ReferenceTransactionCommit(slot_index_1);

    // but Unsubscribe is begun but never finished
    unit_.UnsubscribeTransactionBegin();

    // and when rollback is called
    const auto rollback_result =
        unit_.RollbackProxyElementLog(GetDereferenceSlotCallbackWrapper(), GetUnsubscribeCallbackWrapper());

    // Then the result will contain an error
    EXPECT_FALSE(rollback_result.has_value());

    // and when rollback is called again, then an error should still be returned
    const auto rollback_result_2 =
        unit_.RollbackProxyElementLog(GetDereferenceSlotCallbackWrapper(), GetUnsubscribeCallbackWrapper());
    EXPECT_FALSE(rollback_result_2.has_value());
}

using TransactionLogSkeletonTracingElementFixture = TransactionLogFixture;
TEST_F(TransactionLogSkeletonTracingElementFixture, RollbackWillNotCallCallbackWhenNoTransactionsRecorded)
{
    // Given a valid TransactionLog

    // Expecting that the callback will not be called
    EXPECT_CALL(dereference_slot_callback_, Call(_)).Times(0);

    // When no transactions are recorded

    // and when rollback is called
    const auto rollback_result = unit_.RollbackSkeletonTracingElementLog(GetDereferenceSlotCallbackWrapper());

    // Then the result will not contain an error
    EXPECT_TRUE(rollback_result.has_value());
}

TEST_F(TransactionLogSkeletonTracingElementFixture, RollbackWillNotCallCallbackAfterDereferencingCompleted)
{
    const std::size_t slot_index_0{0U};
    const std::size_t slot_index_1{1U};

    // Given a valid TransactionLog

    // Expecting that the callback will not be called
    EXPECT_CALL(dereference_slot_callback_, Call(_)).Times(0);

    // When 2 slots are succesfully referenced and dereferenced
    unit_.ReferenceTransactionBegin(slot_index_0);
    unit_.ReferenceTransactionCommit(slot_index_0);
    unit_.ReferenceTransactionBegin(slot_index_1);
    unit_.ReferenceTransactionCommit(slot_index_1);

    unit_.DereferenceTransactionBegin(slot_index_0);
    unit_.DereferenceTransactionCommit(slot_index_0);
    unit_.DereferenceTransactionBegin(slot_index_1);
    unit_.DereferenceTransactionCommit(slot_index_1);

    // and when rollback is called
    const auto rollback_result = unit_.RollbackSkeletonTracingElementLog(GetDereferenceSlotCallbackWrapper());

    // Then the result will not contain an error
    EXPECT_TRUE(rollback_result.has_value());
}

TEST_F(TransactionLogSkeletonTracingElementFixture, RollbackWillNotCallCallbackIfRerencingAborted)
{
    const std::size_t slot_index_0{0U};
    const std::size_t slot_index_1{1U};

    // Given a valid TransactionLog

    // Expecting that the callback will not be called
    EXPECT_CALL(dereference_slot_callback_, Call(_)).Times(0);

    // When 2 slots are referenced and aborted
    unit_.ReferenceTransactionBegin(slot_index_0);
    unit_.ReferenceTransactionAbort(slot_index_0);
    unit_.ReferenceTransactionBegin(slot_index_1);
    unit_.ReferenceTransactionAbort(slot_index_1);

    // and when rollback is called
    const auto rollback_result = unit_.RollbackSkeletonTracingElementLog(GetDereferenceSlotCallbackWrapper());

    // Then the result will not contain an error
    EXPECT_TRUE(rollback_result.has_value());
}

TEST_F(TransactionLogSkeletonTracingElementFixture, RollbackWillCallCallbackAfterReferencingCompleted)
{
    const std::size_t slot_index_0{0U};
    const std::size_t slot_index_1{1U};

    // Given a valid TransactionLog

    // Expecting that the decrement slot callback will be called once for 2 slots
    EXPECT_CALL(dereference_slot_callback_, Call(slot_index_0));
    EXPECT_CALL(dereference_slot_callback_, Call(slot_index_1));

    // When 2 slots are referenced but not dereferenced
    unit_.ReferenceTransactionBegin(slot_index_0);
    unit_.ReferenceTransactionCommit(slot_index_0);
    unit_.ReferenceTransactionBegin(slot_index_1);
    unit_.ReferenceTransactionCommit(slot_index_1);

    // and when rollback is called
    const auto rollback_result = unit_.RollbackSkeletonTracingElementLog(GetDereferenceSlotCallbackWrapper());

    // Then the result will not contain an error
    EXPECT_TRUE(rollback_result.has_value());
}

TEST_F(TransactionLogSkeletonTracingElementFixture, RollbackWillReturnErrorIfTransactionDidNotComplete)
{
    const std::size_t slot_index_0{0U};
    const std::size_t slot_index_1{1U};

    // Given a valid TransactionLog

    // Expecting that the callback will be called for the slot which completed its transaction
    EXPECT_CALL(dereference_slot_callback_, Call(slot_index_0));

    // When 2 slots are referenced but doesn't finish for one slot
    unit_.ReferenceTransactionBegin(slot_index_0);
    unit_.ReferenceTransactionCommit(slot_index_0);
    unit_.ReferenceTransactionBegin(slot_index_1);

    // and when rollback is called
    const auto rollback_result = unit_.RollbackSkeletonTracingElementLog(GetDereferenceSlotCallbackWrapper());

    // Then the result will contain an error
    EXPECT_FALSE(rollback_result.has_value());

    // and when rollback is called again, then an error should still be returned
    const auto rollback_result_2 = unit_.RollbackSkeletonTracingElementLog(GetDereferenceSlotCallbackWrapper());
    EXPECT_FALSE(rollback_result_2.has_value());
}

TEST_F(TransactionLogSkeletonTracingElementFixture, RollbackWillReturnErrorIfDereferenceTransactionDidNotComplete)
{
    const std::size_t slot_index_0{0U};
    const std::size_t slot_index_1{1U};

    // Given a valid TransactionLog

    // Expecting that the callback will never be called (as the first slot succesfully dereferences its slot and the
    // second slot fails during rollback)
    EXPECT_CALL(dereference_slot_callback_, Call(_)).Times(0);

    // When 2 slots are referenced
    unit_.ReferenceTransactionBegin(slot_index_0);
    unit_.ReferenceTransactionCommit(slot_index_0);
    unit_.ReferenceTransactionBegin(slot_index_1);
    unit_.ReferenceTransactionCommit(slot_index_1);

    // but dereferencing for one slot never finished
    unit_.DereferenceTransactionBegin(slot_index_0);
    unit_.DereferenceTransactionCommit(slot_index_0);
    unit_.DereferenceTransactionBegin(slot_index_1);

    // and when rollback is called
    const auto rollback_result = unit_.RollbackSkeletonTracingElementLog(GetDereferenceSlotCallbackWrapper());

    // Then the result will contain an error
    EXPECT_FALSE(rollback_result.has_value());

    // and when rollback is called again, then an error should still be returned
    const auto rollback_result_2 = unit_.RollbackSkeletonTracingElementLog(GetDereferenceSlotCallbackWrapper());
    EXPECT_FALSE(rollback_result_2.has_value());
}

}  // namespace
}  // namespace lola
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
