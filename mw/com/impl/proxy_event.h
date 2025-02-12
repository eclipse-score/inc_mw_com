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


#ifndef PLATFORM_AAS_MW_COM_IMPL_PROXYEVENT_H
#define PLATFORM_AAS_MW_COM_IMPL_PROXYEVENT_H

#include "platform/aas/mw/com/impl/instance_identifier.h"
#include "platform/aas/mw/com/impl/plumbing/proxy_event_binding_factory.h"
#include "platform/aas/mw/com/impl/proxy_base.h"
#include "platform/aas/mw/com/impl/proxy_event_base.h"
#include "platform/aas/mw/com/impl/proxy_event_binding.h"
#include "platform/aas/mw/com/impl/runtime.h"
#include "platform/aas/mw/com/impl/tracing/proxy_event_tracing.h"

#include "platform/aas/lib/result/result.h"
#include "platform/aas/mw/log/logging.h"

#include <amp_assert.hpp>
#include <amp_string_view.hpp>

#include <cstddef>
#include <memory>
#include <utility>

namespace bmw
{
namespace mw
{
namespace com
{
namespace impl
{

// False Positive: this is a normal forward declaration.
// Which is used to avoid cyclic dependency with proxy_field.h
// 
template <typename>
class ProxyField;

/// \brief This is the user-visible class of an event that is part of a proxy. It contains ProxyEvent functionality that
/// requires knowledge of the SampleType. All type agnostic functionality is stored in the base class, ProxyEventBase.
///
/// The class itself is a concrete type. However, it delegates all actions to an implementation
/// that is provided by the binding the proxy is operating on.
///
/// \tparam SampleDataType Type of data that is transferred by the event.
template <typename SampleDataType>
class ProxyEvent final : public ProxyEventBase
{
    template <typename T>
    // Design decission: This friend class provides a view on the internals of ProxyEvent.
    // This enables us to hide unncecessary internals from the enduser.
    // 
    friend class ProxyEventView;

    // Design decission: ProxyField uses composition pattern to reuse code from ProxyEvent. These two classes also have
    // shared private APIs which necessitates the use of the friend keyword.
    // 
    friend class ProxyField<SampleDataType>;

    // Empty struct that is used to make the second constructor only accessible to ProxyField (as it is a friend).
    struct PrivateConstructorEnabler
    {
    };

  public:
    using SampleType = SampleDataType;

    /// Constructor that allows to set the binding directly.
    ///
    /// This is used only used for testing.
    ///
    /// \param proxy_binding The binding that shall be associated with this proxy.
    ProxyEvent(ProxyBase& base,
               std::unique_ptr<ProxyEventBinding<SampleType>> proxy_binding,
               const amp::string_view event_name);

    /// Constructor that allows to set the binding directly.
    ///
    /// This is used by ProxyField to pass in a ProxyEventBinding created using the ProxyFieldBindingFactory.
    ///
    /// \param proxy_binding The binding that shall be associated with this proxy.
    ProxyEvent(ProxyBase& base,
               std::unique_ptr<ProxyEventBinding<SampleType>> proxy_binding,
               const amp::string_view event_name,
               PrivateConstructorEnabler);

    /// \brief Constructs a ProxyEvent by querying the base proxie's ProxyBinding for the respective ProxyEventBinding.
    ///
    /// \param base Proxy that contains this event
    /// \param event_name Event name of the event, taken from the AUTOSAR model
    /// \todo Remove unneeded parameter once we get these information from the configuration
    ProxyEvent(ProxyBase& base, const amp::string_view event_name);

    /// \brief Receive pending data from the event.
    ///
    /// The user needs to provide a callable that fulfills the following signature:
    /// void F(SamplePtr<const SampleType>) noexcept. This callback will be called for each sample
    /// that is available at the time of the call. Notice that the number of callback calls cannot
    /// exceed std::min(GetFreeSampleCount(), max_num_samples) times.
    ///
    /// \tparam F Callable with the signature void(SamplePtr<const SampleType>) noexcept
    /// \param receiver Callable with the appropriate signature. GetNewSamples will take ownership
    ///                 of this callable.
    /// \param max_num_samples Maximum number of samples to return via the given callable.
    /// \return Number of samples that were handed over to the callable or an error.
    template <typename F>
    Result<std::size_t> GetNewSamples(F&& receiver, std::size_t max_num_samples) noexcept;

