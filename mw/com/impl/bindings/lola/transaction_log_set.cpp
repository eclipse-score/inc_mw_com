// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#include "platform/aas/mw/com/impl/bindings/lola/transaction_log_set.h"

#include "platform/aas/lib/result/result.h"
#include "platform/aas/mw/com/impl/com_error.h"

#include <amp_assert.hpp>

#include <mutex>

namespace bmw::mw::com::impl::lola
{

namespace
{

std::vector<TransactionLogSet::TransactionLogIndex> FindTransactionLogIndicesToBeRolledBack(
    const TransactionLogSet::TransactionLogCollection& transaction_logs,
    const TransactionLogId& target_transaction_log_id) noexcept
{
    std::vector<TransactionLogSet::TransactionLogIndex> found_indices{};
    for (TransactionLogSet::TransactionLogIndex i = 0; i < transaction_logs.size(); ++i)
    {
        const auto& transaction_log_node = transaction_logs.at(i);
        const bool is_active = transaction_log_node.IsActive();
        const bool has_matching_id = (transaction_log_node.GetTransactionLogId() == target_transaction_log_id);
        const bool needs_rollback = transaction_log_node.NeedsRollback();
        if (is_active && has_matching_id && needs_rollback)
        {
            found_indices.push_back(i);
        }
    }
    return found_indices;
}

amp::optional<TransactionLogSet::TransactionLogIndex> FindNextAvailableSlotIndex(
    const TransactionLogSet::TransactionLogCollection& transaction_logs) noexcept
{
    for (TransactionLogSet::TransactionLogIndex i = 0; i < transaction_logs.size(); ++i)
    {
        const auto& transaction_log_node = transaction_logs.at(i);
        if (!transaction_log_node.IsActive())
        {
            return i;
        }
    }
    return {};
}

bool IsSkeletonElementTransactionLogIndex(const TransactionLogSet::TransactionLogIndex transaction_log_index) noexcept
{
    return transaction_log_index == TransactionLogSet::kSkeletonIndexSentinel;
}

}  // namespace

void TransactionLogSet::TransactionLogNode::Reset() noexcept
{
    AMP_ASSERT_PRD_MESSAGE(!transaction_log_.ContainsTransactions(),
                           "Cannot Reset TransactionLog as it still contains some old transactions.");
    is_active_ = false;
    needs_rollback_ = false;
}

TransactionLogSet::TransactionLogSet(const TransactionLogIndex max_number_of_logs,
                                     const std::size_t number_of_slots,
                                     const memory::shared::MemoryResourceProxy* const proxy) noexcept
    : proxy_transaction_logs_(max_number_of_logs, TransactionLogNode{number_of_slots, proxy}, proxy),
      skeleton_tracing_transaction_log_{number_of_slots, proxy},
      proxy_{proxy},
      transaction_log_mutex_{}
{
    AMP_PRECONDITION_PRD_MESSAGE(
        max_number_of_logs != kSkeletonIndexSentinel,
        "kSkeletonIndexSentinel is a reserved sentinel value so the max_number_of_logs must be reduced.");
}

void TransactionLogSet::MarkTransactionLogsNeedRollback(const TransactionLogId& transaction_log_id) noexcept
{
    for (auto& transaction_log_node : proxy_transaction_logs_)
    {
        const bool log_is_active = transaction_log_node.IsActive();
        const bool has_matching_id = (transaction_log_node.GetTransactionLogId() == transaction_log_id);
        if (log_is_active && has_matching_id)
        {
            transaction_log_node.MarkNeedsRollback(true);
        }
    }
}

ResultBlank TransactionLogSet::RollbackProxyTransactions(
    const TransactionLogId& transaction_log_id,
    const TransactionLog::DereferenceSlotCallback dereference_slot_callback,
    const TransactionLog::UnsubscribeCallback unsubscribe_callback) noexcept
{
    std::lock_guard lock{transaction_log_mutex_};
    const auto transaction_log_indices_to_be_rolled_back =
        FindTransactionLogIndicesToBeRolledBack(proxy_transaction_logs_, transaction_log_id);

    // Keep trying to rollback a TransactionLog. If a rollback succeeds, return. If a rollback fails, try to rollback
    // the next TransactionLog. If there are only TransactionLogs remaining which cannot be rolled back, return an
    // error.
    ResultBlank rollback_result{};
    for (const auto transaction_log_index : transaction_log_indices_to_be_rolled_back)
    {
        auto& transaction_log_node = proxy_transaction_logs_.at(transaction_log_index);
        rollback_result = transaction_log_node.GetTransactionLog().RollbackProxyElementLog(dereference_slot_callback,
                                                                                           unsubscribe_callback);
        if (rollback_result.has_value())
        {
            transaction_log_node.Reset();
            return {};
        }
    }
    return rollback_result;
}

ResultBlank TransactionLogSet::RollbackSkeletonTracingTransactions(
    const TransactionLog::DereferenceSlotCallback dereference_slot_callback) noexcept
{
    if (!skeleton_tracing_transaction_log_.IsActive())
    {
        return {};
    }
    const auto rollback_result =
        skeleton_tracing_transaction_log_.GetTransactionLog().RollbackSkeletonTracingElementLog(
            dereference_slot_callback);
    if (!rollback_result.has_value())
    {
        return rollback_result;
    }
    skeleton_tracing_transaction_log_.Reset();
    return {};
}

bmw::Result<TransactionLogSet::TransactionLogIndex> TransactionLogSet::RegisterProxyElement(
    const TransactionLogId& transaction_log_id) noexcept
{
    const std::lock_guard lock{transaction_log_mutex_};
    const auto next_available_slot_index_result = FindNextAvailableSlotIndex(proxy_transaction_logs_);
    if (!next_available_slot_index_result.has_value())
    {
        return MakeUnexpected(
            ComErrc::kMaxSubscribersExceeded,
            "Could not register with TransactionLogId as there are no available slots in the "
            "TransactionLogSet. This is likely because the number of subscribers has exceeded the configuration "
            "value of max_subscribers.");
    }
    auto& proxy_transaction_log = proxy_transaction_logs_.at(next_available_slot_index_result.value());
    proxy_transaction_log.SetTransactionLogId(transaction_log_id);
    proxy_transaction_log.MarkActive(true);
    proxy_transaction_log.MarkNeedsRollback(false);
    AMP_ASSERT_PRD_MESSAGE(!proxy_transaction_log.GetTransactionLog().ContainsTransactions(),
                           "Cannot reuse TransactionLog as it still contains some old transactions.");
    return *next_available_slot_index_result;
}

TransactionLogSet::TransactionLogIndex TransactionLogSet::RegisterSkeletonTracingElement() noexcept
{
    AMP_ASSERT_PRD_MESSAGE(!skeleton_tracing_transaction_log_.IsActive(),
                           "Can only register a single Skeleton Tracing element.");
    skeleton_tracing_transaction_log_.MarkActive(true);
    return kSkeletonIndexSentinel;
}

void TransactionLogSet::Unregister(const TransactionLogIndex transaction_log_index) noexcept
{
    if (IsSkeletonElementTransactionLogIndex(transaction_log_index))
    {
        skeleton_tracing_transaction_log_.Reset();
    }
    else
    {
        std::lock_guard lock{transaction_log_mutex_};
        proxy_transaction_logs_.at(transaction_log_index).Reset();
    }
}

TransactionLog& TransactionLogSet::GetTransactionLog(const TransactionLogIndex transaction_log_index) noexcept
{
    if (IsSkeletonElementTransactionLogIndex(transaction_log_index))
    {
        AMP_PRECONDITION_PRD_MESSAGE(skeleton_tracing_transaction_log_.IsActive(),
                                     "Skeleton tracing transaction log must be registered before being retrieved.");
        return skeleton_tracing_transaction_log_.GetTransactionLog();
    }
    AMP_PRECONDITION_PRD_MESSAGE(proxy_transaction_logs_.at(transaction_log_index).IsActive(),
                                 "Proxy tracing transaction log must be registered before being retrieved.");
    return proxy_transaction_logs_.at(transaction_log_index).GetTransactionLog();
}

}  // namespace bmw::mw::com::impl::lola
