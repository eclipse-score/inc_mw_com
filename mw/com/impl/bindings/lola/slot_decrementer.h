// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#ifndef PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_SLOT_DECREMENTER_H
#define PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_SLOT_DECREMENTER_H

#include "platform/aas/mw/com/impl/bindings/lola/event_data_control.h"
#include "platform/aas/mw/com/impl/bindings/lola/transaction_log_set.h"

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

class SlotDecrementer
{
  public:
    SlotDecrementer(EventDataControl* event_data_control,
                    const EventDataControl::SlotIndexType event_slot_index,
                    const TransactionLogSet::TransactionLogIndex transaction_log_idx) noexcept;
    ~SlotDecrementer() noexcept;

    SlotDecrementer(const SlotDecrementer&) = delete;
    SlotDecrementer& operator=(const SlotDecrementer&) = delete;
    SlotDecrementer(SlotDecrementer&& other) noexcept;
    SlotDecrementer& operator=(SlotDecrementer&& other) noexcept;

  private:
    void internal_delete() noexcept;

    EventDataControl* event_data_control_;
    EventDataControl::SlotIndexType event_slot_index_;
    TransactionLogSet::TransactionLogIndex transaction_log_idx_;
};

}  // namespace lola
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_SLOT_DECREMENTER_H
