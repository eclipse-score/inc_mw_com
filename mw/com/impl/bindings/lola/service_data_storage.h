// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#ifndef PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_SKELETON_DATA_STORAGE_H
#define PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_SKELETON_DATA_STORAGE_H

#include "platform/aas/mw/com/impl/bindings/lola/element_fq_id.h"
#include "platform/aas/mw/com/impl/bindings/lola/event_meta_info.h"

#include "platform/aas/lib/memory/shared/map.h"
#include "platform/aas/lib/memory/shared/memory_resource_proxy.h"
#include "platform/aas/lib/memory/shared/offset_ptr.h"

#include "platform/aas/lib/os/unistd.h"

namespace bmw::mw::com::impl::lola
{

class ServiceDataStorage
{
  public:
    explicit ServiceDataStorage(const bmw::memory::shared::MemoryResourceProxy* const proxy)
        : events_(proxy), events_metainfo_(proxy), skeleton_pid_{bmw::os::Unistd::instance().getpid()}
    {
    }

    bmw::memory::shared::Map<ElementFqId, bmw::memory::shared::OffsetPtr<void>> events_;
    bmw::memory::shared::Map<ElementFqId, EventMetaInfo> events_metainfo_;
    pid_t skeleton_pid_;
};

}  // namespace bmw::mw::com::impl::lola

#endif  // PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_SKELETON_DATA_STORAGE_H
