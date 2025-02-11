// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#ifndef PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_EVENT_DATA_CONTROL_H
#define PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_EVENT_DATA_CONTROL_H

#include "platform/aas/mw/com/impl/bindings/lola/event_slot_status.h"

#include "platform/aas/mw/com/impl/bindings/lola/transaction_log_id.h"
#include "platform/aas/mw/com/impl/bindings/lola/transaction_log_set.h"

#include "platform/aas/lib/memory/shared/memory_resource_proxy.h"
#include "platform/aas/lib/memory/shared/polymorphic_offset_ptr_allocator.h"

#include "platform/aas/lib/memory/shared/atomic_indirector.h"

#include "platform/aas/lib/containers/dynamic_array.h"

#include <amp_optional.hpp>

#include <atomic>
#include <cstdint>
#include <tuple>

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
class EventDataControlAttorney;
class EventDataControlCompositeAttorney;

namespace detail_event_data_control_composite
{
template <template <class> class T>
class EventDataControlCompositeImpl;
}  // namespace detail_event_data_control_composite

namespace detail_event_data_control
{

/// \brief EventDataControlImpl encapsulates the overall control information for one event. It is stored in Shared
/// Memory.
///
/// \details Underlying EventDataControlImpl holds a dynamic array of multiple slots, which hold EventSlotStatus. The
/// event has another equally sized dynamic array of slots which will contain the data. Both data points (data and
/// control information) are related by their slot index. The number of slots is configured on construction (start-up of
/// a process).
///
/// It is one of the corner stone elements of our LoLa IPC for Events!
template <template <class> class AtomicIndirectorType = memory::shared::AtomicIndirectorReal>
class EventDataControlImpl final
{
    friend class lola::EventDataControlAttorney;
    friend class lola::EventDataControlCompositeAttorney;

  public:
    /// \brief Represents the type for the index to access the underlying slots
    using SlotIndexType = std::uint16_t;
    using EventControlSlots = bmw::containers::DynamicArray<
        std::atomic<EventSlotStatus::value_type>,
        memory::shared::PolymorphicOffsetPtrAllocator<std::atomic<EventSlotStatus::value_type>>>;

    /// \brief Will construct EventDataControlImpl and dynamically allocate memory on provided resource on
    /// construction
    /// \param max_slots The number of slots that shall be allocated (const afterwards)
    /// \param proxy The memory resource proxy where the memory shall be allocated (e.g. Shared Memory)
    /// \param max_number_combined_subscribers The max number of subscribers which can subscribe to the SkeletonEvent
    ///        owning this EventDataControl at any one time.
    EventDataControlImpl(
        const SlotIndexType max_slots,
        const bmw::memory::shared::MemoryResourceProxy* const proxy,
        const LolaEventInstanceDeployment::SubscriberCountType max_number_combined_subscribers) noexcept;
    ~EventDataControlImpl() noexcept = default;

    EventDataControlImpl(const EventDataControlImpl&) = delete;
    EventDataControlImpl& operator=(const EventDataControlImpl&) = delete;
    EventDataControlImpl(EventDataControlImpl&&) noexcept = delete;
    EventDataControlImpl& operator=(EventDataControlImpl&& other) noexcept = delete;

    /// \brief Checks for the oldest unused slot and acquires for writing (thread-safe, wait-free)
    ///
    /// \details This method will perform retries (bounded) on data-races. In order to ensure that _always_
    /// a slot is found, it needs to be ensured that:
    /// * enough slots are allocated (sum of all possible max allocations by consumer + 1)
    /// * enough retries are performed (currently max number of parallel actions is restricted to 50 (number of
    /// possible transactions (2) * number of parallel actions = number of retries))
    ///
    /// \return reserved slot for writing if found, empty otherwise
    /// \post EventReady() is invoked to withdraw write-ownership
    amp::optional<SlotIndexType> AllocateNextSlot() noexcept;

    /// \brief Indicates that a slot is ready for reading - writing has finished. (thread-safe, wait-free)
    /// \pre AllocateNextSlot() was invoked to obtain write-ownership
    void EventReady(const SlotIndexType slot_index, const EventSlotStatus::EventTimeStamp time_stamp) noexcept;

    /// \brief Marks selected slot as invalid, if it was not yet marked as ready
    ///
    /// \details We don't discard elements that are already ready, since it is possible that a user might already
    /// read them. This just might be the case if a SampleAllocateePtr is destroyed after invoking Send().
    ///
    /// \pre AllocateNextSlot() was invoked to obtain write-ownership
    void Discard(const SlotIndexType slot_index) noexcept;

