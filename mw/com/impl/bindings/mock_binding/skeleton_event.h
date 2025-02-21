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


#ifndef PLATFORM_AAS_MW_COM_IMPL_BINDINGS_MOCK_BINDING_SKELETON_EVENT_H
#define PLATFORM_AAS_MW_COM_IMPL_BINDINGS_MOCK_BINDING_SKELETON_EVENT_H

#include "platform/aas/mw/com/impl/plumbing/sample_allocatee_ptr.h"
#include "platform/aas/mw/com/impl/skeleton_event_binding.h"

#include <gmock/gmock.h>

#include <memory>

namespace bmw
{
namespace mw
{
namespace com
{
namespace impl
{
namespace mock_binding
{

class SkeletonEventBase : public SkeletonEventBindingBase
{
  public:
    MOCK_METHOD(ResultBlank, PrepareOffer, (), (noexcept, override));
    MOCK_METHOD(void, PrepareStopOffer, (), (noexcept, override));
    MOCK_METHOD(std::size_t, GetMaxSize, (), (const, noexcept, override));
    MOCK_METHOD(BindingType, GetBindingType, (), (const, noexcept, override));
    MOCK_METHOD(void, SetSkeletonEventTracingData, (impl::tracing::SkeletonEventTracingData), (noexcept, override));
};

template <typename SampleType>
class SkeletonEvent : public SkeletonEventBinding<SampleType>
{
  public:
    MOCK_METHOD(ResultBlank,
                Send,
                (const SampleType& value, amp::optional<typename SkeletonEventBinding<SampleType>::SendTraceCallback>),
                (noexcept, override));
    MOCK_METHOD(ResultBlank,
                Send,
                (SampleAllocateePtr<SampleType> sample,
                 amp::optional<typename SkeletonEventBinding<SampleType>::SendTraceCallback>),
                (noexcept, override));
    MOCK_METHOD(Result<SampleAllocateePtr<SampleType>>, Allocate, (), (noexcept, override));
    MOCK_METHOD(ResultBlank, PrepareOffer, (), (noexcept, override));
    MOCK_METHOD(void, PrepareStopOffer, (), (noexcept, override));
    MOCK_METHOD(std::size_t, GetMaxSize, (), (const, noexcept, override));
    MOCK_METHOD(BindingType, GetBindingType, (), (const, noexcept, override));
    MOCK_METHOD(void, SetSkeletonEventTracingData, (impl::tracing::SkeletonEventTracingData), (noexcept, override));
};

}  // namespace mock_binding
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_BINDINGS_MOCK_BINDING_SKELETON_EVENT_H
