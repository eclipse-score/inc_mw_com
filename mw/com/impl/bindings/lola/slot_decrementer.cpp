// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#include "platform/aas/mw/com/impl/bindings/lola/slot_decrementer.h"

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

SlotDecrementer::SlotDecrementer(EventDataControl* event_data_control,
                                 const EventDataControl::SlotIndexType event_slot_index,
                                 const TransactionLogSet::TransactionLogIndex transaction_log_idx) noexcept
    : event_data_control_{event_data_control},
      event_slot_index_{event_slot_index},
      transaction_log_idx_{transaction_log_idx}
{
}

SlotDecrementer::~SlotDecrementer() noexcept
{
    internal_delete();
}

SlotDecrementer::SlotDecrementer(SlotDecrementer&& other) noexcept
    : event_data_control_{other.event_data_control_},
      event_slot_index_{other.event_slot_index_},
      transaction_log_idx_{other.transaction_log_idx_}
{
    other.event_data_control_ = nullptr;
}

SlotDecrementer& SlotDecrementer::operator=(SlotDecrementer&& other) noexcept
{
    if (this != &other)
    {
        internal_delete();
        event_data_control_ = other.event_data_control_;
        event_slot_index_ = other.event_slot_index_;
        transaction_log_idx_ = other.transaction_log_idx_;

        other.event_data_control_ = nullptr;
    }
    return *this;
}

void SlotDecrementer::internal_delete() noexcept
{
    if (event_data_control_ != nullptr)
    {
        event_data_control_->DereferenceEvent(event_slot_index_, transaction_log_idx_);
    }
}

}  // namespace lola
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
