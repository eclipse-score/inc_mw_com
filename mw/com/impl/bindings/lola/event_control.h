// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#ifndef PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_EVENT_CONTROL_H
#define PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_EVENT_CONTROL_H

#include "platform/aas/mw/com/impl/bindings/lola/event_data_control.h"
#include "platform/aas/mw/com/impl/bindings/lola/event_subscription_control.h"

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

class EventControl
{
  public:
    using SlotIndexType = EventDataControl::SlotIndexType;
    using SubscriberCountType = EventSubscriptionControl::SubscriberCountType;
    EventControl(const SlotIndexType number_of_slots,
                 const SubscriberCountType max_subscribers,
                 const bool enforce_max_samples,
                 const bmw::memory::shared::MemoryResourceProxy* const proxy) noexcept;
    EventDataControl data_control;
    EventSubscriptionControl subscription_control;
};

}  // namespace lola
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_EVENT_CONTROL_H