    /// \brief Increments refcount of given slot by one (given it is in the correct state i.e. being accessible/
    ///        readable)
    /// \details This is a specific feature - not used by the standard proxy/consumer, which is using
    ///          ReferenceNextEvent(). This API has been introduced in the context of IPC-Tracing, where a skeleton is
    ///          referencing/using a slot it just has allocated to trace out the content via Trace-API and
    ///          de-referencing it after tracing of the slot data has been accomplished.
    /// \param slot_index index of the slot to be referenced.
    bool ReferenceSpecificEvent(const SlotIndexType slot_index,
                                const TransactionLogSet::TransactionLogIndex transaction_log_index) noexcept;

    /// \brief Will search for the next slot that shall be read, after the last time and mark it for reading
    /// \param last_search_time The time stamp the last time a search for an event was performed
    ///
    /// \details This method will perform retries (bounded) on data-races. I.e. if a viable slot failed to be marked
    /// for reading because of a data race, retries are made.
    ///
    /// \return Will return the index of an event, if one exists > last_search_time, empty otherwise
    /// \post DereferenceEvent() is invoked to withdraw read-ownership
    amp::optional<SlotIndexType> ReferenceNextEvent(
        const EventSlotStatus::EventTimeStamp last_search_time,
        const TransactionLogSet::TransactionLogIndex transaction_log_index,
        const EventSlotStatus::EventTimeStamp upper_limit = EventSlotStatus::TIMESTAMP_MAX) noexcept;

    /// \brief Returns number/count of events within event slots, which are newer than the given timestamp.
    /// \param reference_time given reference timestamp.
    /// \return number/count of available events, which are newer than the given reference_time.
    std::size_t GetNumNewEvents(const EventSlotStatus::EventTimeStamp reference_time) const noexcept;

    /// \brief Indicates that a consumer is finished reading (thread-safe, wait-free)
    /// \pre ReferenceNextEvent() was invoked to obtain read-ownership
    ///
    /// \details Will also record the transaction in the TransactionLog corresponding to transaction_log_index
    void DereferenceEvent(const SlotIndexType event_slot_index,
                          const TransactionLogSet::TransactionLogIndex transaction_log_index) noexcept;

    /// \brief Indicates that a consumer is finished reading (thread-safe, wait-free)
    /// \pre ReferenceNextEvent() was invoked to obtain read-ownership
    ///
    /// \details Will not record the transaction in any TransactionLog. This function should be called by the
    /// TransactionLog::DereferenceSlotCallback created by ProxyEventCommon. In that case, the transaction will be
    /// recorded within TransactionLog::Rollback before calling the callback.
    void DereferenceEventWithoutTransactionLogging(const SlotIndexType event_slot_index) noexcept;

    /// \brief Directly access EventSlotStatus for one specific slot (no bound check performed!)
    EventSlotStatus operator[](const SlotIndexType slot_index) const noexcept;

    /// \brief Marks all Slots which are `InWriting` as `Invalid`.
    /// \details This function shall _only_ be called on skeleton side and _only_ if a previous skeleton instance died.
    void RemoveAllocationsForWriting() noexcept;

    TransactionLogSet& GetTransactionLogSet() noexcept { return transaction_log_set_; }

    // helper for performance indication (no production usage)
    static void DumpPerformanceCounters();
    static void ResetPerformanceCounters();

  private:
    amp::optional<SlotIndexType> FindOldestUnusedSlot() const noexcept;

    // Shared Memory ready :)!
    // we don't implement a smarter structure and just iterate through it, because we believe that by
    // cache optimization this is way faster then e.g. a tree, since a tree also needs to be
    // implement wait-free.
    EventControlSlots state_slots_;

    TransactionLogSet transaction_log_set_;

    // helper variables to calculated performance indicators
    static inline std::atomic_uint_fast64_t num_alloc_misses{0U};
    static inline std::atomic_uint_fast64_t num_ref_misses{0U};
    static inline std::atomic_uint_fast64_t num_alloc_retries{0U};
    static inline std::atomic_uint_fast64_t num_ref_retries{0U};

    template <template <typename> class T>
    friend class detail_event_data_control_composite::EventDataControlCompositeImpl;
};

}  // namespace detail_event_data_control

using EventDataControl = detail_event_data_control::EventDataControlImpl<>;

}  // namespace lola
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_EVENT_DATA_CONTROL_H
