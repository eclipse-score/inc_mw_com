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


#include "platform/aas/mw/com/impl/bindings/lola/transaction_log_registration_guard.h"

#include "platform/aas/lib/result/result.h"

#include <amp_assert.hpp>

#include <utility>

namespace bmw::mw::com::impl::lola
{

bmw::Result<TransactionLogRegistrationGuard> TransactionLogRegistrationGuard::Create(
    EventDataControl& event_data_control,
    const TransactionLogId& transaction_log_id) noexcept
{
    auto transaction_log_index_result =
        event_data_control.GetTransactionLogSet().RegisterProxyElement(transaction_log_id);
    if (!(transaction_log_index_result.has_value()))
    {
        return MakeUnexpected<TransactionLogRegistrationGuard>(std::move(transaction_log_index_result).error());
    }
    return TransactionLogRegistrationGuard{event_data_control, transaction_log_index_result.value()};
}

TransactionLogRegistrationGuard::TransactionLogRegistrationGuard(
    EventDataControl& event_data_control,
    const TransactionLogSet::TransactionLogIndex transaction_log_index) noexcept
    : event_data_control_{event_data_control}, transaction_log_index_{transaction_log_index}
{
}

TransactionLogRegistrationGuard::~TransactionLogRegistrationGuard() noexcept
{
    if (transaction_log_index_.has_value())
    {
        event_data_control_.get().GetTransactionLogSet().Unregister(*transaction_log_index_);
    }
}

TransactionLogRegistrationGuard::TransactionLogRegistrationGuard(TransactionLogRegistrationGuard&& other) noexcept
    : event_data_control_{other.event_data_control_}, transaction_log_index_{other.transaction_log_index_}
{
    other.transaction_log_index_.reset();
}

TransactionLogSet::TransactionLogIndex TransactionLogRegistrationGuard::GetTransactionLogIndex() const noexcept
{
    AMP_PRECONDITION_PRD_MESSAGE(transaction_log_index_.has_value(),
                                 "GetTransactionLogIndex cannot be called without a transaction_log_index");
    return *transaction_log_index_;
}

}  // namespace bmw::mw::com::impl::lola
