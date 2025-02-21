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


#ifndef PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_TRANSACTION_LOG_SET_H
#define PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_TRANSACTION_LOG_SET_H

#include "platform/aas/mw/com/impl/bindings/lola/transaction_log.h"
#include "platform/aas/mw/com/impl/bindings/lola/transaction_log_id.h"
#include "platform/aas/mw/com/impl/configuration/lola_event_instance_deployment.h"
#include "platform/aas/mw/com/impl/util/copyable_atomic.h"

#include "platform/aas/lib/memory/shared/memory_resource_proxy.h"
#include "platform/aas/lib/memory/shared/polymorphic_offset_ptr_allocator.h"
#include "platform/aas/lib/os/utils/interprocess/interprocess_mutex.h"
#include "platform/aas/lib/result/result.h"

#include <atomic>
#include <vector>

namespace bmw::mw::com::impl::lola
{

/// \brief TransactionLogSet keeps track of all the TransactionLogs for all the Proxy service elements corresponding to
///        a specific Skeleton service element. It also tracks a TransactionLog for the Skeleton service element in case
///        tracing is enabled.
///
/// Synchronisation: The TransactionLogSet consists of elements containing: a TransactionLogId and a TransactionLog.
/// Each TransactionLog will be used by a single Proxy service element in a single thread. However, different processes
/// or threads can iterate over the vector and read the TransactionLogId concurrently. Therefore, an element must not be
/// created or destroyed while another process is reading it. This could be solved using a lock free data structure
/// which reference counts the slots to ensure writing is only done when there are no readers. However, this approach
/// would require also recording the reference counting in the TransactionLog in case there is a crash while creating /
/// destroying one of the elements. Since the synchronisation is only required during Proxy service element construction
/// (which calls Rollbacktransactions()) and calls to Subscribe / Unsubscribe (which call Register() / Unregister(),
/// respectively), we will assume that the overhead of an interprocess mutex is bearable and will leave further
/// optimisations for the future if profiling identifies that the mutex is a bottleneck. GetTransactionLog(), which is
/// called with the highest frequency, will not be called under lock. This means that it cannot be called concurrently
/// with the same transaction_log_index as Unregister().
///
/// We use a vector instead of a map because we need to set the maximum size of the data structure (i.e. one element per
/// Proxy service element) and this is either not possible or not trivial with a hash map. We think that iterating over
/// this vector should be very quick due to the limited size of the vector and CPU caching (similar to the control
/// vector in EventDataControl).
class TransactionLogSet
{
    friend class TransactionLogSetAttorney;

  public:
    using TransactionLogIndex = LolaEventInstanceDeployment::SubscriberCountType;

    /// \brief Struct that stores the status of a given TransactionLog
    class TransactionLogNode
    {
      public:
        TransactionLogNode(const std::size_t number_of_slots,
                           const memory::shared::MemoryResourceProxy* const proxy) noexcept
            : is_active_{false}, needs_rollback_{false}, transaction_log_id_{}, transaction_log_(number_of_slots, proxy)
        {
        }
        bool IsActive() const noexcept { return is_active_; }
        bool NeedsRollback() const noexcept { return needs_rollback_; }

        void MarkActive(const bool is_active) noexcept { is_active_ = is_active; }
        void MarkNeedsRollback(const bool needs_rollback) noexcept { needs_rollback_ = needs_rollback; }

        void SetTransactionLogId(const TransactionLogId transaction_log_id) noexcept
        {
            transaction_log_id_ = transaction_log_id;
        }
        TransactionLogId GetTransactionLogId() const noexcept { return transaction_log_id_; }

        TransactionLog& GetTransactionLog() noexcept { return transaction_log_; }

        void Reset() noexcept;

      private:
        /// \brief Whether or not a TransactionLog is active.
        ///
        /// This is set in ProxyEvent::Subscribe and cleared in ProxyEvent::Unsubscribe. We use a flag to designate this
        /// instead of an amp::optional around the TransactionLog so that we can initialise the TransactionLog when we
        /// setup the shared memory so that we can correctly size the memory region.
        CopyableAtomic<bool> is_active_;

        /// \brief Whether or not the TransactionLog was created before a process crash.
        ///
        /// Will be set on Proxy::Create by the first Proxy in the same process with the same transaction_log_id. Will
        /// be cleared once Rollback is called on transaction_log.
        CopyableAtomic<bool> needs_rollback_;

        TransactionLogId transaction_log_id_;
        TransactionLog transaction_log_;
    };

    using TransactionLogCollection =
        std::vector<TransactionLogNode, memory::shared::PolymorphicOffsetPtrAllocator<TransactionLogNode>>;

