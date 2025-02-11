// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#ifndef PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_EVENT_DATA_STORAGE_H
#define PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_EVENT_DATA_STORAGE_H

#include "platform/aas/lib/containers/dynamic_array.h"
#include "platform/aas/lib/memory/shared/polymorphic_offset_ptr_allocator.h"

#include <scoped_allocator>

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

/// \brief Container for storing the actual data of a LoLa Event
///
/// \details This container will be accessed in parallel by multiple threads. The access must be synchronized via the
/// EventDataControl block. The idea is that a producer first needs to claim an event slot, then change the data within
/// the storage and then mark the slot as ready (similar for a consumer). This enables us cache optimized access of
/// these data structures. The overall contract will be abstracted for the end-user anyhow, so the separation into two
/// classes should be no problem.
template <typename SampleType>
using EventDataStorage = bmw::containers::
    DynamicArray<SampleType, std::scoped_allocator_adaptor<memory::shared::PolymorphicOffsetPtrAllocator<SampleType>>>;

}  // namespace lola
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_EVENT_DATA_STORAGE_H
