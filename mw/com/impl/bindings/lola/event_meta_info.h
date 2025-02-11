// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#ifndef PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_EVENT_META_INFO_H
#define PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_EVENT_META_INFO_H

#include "platform/aas/lib/memory/shared/offset_ptr.h"
#include "platform/aas/mw/com/impl/bindings/lola/data_type_meta_info.h"

namespace bmw::mw::com::impl::lola
{

/// \brief meta-information about an event/its type.
/// \details Normally proxies/skeletons or "user code" dealing with an event, know its properties. This info is
///          provided and placed into shared-memory for the GenericProxy use-case, where a proxy connects to a provided
///          service based on only deployment info, NOT having any knowledge about the exact data type of the event.
class EventMetaInfo
{
  public:
    EventMetaInfo(const DataTypeMetaInfo data_type_info, const memory::shared::OffsetPtr<void> event_slots_raw_array)
        : data_type_info_(data_type_info), event_slots_raw_array_(event_slots_raw_array)
    {
    }
    DataTypeMetaInfo data_type_info_;
    memory::shared::OffsetPtr<void> event_slots_raw_array_;
};

}  // namespace bmw::mw::com::impl::lola

#endif  // PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_EVENT_META_INFO_H
