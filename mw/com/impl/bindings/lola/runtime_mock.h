// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#ifndef PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_RUNTIME_MOCK_H
#define PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_RUNTIME_MOCK_H

#include "platform/aas/mw/com/impl/bindings/lola/i_runtime.h"

#include "gmock/gmock.h"

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

/// \brief Mock class for mocking of LoLa binding specific runtime interface.
class RuntimeMock : public IRuntime
{
  public:
    MOCK_METHOD(IMessagePassingService&, GetLolaMessaging, (), (noexcept, override));
    // 
    MOCK_METHOD(bool, HasAsilBSupport, (), (const, noexcept, override));
    // 
    MOCK_METHOD(BindingType, GetBindingType, (), (const, noexcept, override));
    // 
    MOCK_METHOD(ShmSizeCalculationMode, GetShmSizeCalculationMode, (), (const, override));
    // 
    MOCK_METHOD(IServiceDiscoveryClient&, GetServiceDiscoveryClient, (), (noexcept, override));
    // 
    MOCK_METHOD(impl::tracing::ITracingRuntimeBinding*, GetTracingRuntime, (), (noexcept, override));
    // 
    MOCK_METHOD(RollbackData&, GetRollbackData, (), (noexcept, override));
    // 
    MOCK_METHOD(pid_t, GetPid, (), (const, noexcept, override));
    // 
    MOCK_METHOD(uid_t, GetUid, (), (const, noexcept, override));
};

}  // namespace lola
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_RUNTIME_MOCK_H
