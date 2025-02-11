// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#ifndef PLATFORM_AAS_MW_COM_IMPL_TRACING_TRACING_RUNTIME_MOCK_H
#define PLATFORM_AAS_MW_COM_IMPL_TRACING_TRACING_RUNTIME_MOCK_H

#include "platform/aas/mw/com/impl/tracing/i_tracing_runtime.h"

#include "gmock/gmock.h"

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

class TracingRuntimeMock : public ITracingRuntime
{
  public:
    MOCK_METHOD(void, DisableTracing, (), (noexcept, override));
    MOCK_METHOD(impl::tracing::ITracingRuntimeBinding::TraceContextId,
                RegisterServiceElement,
                (BindingType),
                (noexcept, override));
    MOCK_METHOD(void, SetDataLossFlag, (BindingType), (noexcept, override));
    MOCK_METHOD(void,
                RegisterShmObject,
                (BindingType, ServiceElementInstanceIdentifierView, analysis::tracing::ShmObjectHandle, void*),
                (noexcept, override));
    MOCK_METHOD(void, UnregisterShmObject, (BindingType, ServiceElementInstanceIdentifierView), (noexcept, override));

    MOCK_METHOD(ResultBlank,
                Trace,
                (BindingType,
                 impl::tracing::ITracingRuntimeBinding::TraceContextId trace_context_id,
                 ServiceElementInstanceIdentifierView,
                 TracePointType,
                 TracePointDataId,
                 TypeErasedSamplePtr sample_ptr,
                 const void*,
                 std::size_t),
                (noexcept, override));
    MOCK_METHOD(ResultBlank,
                Trace,
                (BindingType,
                 ServiceElementInstanceIdentifierView,
                 TracePointType,
                 amp::optional<TracePointDataId>,
                 const void*,
                 std::size_t),
                (noexcept, override));
};

}  // namespace tracing
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_TRACING_TRACING_RUNTIME_MOCK_H
