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


#include "platform/aas/mw/com/impl/bindings/lola/slot_collector.h"

#include "platform/aas/mw/com/impl/bindings/lola/i_runtime.h"
#include "platform/aas/mw/com/impl/bindings/lola/shm_path_builder.h"

#include "platform/aas/mw/com/impl/binding_event_receive_handler.h"
#include "platform/aas/mw/com/impl/runtime.h"

#include <iterator>
#include <utility>

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

SlotCollector::SlotCollector(EventDataControl& event_data_control,
                             const std::size_t max_slots,
                             TransactionLogSet::TransactionLogIndex transaction_log_index) noexcept
    : event_data_control_{event_data_control},
      last_ts_{0},
      collected_slots_(max_slots),
      transaction_log_index_{transaction_log_index}
{
}

std::size_t SlotCollector::GetNumNewSamplesAvailable() const noexcept
{
    return event_data_control_.get().GetNumNewEvents(last_ts_);
}

SlotCollector::SlotIndices SlotCollector::GetNewSamplesSlotIndices(const std::size_t max_count) noexcept
{
    const auto collected_slots_end_const_iterator = CollectSlots(max_count);

    EventSlotStatus::EventTimeStamp highest_delivered{last_ts_};
    for (auto slot = std::make_reverse_iterator(collected_slots_end_const_iterator); slot != collected_slots_.crend();
         ++slot)
    {
        const EventSlotStatus slot_status{event_data_control_.get()[*slot]};
        highest_delivered = std::max(highest_delivered, slot_status.GetTimeStamp());
    }

    last_ts_ = highest_delivered;

    return {std::make_reverse_iterator(collected_slots_end_const_iterator), collected_slots_.crend()};
}

SlotCollector::SlotIndexVector::const_iterator SlotCollector::CollectSlots(const std::size_t max_count) noexcept
{
    EventSlotStatus::EventTimeStamp current_highest = EventSlotStatus::TIMESTAMP_MAX;
    auto collected_slot = collected_slots_.begin();
    bool collected_slots_remaining = (collected_slot != collected_slots_.end());
    for (std::size_t count = 0; ((count < max_count) && collected_slots_remaining); ++count)
    {
        const amp::optional<EventDataControl::SlotIndexType> slot =
            event_data_control_.get().ReferenceNextEvent(last_ts_, transaction_log_index_, current_highest);
        if (slot.has_value())
        {
            const EventSlotStatus event_status{event_data_control_.get()[*slot]};
            current_highest = event_status.GetTimeStamp();
            *collected_slot = *slot;
            ++collected_slot;
        }
        else
        {
            break;
        }
        collected_slots_remaining = collected_slot != collected_slots_.end();
    }

    return collected_slot;
}

}  // namespace lola
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