    /// \brief Sentinel index value used to identify the skeleton_tracing_transaction_log_
    ///
    /// This value will be returned by RegisterSkeletonTracingElement() and when passed to GetTransactionLog(), the
    /// skeleton_tracing_transaction_log_ will be returned. We do this rather than having an additional
    /// GetTransactionLog overload for returned skeleton_tracing_transaction_log_ so that calling code can be agnostic
    /// to whether they're dealing with a proxy or skeleton transaction log.
    static constexpr const TransactionLogIndex kSkeletonIndexSentinel{std::numeric_limits<TransactionLogIndex>::max()};

    /// \brief Constructor
    /// \param max_number_of_logs The maximum number of logs that can be registered via Register().
    /// \param number_of_slots number of slots each of the transaction logs within the TransactionLogSet will contain.
    ///        It is deduced by the number_of_slots, the skeleton created for the related event/field service element.
    /// \param proxy The MemoryResourceProxy that will be used by the vector of transaction logs
    TransactionLogSet(const TransactionLogIndex max_number_of_logs,
                      const std::size_t number_of_slots,
                      const memory::shared::MemoryResourceProxy* const proxy) noexcept;
    ~TransactionLogSet() noexcept = default;

    TransactionLogSet(const TransactionLogSet&) = delete;
    TransactionLogSet& operator=(const TransactionLogSet&) = delete;
    TransactionLogSet(TransactionLogSet&&) noexcept = delete;
    TransactionLogSet& operator=(TransactionLogSet&& other) noexcept = delete;

    void MarkTransactionLogsNeedRollback(const TransactionLogId& transaction_log_id) noexcept;

    /// \brief Rolls back all Proxy TransactionLogs corresponding to the provided TransactionLogId.
    /// \return Returns an a blank result if the rollback succeeded or did not need to be done (because there's no
    ///         TransactionLog associated with the provided TransactionLogId or another Proxy instance with the same
    ///         TransactionLogId in the same process already performed the rollback), otherwise, an error.
    ///
    /// Multiple instances of the same Proxy service element will have the same transaction_log_id. Therefore, the first
    /// call to RollbackProxyTransactions per process will rollback _all_ the TransactionLogs corresponding to
    /// transaction_log_id. Any further calls to RollbackProxyTransactions within the same process will not perform any
    /// rollbacks. This prevents one thread calling RollbackProxyTransactions and then registering a new TransactionLog.
    /// Then another thread with the same transaction_log_id calls RollbackProxyTransactions which would rollback and
    /// destroy the newly created TransactionLog.
    ResultBlank RollbackProxyTransactions(const TransactionLogId& transaction_log_id,
                                          const TransactionLog::DereferenceSlotCallback dereference_slot_callback,
                                          const TransactionLog::UnsubscribeCallback unsubscribe_callback) noexcept;

    /// \brief If a Skeleton TransactionLog exists, performs a rollback on it.
    ResultBlank RollbackSkeletonTracingTransactions(
        const TransactionLog::DereferenceSlotCallback dereference_slot_callback) noexcept;

    /// \brief Creates a new transaction log in the vector of transaction logs.
    ///
    /// Will terminate if transaction_log_id already exists within the vector of transaction logs.
    bmw::Result<TransactionLogSet::TransactionLogIndex> RegisterProxyElement(
        const TransactionLogId& transaction_log_id) noexcept;

    /// \brief Creates a new skeleton tracing transaction log
    /// \return Returns kSkeletonIndexSentinel which is a special sentinel value which will return the registered
    /// skeleton tracing transaction log when passing the sentinel value to GetTransactionLog.
    ///
    /// Will terminate if a skeleton tracing transaction log was already registered.
    TransactionLogIndex RegisterSkeletonTracingElement() noexcept;

    /// \brief Deletes the element (by clearing the amp::optional) in the vector of transaction logs corresponding to
    /// the provided index.
    ///
    /// Must not be called concurrently with GetTransactionLog() with the same transaction_log_index.
    void Unregister(const TransactionLogIndex transaction_log_index) noexcept;

    /// \brief Returns a reference to a TransactionLog corresponding to the provided index.
    ///
    /// Must not be called concurrently with Unregister() with the same transaction_log_index.
    TransactionLog& GetTransactionLog(const TransactionLogIndex transaction_log_index) noexcept;

  private:
    TransactionLogCollection proxy_transaction_logs_;
    TransactionLogNode skeleton_tracing_transaction_log_;
    const memory::shared::MemoryResourceProxy* proxy_;
    os::InterprocessMutex transaction_log_mutex_;
};

}  // namespace bmw::mw::com::impl::lola

#endif  // PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_TRANSACTION_LOG_SET_H
