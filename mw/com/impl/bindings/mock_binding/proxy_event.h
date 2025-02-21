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


#ifndef PLATFORM_AAS_MW_COM_IMPL_BINDINGS_MOCK_BINDING_PROXY_EVENT_H
#define PLATFORM_AAS_MW_COM_IMPL_BINDINGS_MOCK_BINDING_PROXY_EVENT_H

#include "platform/aas/mw/com/impl/bindings/mock_binding/sample_ptr.h"
#include "platform/aas/mw/com/impl/proxy_event_binding.h"
#include "platform/aas/mw/com/impl/proxy_event_binding_base.h"

#include <amp_assert.hpp>

#include <gmock/gmock.h>
#include <algorithm>
#include <iterator>
#include <memory>
#include <utility>
#include <vector>

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

class ProxyEventBase : public ProxyEventBindingBase
{
  public:
    ~ProxyEventBase() override = default;

    MOCK_METHOD(SubscriptionState, GetSubscriptionState, (), (const, noexcept, override));
    MOCK_METHOD(void, Unsubscribe, (), (noexcept, override));
    MOCK_METHOD(ResultBlank, Subscribe, (std::size_t), (noexcept, override));
    MOCK_METHOD(Result<std::size_t>, GetNumNewSamplesAvailable, (), (const, noexcept, override));
    MOCK_METHOD(ResultBlank, SetReceiveHandler, (BindingEventReceiveHandler), (noexcept, override));
    MOCK_METHOD(ResultBlank, UnsetReceiveHandler, (), (noexcept, override));
    MOCK_METHOD(amp::optional<std::uint16_t>, GetMaxSampleCount, (), (const, noexcept, override));
    MOCK_METHOD(BindingType, GetBindingType, (), (const, noexcept, override));
    MOCK_METHOD(void, NotifyServiceInstanceChangedAvailability, (bool, pid_t), (noexcept, override));
};

/// \brief Mock implementation for proxy event bindings.
///
/// This mock also includes a default behavior for GetNewSamples(): If there are fake samples added to an internal FIFO,
/// these samples are returned in order to the provided callback, unless stated otherwise with EXPECT_CALL.
///
/// \tparam SampleType Data type to be received by this proxy event.
template <typename SampleType>
class ProxyEvent : public ProxyEventBinding<SampleType>
{
  public:
    ProxyEvent() : ProxyEventBinding<SampleType>{}
    {
        ON_CALL(*this, GetNewSamples(::testing::_, ::testing::_))
            .WillByDefault(
                ::testing::WithArgs<0, 1>(::testing::Invoke(this, &ProxyEvent<SampleType>::GetNewFakeSamples)));
    }

    ~ProxyEvent() = default;

    MOCK_METHOD(SubscriptionState, GetSubscriptionState, (), (const, noexcept, override));
    MOCK_METHOD(void, Unsubscribe, (), (noexcept, override));
    MOCK_METHOD(ResultBlank, Subscribe, (std::size_t), (noexcept, override));
    MOCK_METHOD(Result<std::size_t>, GetNumNewSamplesAvailable, (), (const, noexcept, override));
    MOCK_METHOD(Result<std::size_t>,
                GetNewSamples,
                (typename ProxyEventBinding<SampleType>::Callback&&, TrackerGuardFactory&),
                (noexcept, override));
    MOCK_METHOD(ResultBlank, SetReceiveHandler, (BindingEventReceiveHandler), (noexcept, override));
    MOCK_METHOD(ResultBlank, UnsetReceiveHandler, (), (noexcept, override));
    MOCK_METHOD(amp::optional<std::uint16_t>, GetMaxSampleCount, (), (const, noexcept, override));
    MOCK_METHOD(BindingType, GetBindingType, (), (const, noexcept, override));
    MOCK_METHOD(void, NotifyServiceInstanceChangedAvailability, (bool, pid_t), (noexcept, override));

    /// \brief Add a sample to the internal queue of fake events.
    ///
    /// On a call to GetNewSamples(), these samples will be forwarded to the provided callable in case there is no
    /// EXPECT_CALL that overrides this behavior. This can be used to simulate received data on the proxy side.
    ///
    /// \param sample The data to be added to the incoming queue.
    void PushFakeSample(SampleType&& sample)
    {
        fake_samples_.push_back(std::make_unique<SampleType>(std::forward<SampleType>(sample)));
    }

  private:
    using FakeSamples = std::vector<SamplePtr<SampleType>>;
    FakeSamples fake_samples_{};

    Result<std::size_t> GetNewFakeSamples(typename ProxyEventBinding<SampleType>::Callback&& callable,
                                          TrackerGuardFactory& tracker);
};

template <typename SampleType>
Result<std::size_t> ProxyEvent<SampleType>::GetNewFakeSamples(
    typename ProxyEventBinding<SampleType>::Callback&& callable,
    TrackerGuardFactory& tracker)
{
    const std::size_t num_samples = std::min(fake_samples_.size(), tracker.GetNumAvailableGuards());
    const auto start_iter = fake_samples_.end() - static_cast<typename FakeSamples::difference_type>(num_samples);
    auto sample = std::make_move_iterator(start_iter);

    while (sample.base() != fake_samples_.end())
    {
        amp::optional<SampleReferenceGuard> guard = tracker.TakeGuard();
        AMP_ASSERT_PRD_MESSAGE(guard.has_value(), "No guards available.");
        SamplePtr<SampleType> ptr = *sample;
        impl::SamplePtr<SampleType> impl_ptr = this->MakeSamplePtr(std::move(ptr), std::move(*guard));

        const tracing::ITracingRuntime::TracePointDataId dummy_trace_point_data_id{0U};
        callable(std::move(impl_ptr), dummy_trace_point_data_id);
        ++sample;
    }

    fake_samples_.clear();
    return {num_samples};
}

}  // namespace mock_binding
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_BINDINGS_MOCK_BINDING_PROXY_EVENT_H
