// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#ifndef PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_TRANSACTION_LOG_REGISTRATION_GUARD_H
#define PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_TRANSACTION_LOG_REGISTRATION_GUARD_H

#include "platform/aas/lib/result/result.h"
#include "platform/aas/mw/com/impl/bindings/lola/event_data_control.h"
#include "platform/aas/mw/com/impl/bindings/lola/transaction_log_id.h"
#include "platform/aas/mw/com/impl/bindings/lola/transaction_log_set.h"

#include <amp_optional.hpp>

#include <functional>

namespace bmw::mw::com::impl::lola
{

/**
 * RAII helper class that will call TransactionLogSet::RegisterTransactionLog on construction and
 * EventDataControl::UnregisterTransactionLog on destruction.
 */
class TransactionLogRegistrationGuard
{
  public:
    static bmw::Result<TransactionLogRegistrationGuard> Create(EventDataControl& event_data_control,
                                                               const TransactionLogId& transaction_log_id) noexcept;

    ~TransactionLogRegistrationGuard() noexcept;

    TransactionLogRegistrationGuard(const TransactionLogRegistrationGuard&) = delete;
    TransactionLogRegistrationGuard& operator=(const TransactionLogRegistrationGuard&) = delete;
    TransactionLogRegistrationGuard& operator=(TransactionLogRegistrationGuard&& other) noexcept = delete;

    // The TransactionLogRegistrationGuard must be move constructible so that it can be wrapped in an amp::optional.
    TransactionLogRegistrationGuard(TransactionLogRegistrationGuard&& other) noexcept;

    TransactionLogSet::TransactionLogIndex GetTransactionLogIndex() const noexcept;

  private:
    TransactionLogRegistrationGuard(EventDataControl& event_data_control,
                                    const TransactionLogSet::TransactionLogIndex transaction_log_index) noexcept;

    std::reference_wrapper<EventDataControl> event_data_control_;
    amp::optional<TransactionLogSet::TransactionLogIndex> transaction_log_index_;
};

}  // namespace bmw::mw::com::impl::lola

#endif  // PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_TRANSACTION_LOG_REGISTRATION_GUARD_H
