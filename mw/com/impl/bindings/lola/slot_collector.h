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


#ifndef PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_PROXY_EVENT_CLIENT_H
#define PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_PROXY_EVENT_CLIENT_H

#include "platform/aas/mw/com/impl/bindings/lola/element_fq_id.h"
#include "platform/aas/mw/com/impl/bindings/lola/event_data_control.h"
#include "platform/aas/mw/com/impl/bindings/lola/event_slot_status.h"
#include "platform/aas/mw/com/impl/bindings/lola/proxy.h"
#include "platform/aas/mw/com/impl/bindings/lola/transaction_log_set.h"

#include "platform/aas/mw/com/impl/binding_event_receive_handler.h"
#include "platform/aas/mw/com/impl/com_error.h"

#include <functional>
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

/// This class interfaces with the EventDataControl in shared memory to handle finding the slots containing new samples
/// that are pending reception.
class SlotCollector final
{
  public:
    using SlotIndexVector = std::vector<EventDataControl::SlotIndexType>;

    struct SlotIndices
    {
        SlotCollector::SlotIndexVector::const_reverse_iterator begin;
        SlotCollector::SlotIndexVector::const_reverse_iterator end;
    };

    /// Create a SlotCollector for the specified service instance and event.
    ///
    /// \param event_data_control EventDataControl to be used for data reception.
    /// \param max_slots Maximum number of samples that will be received in one call to GetNewSamples.
    SlotCollector(EventDataControl& event_data_control,
                  const std::size_t max_slots,
                  TransactionLogSet::TransactionLogIndex transaction_log_index) noexcept;

    SlotCollector(SlotCollector&& other) noexcept = default;
    SlotCollector& operator=(SlotCollector&& other) & noexcept = delete;
    SlotCollector& operator=(const SlotCollector&) & = delete;
    SlotCollector(const SlotCollector&) = delete;

    ~SlotCollector() = default;

    /// \brief Returns the number of new samples a call to GetNewSamples() (given that parameter max_count
    ///        doesn't restrict it) would currently provide.
    ///
    /// \return number of new samples a call to GetNewSamples() would currently provide.
    std::size_t GetNumNewSamplesAvailable() const noexcept;

    /// Get the indices of the slots containing samples that are pending for reception.
    ///
    /// This function is not thread-safe: It may be called from different threads, but the calls need to be
    /// synchronized.
    ///
    /// \param max_count The maximum number of callbacks that shall be executed.
    /// \return Number of samples received.
    SlotIndices GetNewSamplesSlotIndices(const std::size_t max_count) noexcept;

  private:
    SlotIndexVector::const_iterator CollectSlots(const std::size_t max_count) noexcept;

    std::reference_wrapper<EventDataControl> event_data_control_;
    EventSlotStatus::EventTimeStamp last_ts_;
    SlotIndexVector collected_slots_;  // Pre-allocated scratchpad memory to present the events in-order to the user.
    TransactionLogSet::TransactionLogIndex transaction_log_index_;
};

}  // namespace lola
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_PROXY_EVENT_CLIENT_H
