// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#ifndef PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_TRANSACTION_LOG_H
#define PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_TRANSACTION_LOG_H

#include "platform/aas/mw/com/impl/bindings/lola/transaction_log_slot.h"

#include "platform/aas/lib/memory/shared/memory_resource_proxy.h"
#include "platform/aas/lib/memory/shared/polymorphic_offset_ptr_allocator.h"
#include "platform/aas/lib/result/result.h"

#include <amp_callback.hpp>

#include <cstdint>
#include <vector>

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

/// \brief Class which contains the state of a Proxy service element's (i.e. ProxyEvent / ProxyField) interaction with
///        shared memory.
///
/// Each Proxy service element instance will have its own TransactionLog which will record any Subscribe / Unsubscribe
/// calls as well as increments / decrements to the reference count of the corresponding Skeleton service element. The
/// TransactionLog has a Rollback function which undoes any previous operations that were recorded in the TransactionLog
/// so that the service element can be recreated (e.g. in the case of a crash).
class TransactionLog
{
    friend class TransactionLogAttorney;

  public:
    /// \todo Add a central location in which all the type aliases are placed so that these types align with their
    /// usages in other parts of the code base.
    using SlotIndexType = std::uint16_t;
    using MaxSampleCountType = std::uint16_t;

    using TransactionLogSlots =
        std::vector<TransactionLogSlot, memory::shared::PolymorphicOffsetPtrAllocator<TransactionLogSlot>>;

    /// \brief Callbacks called during Roll back
    ///
    /// These callbacks will be provided by reference and may be called multiple times by TransactionLogSet. Therefore,
    /// it should be ensured that it is safe to call these callbacks multiple times without violating any invariants in
    /// the state of the callbacks.
    using DereferenceSlotCallback = amp::callback<void(SlotIndexType slot_index)>;
    using UnsubscribeCallback = amp::callback<void(MaxSampleCountType subscription_max_sample_count)>;

    TransactionLog(std::size_t number_of_slots, const memory::shared::MemoryResourceProxy* proxy) noexcept;

    void SubscribeTransactionBegin(const std::size_t subscription_max_sample_count) noexcept;
    void SubscribeTransactionCommit() noexcept;
    void SubscribeTransactionAbort() noexcept;

    void UnsubscribeTransactionBegin() noexcept;
    void UnsubscribeTransactionCommit() noexcept;

    void ReferenceTransactionBegin(SlotIndexType slot_index) noexcept;
    void ReferenceTransactionCommit(SlotIndexType slot_index) noexcept;
    void ReferenceTransactionAbort(SlotIndexType slot_index) noexcept;

    void DereferenceTransactionBegin(SlotIndexType slot_index) noexcept;
    void DereferenceTransactionCommit(SlotIndexType slot_index) noexcept;

    /// \brief Rollback all previous increments and subscriptions that were recorded in the transaction log.
    /// \param dereference_slot_callback Callback which will decrement the slot in EventDataControl with the provided
    ///        index.
    /// \param unsubscribe_callback Callback which will perform the unsubscribe with the stored
    ///        subscription_max_sample_count_.
    ///
    /// This function should be called when trying to create a Proxy service element that had previously crashed. It
    /// will decrement all reference counts that the old Proxy had incremented in the EventDataControl which were
    /// recorded in this TransactionLog.
    ResultBlank RollbackProxyElementLog(const DereferenceSlotCallback& dereference_slot_callback,
                                        const UnsubscribeCallback& unsubscribe_callback) noexcept;

    /// \brief Rollback all previous increments that were recorded in the transaction log.
    /// \param dereference_slot_callback Callback which will decrement the slot in EventDataControl with the provided
    ///        index.
    ///
    /// This function should be called when trying to create a Skeleton service element that had previously crashed. It
    /// will decrement all reference counts that the old Skeleton (due to tracing) had incremented in the
    /// EventDataControl which were recorded in this TransactionLog.
    ResultBlank RollbackSkeletonTracingElementLog(const DereferenceSlotCallback& dereference_slot_callback) noexcept;

    /// \brief Checks whether the TransactionLog contains any transactions
    /// \return Returns true if there is at least one Subscribe transaction or Reference transaction that hasn't been
    ///         finished with a completed Unsubscribe or Dereference transaction.
    bool ContainsTransactions() const noexcept;

  private:
    ResultBlank RollbackIncrementTransactions(const DereferenceSlotCallback& dereference_slot_callback) noexcept;
    ResultBlank RollbackSubscribeTransactions(const UnsubscribeCallback& unsubscribe_callback) noexcept;

    /// \brief Vector containing one TransactionLogSlot for each slot in the corresponding control vector.
    TransactionLogSlots reference_count_slots_;

    /// \brief TransactionLogSlot in shared memory which will record subscribe / unsubscribe transactions.
    TransactionLogSlot subscribe_transactions_;

    /// \brief The max sample count used for the recorded subscription transaction.
    ///
    /// This is set in SubscribeTransactionBegin() and used in the UnsubscribeCallback which is called during Rollback()
    amp::optional<MaxSampleCountType> subscription_max_sample_count_;
};

}  // namespace lola
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_TRANSACTION_LOG_H
