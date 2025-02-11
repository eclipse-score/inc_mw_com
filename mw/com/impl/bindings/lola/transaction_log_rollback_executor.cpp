// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#include "platform/aas/mw/com/impl/bindings/lola/transaction_log_rollback_executor.h"

#include "platform/aas/mw/com/impl/bindings/lola/runtime.h"
#include "platform/aas/mw/com/impl/bindings/lola/transaction_log_set.h"
#include "platform/aas/mw/com/impl/runtime.h"

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

void MarkTransactionLogsNeedRollback(std::unordered_set<ServiceDataControl*>& synchronisation_data_set,
                                     ServiceDataControl* const service_data_control,
                                     const TransactionLogId transaction_log_id) noexcept
{
    for (auto& element : service_data_control->event_controls_)
    {
        auto& event_data_control = element.second.data_control;
        auto& transaction_log_set = event_data_control.GetTransactionLogSet();
        transaction_log_set.MarkTransactionLogsNeedRollback(transaction_log_id);
    }
    auto rollback_data_it = synchronisation_data_set.insert(service_data_control);
    AMP_ASSERT_PRD_MESSAGE(rollback_data_it.second, "Failed to emplace rollback data!");
}

}  // namespace

TransactionLogRollbackExecutor::TransactionLogRollbackExecutor(ServiceDataControl* const service_data_control,
                                                               const QualityType asil_level,
                                                               pid_t provider_pid,
                                                               const TransactionLogId transaction_log_id) noexcept
    : service_data_control_{service_data_control},
      asil_level_{asil_level},
      provider_pid_{provider_pid},
      transaction_log_id_{transaction_log_id}
{
}

void TransactionLogRollbackExecutor::PrepareRollback() noexcept
{
    auto* const lola_runtime =
        dynamic_cast<lola::IRuntime*>(mw::com::impl::Runtime::getInstance().GetBindingRuntime(BindingType::kLoLa));
    AMP_ASSERT_PRD_MESSAGE(lola_runtime != nullptr, "Lola runtime does not exist!");
    auto& rollback_data = lola_runtime->GetRollbackData();

    std::lock_guard map_lock{rollback_data.set_mutex};

    // If another Proxy instance has already prepared the rollback for the given service_data_control_ (the special case
    // where we have more than one proxy instance in the same process, which uses the same service instance ->
    // service_data_control_), then we can return early.
    if (rollback_data.synchronisation_data_set.count(service_data_control_) > 0)
    {
        return;
    }

    // Register with our uid (which is the transaction_log_id_ and current pid in the
    // service_data_control_->uid_pid_mapping_
    const auto current_pid = lola_runtime->GetPid();
    const auto previous_pid = service_data_control_->uid_pid_mapping_.RegisterPid(transaction_log_id_, current_pid);
    AMP_ASSERT_PRD_MESSAGE(previous_pid.has_value(), "Couldn't Register current PID for UID within shared-memory!");
    if (previous_pid.value() != current_pid)
    {
        // we found an old/outdated PID for our UID in the shared-memory of the service-instance. Notify provider, that
        // this pid is outdated.
        lola_runtime->GetLolaMessaging().NotifyOutdatedNodeId(asil_level_, previous_pid.value(), provider_pid_);
    }

    // Mark all TransactionLogs for each event that correspond to transaction_log_id as needing to be rolled back.
    MarkTransactionLogsNeedRollback(rollback_data.synchronisation_data_set, service_data_control_, transaction_log_id_);
}

ResultBlank TransactionLogRollbackExecutor::RollbackTransactionLogs() noexcept
{
    PrepareRollback();

    for (auto& element : service_data_control_->event_controls_)
    {
        auto& event_control = element.second;
        auto& transaction_log_set = event_control.data_control.GetTransactionLogSet();
        const auto rollback_result = transaction_log_set.RollbackProxyTransactions(
            transaction_log_id_,
            [&event_control](const TransactionLog::SlotIndexType slot_index) noexcept {
                event_control.data_control.DereferenceEventWithoutTransactionLogging(slot_index);
            },
            [&event_control](const TransactionLog::MaxSampleCountType subscription_max_sample_count) noexcept {
                event_control.subscription_control.Unsubscribe(subscription_max_sample_count);
            });
        if (!rollback_result.has_value())
        {
            return rollback_result;
        }
    }
    return {};
}

}  // namespace lola
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
