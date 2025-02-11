// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#ifndef PLATFORM_AAS_MW_COM_IMPL_BINDINGS_MOCK_BINDING_TRACING_TRACING_RUNTIME_H
#define PLATFORM_AAS_MW_COM_IMPL_BINDINGS_MOCK_BINDING_TRACING_TRACING_RUNTIME_H

#include "platform/aas/mw/com/impl/tracing/i_tracing_runtime_binding.h"

#include <gmock/gmock.h>

namespace bmw
{
namespace mw
{
namespace com
{
namespace impl
{
namespace tracing
{
namespace mock_binding
{

class TracingRuntime : public ITracingRuntimeBinding
{
  public:
    MOCK_METHOD(TraceContextId, RegisterServiceElement, (), (noexcept, override));
    MOCK_METHOD(bool, RegisterWithGenericTraceApi, (), (noexcept, override));
    MOCK_METHOD(analysis::tracing::TraceClientId, GetTraceClientId, (), (const, noexcept, override));
    MOCK_METHOD(void, SetDataLossFlag, (const bool new_value), (noexcept, override));
    MOCK_METHOD(bool, GetDataLossFlag, (), (const, noexcept, override));
    MOCK_METHOD(void,
                RegisterShmObject,
                (const impl::tracing::ServiceElementInstanceIdentifierView&, analysis::tracing::ShmObjectHandle, void*),
                (noexcept, override));
    MOCK_METHOD(void,
                UnregisterShmObject,
                (const impl::tracing::ServiceElementInstanceIdentifierView&),
                (noexcept, override));
    MOCK_METHOD(amp::optional<analysis::tracing::ShmObjectHandle>,
                GetShmObjectHandle,
                (const impl::tracing::ServiceElementInstanceIdentifierView&),
                (const, noexcept, override));
    MOCK_METHOD(amp::optional<void*>,
                GetShmRegionStartAddress,
                (const impl::tracing::ServiceElementInstanceIdentifierView&),
                (const, noexcept, override));
    MOCK_METHOD(void,
                CacheFileDescriptorForReregisteringShmObject,
                (const impl::tracing::ServiceElementInstanceIdentifierView&,
                 memory::shared::ISharedMemoryResource::FileDescriptor,
                 void*),
                (noexcept, override));
    MOCK_METHOD((amp::optional<std::pair<memory::shared::ISharedMemoryResource::FileDescriptor, void*>>),
                GetCachedFileDescriptorForReregisteringShmObject,
                (const impl::tracing::ServiceElementInstanceIdentifierView&),
                (const, noexcept, override));
    MOCK_METHOD(void,
                ClearCachedFileDescriptorForReregisteringShmObject,
                (const impl::tracing::ServiceElementInstanceIdentifierView&),
                (noexcept, override));
    MOCK_METHOD(analysis::tracing::ServiceInstanceElement,
                ConvertToTracingServiceInstanceElement,
                (impl::tracing::ServiceElementInstanceIdentifierView),
                (const, override));
    MOCK_METHOD(bool, IsServiceElementTracingActive, (TraceContextId), (noexcept, override));
    MOCK_METHOD(void, SetTypeErasedSamplePtr, (TypeErasedSamplePtr, TraceContextId), (noexcept, override));
    MOCK_METHOD(void, ClearTypeErasedSamplePtr, (TraceContextId), (noexcept, override));
};

}  // namespace mock_binding
}  // namespace tracing
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_BINDINGS_MOCK_BINDING_TRACING_TRACING_RUNTIME_H
