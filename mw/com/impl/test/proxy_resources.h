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


#ifndef PLATFORM_AAS_MW_COM_IMPL_TEST_PROXY_RESOURCES_H
#define PLATFORM_AAS_MW_COM_IMPL_TEST_PROXY_RESOURCES_H

#include "platform/aas/mw/com/impl/bindings/mock_binding/proxy_event.h"
#include "platform/aas/mw/com/impl/proxy_event_base.h"
#include "platform/aas/mw/com/impl/proxy_field.h"

#include <amp_assert.hpp>

#include <gmock/gmock.h>

namespace bmw
{
namespace mw
{
namespace com
{
namespace impl
{

template <typename FieldType>
class ProxyFieldAttorney
{
  public:
    ProxyFieldAttorney(ProxyField<FieldType>& proxy_field) noexcept : proxy_field_{proxy_field} {}

    ProxyEvent<FieldType>& GetProxyEvent() noexcept { return proxy_field_.proxy_event_dispatch_; }

  private:
    ProxyField<FieldType>& proxy_field_;
};

class ProxyEventBaseAttorney
{
  public:
    ProxyEventBaseAttorney(ProxyEventBase& proxy_event_base) noexcept : proxy_event_base_{proxy_event_base} {}

    template <typename FieldType>
    ProxyEventBaseAttorney(ProxyField<FieldType>& proxy_field) noexcept
        : proxy_event_base_{ProxyFieldAttorney<FieldType>{proxy_field}.GetProxyEvent()}
    {
    }

    ::testing::StrictMock<mock_binding::ProxyEventBase>* GetMockBinding() noexcept
    {
        auto* const mock_event_binding =
            dynamic_cast<::testing::StrictMock<mock_binding::ProxyEventBase>*>(proxy_event_base_.binding_base_.get());
        AMP_ASSERT_DBG(mock_event_binding != nullptr);
        return mock_event_binding;
    }

    SampleReferenceTracker& GetSampleReferenceTracker() noexcept
    {
        AMP_ASSERT_DBG(proxy_event_base_.tracker_ != nullptr);
        return *proxy_event_base_.tracker_;
    }

    tracing::ProxyEventTracingData GetProxyEventTracing() const noexcept { return proxy_event_base_.tracing_data_; }

  private:
    ProxyEventBase& proxy_event_base_;
};

}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_TEST_PROXY_RESOURCES_H