  private:
    ProxyEventBinding<SampleType>* GetTypedEventBinding() const noexcept;
};

template <typename SampleType>
ProxyEvent<SampleType>::ProxyEvent(ProxyBase& base,
                                   std::unique_ptr<ProxyEventBinding<SampleType>> proxy_binding,
                                   const amp::string_view event_name)
    : ProxyEventBase{base, std::move(proxy_binding), event_name}
{
}

template <typename SampleType>
ProxyEvent<SampleType>::ProxyEvent(ProxyBase& base, const amp::string_view event_name)
    : ProxyEventBase{base, ProxyEventBindingFactory<SampleType>::Create(base, event_name), event_name}
{
    const ProxyBaseView proxy_base_view{base};
    const auto& instance_identifier = proxy_base_view.GetAssociatedHandleType().GetInstanceIdentifier();
    tracing_data_ = tracing::GenerateProxyTracingStructFromEventConfig(instance_identifier, event_name);
}

template <typename SampleType>
ProxyEvent<SampleType>::ProxyEvent(ProxyBase& base,
                                   std::unique_ptr<ProxyEventBinding<SampleType>> proxy_binding,
                                   const amp::string_view event_name,
                                   PrivateConstructorEnabler)
    : ProxyEventBase{base, std::move(proxy_binding), event_name}
{
    const ProxyBaseView proxy_base_view{base};
    const auto& instance_identifier = proxy_base_view.GetAssociatedHandleType().GetInstanceIdentifier();
    tracing_data_ = tracing::GenerateProxyTracingStructFromFieldConfig(instance_identifier, event_name);
}

template <typename SampleType>
template <typename F>
Result<std::size_t> ProxyEvent<SampleType>::GetNewSamples(F&& receiver, std::size_t max_num_samples) noexcept
{
    tracing::TraceGetNewSamples(tracing_data_, *binding_base_);

    auto guard_factory{tracker_->Allocate(max_num_samples)};
    if (guard_factory.GetNumAvailableGuards() == 0U)
    {
        bmw::mw::log::LogWarn("lola")
            << "Unable to emit new samples, no free sample slots for this subscription available.";
        return MakeUnexpected(ComErrc::kMaxSamplesReached);
    }

    auto tracing_receiver = tracing::CreateTracingGetNewSamplesCallback<SampleType, F>(
        tracing_data_, *binding_base_, std::forward<F>(receiver));

    const auto get_new_samples_result =
        GetTypedEventBinding()->GetNewSamples(std::move(tracing_receiver), guard_factory);
    if (!get_new_samples_result.has_value())
    {
        if (get_new_samples_result.error() == ComErrc::kNotSubscribed)
        {
            return get_new_samples_result;
        }
        else
        {
            return MakeUnexpected(ComErrc::kBindingFailure);
        }
    }
    return get_new_samples_result;
}

template <typename SampleType>
ProxyEventBinding<SampleType>* ProxyEvent<SampleType>::GetTypedEventBinding() const noexcept
{
    auto typed_binding = dynamic_cast<ProxyEventBinding<SampleType>*>(binding_base_.get());
    AMP_ASSERT_PRD_MESSAGE(typed_binding != nullptr, "Downcast to ProxyEventBinding failed!");
    return typed_binding;
}

template <typename SampleType>
class ProxyEventView
{
  public:
    explicit ProxyEventView(ProxyEvent<SampleType>& proxy_event) : proxy_event_{proxy_event} {}

    ProxyEventBinding<SampleType>* GetBinding() const noexcept { return proxy_event_.GetTypedEventBinding(); }

  private:
    ProxyEvent<SampleType>& proxy_event_;
};

}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_PROXYEVENT_H
