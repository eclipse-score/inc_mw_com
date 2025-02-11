// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#ifndef PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_TEST_DOUBLES_FAKE_MEMORY_RESOURCE_H
#define PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_TEST_DOUBLES_FAKE_MEMORY_RESOURCE_H

#include "platform/aas/lib/memory/shared/managed_memory_resource.h"

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

/// \brief Memory Resource that will directly allocate memory on HEAP
class FakeMemoryResource : public memory::shared::ManagedMemoryResource
{
  public:
    const memory::shared::MemoryResourceProxy* getMemoryResourceProxy() noexcept override { return nullptr; }

    void* getBaseAddress() override { return nullptr; }

    void* getUsableBaseAddress() override { return nullptr; }

    std::size_t GetUserAllocatedBytes() const override { return 0U; };

    bool IsBoundsCheckingBypassingEnabled() const noexcept override { return true; }

  private:
    const void* getEndAddress() const noexcept override { return nullptr; }
    
    void* do_allocate(const std::size_t bytes, std::size_t) override { return std::malloc(bytes); }

    void do_deallocate(void* const memory, std::size_t, std::size_t) override { std::free(memory); }
    

    bool do_is_equal(const memory_resource&) const noexcept override { return false; }
};

}  // namespace lola
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_TEST_DOUBLES_FAKE_MEMORY_RESOURCE_H
