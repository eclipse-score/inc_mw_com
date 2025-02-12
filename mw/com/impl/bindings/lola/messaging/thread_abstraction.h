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


#ifndef PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_THREADABSTRACTION_H
#define PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_THREADABSTRACTION_H

#include <cstdint>
#include <thread>

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

class ThreadHWConcurrencyIfc
{
  public:
    ThreadHWConcurrencyIfc() noexcept = default;

    virtual ~ThreadHWConcurrencyIfc() noexcept = default;

    ThreadHWConcurrencyIfc(ThreadHWConcurrencyIfc&&) = delete;
    ThreadHWConcurrencyIfc& operator=(ThreadHWConcurrencyIfc&&) = delete;
    ThreadHWConcurrencyIfc(const ThreadHWConcurrencyIfc&) = delete;
    ThreadHWConcurrencyIfc& operator=(const ThreadHWConcurrencyIfc&) = delete;

    virtual std::uint32_t hardware_concurrency() const noexcept = 0;
};

class ThreadHWConcurrency
{
  public:
    static std::uint32_t hardware_concurrency() noexcept;
    static void injectMock(const ThreadHWConcurrencyIfc* const mock) { mock_ = mock; }

  private:
    const static ThreadHWConcurrencyIfc* mock_;
};

}  // namespace lola
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_THREADABSTRACTION_H
