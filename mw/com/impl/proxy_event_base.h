// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



///
/// @file
/// 
///

#ifndef PLATFORM_AAS_MW_COM_IMPL_PROXY_EVENT_BASE_H
#define PLATFORM_AAS_MW_COM_IMPL_PROXY_EVENT_BASE_H

#include "platform/aas/mw/com/impl/event_receive_handler.h"
#include "platform/aas/mw/com/impl/proxy_base.h"
#include "platform/aas/mw/com/impl/proxy_event_binding_base.h"
#include "platform/aas/mw/com/impl/sample_reference_tracker.h"
#include "platform/aas/mw/com/impl/subscription_state.h"
#include "platform/aas/mw/com/impl/tracing/proxy_event_tracing_data.h"

#include "platform/aas/language/safecpp/scoped_function/scope.h"
#include "platform/aas/lib/result/result.h"

#include <cstddef>
#include <memory>

namespace bmw::mw::com::impl
{

class EventBindingRegistrationGuard;

/// \brief This is the user-visible class of an event that is part of a proxy. It contains ProxyEvent functionality that
/// is agnostic of the data type that is transferred by the event.
///
/// The class itself is a concrete type. However, it delegates all actions to an implementation
/// that is provided by the binding the proxy is operating on.
class ProxyEventBase
{
    friend class ProxyEventBaseAttorney;

  public:
    ProxyEventBase(ProxyBase& proxy_base,
                   std::unique_ptr<ProxyEventBindingBase> proxy_event_binding,
                   amp::string_view event_name) noexcept;

    /// \brief A ProxyEventBase shall not be copyable
    ProxyEventBase(const ProxyEventBase&) = delete;
    ProxyEventBase& operator=(const ProxyEventBase&) = delete;

    /// \brief A ProxyEventBase shall be moveable
    ProxyEventBase(ProxyEventBase&&) noexcept;
    // 
    // 
    ProxyEventBase& operator=(ProxyEventBase&&) noexcept;

    virtual ~ProxyEventBase() noexcept;

    /// Subscribe to the event.
    ///
    /// This will initialize the event so that event data can be received once it arrives.
    ///
    /// \param max_sample_count Specify the maximum number of concurrent samples that this event shall
    ///                         be able to offer to the using application.
    /// \return On failure, returns an error code.
    ResultBlank Subscribe(const std::size_t max_sample_count) noexcept;

    /// \brief Get the subscription state of this event.
    ///
    /// This method can always be called regardless of the state of the event.
    ///
    /// \return Subscription state of the event.
    SubscriptionState GetSubscriptionState() const noexcept { return binding_base_->GetSubscriptionState(); }

    /// \brief End subscription to an event and release needed resources.
    ///
    /// It is illegal to call this method while data is still held by the application in the form of SamplePtr. Doing so
    /// will result in undefined behavior.
    ///
    /// After a call to this method, the event behaves as if it had just been constructed.
    void Unsubscribe() noexcept;

    /// \brief Get the number of samples that can still be received by the user of this event.
    ///
    /// If this returns 0, the user first has to drop at least one SamplePtr before it is possible to receive data via
    /// GetNewSamples again. If there is no subscription for this event, the returned value is unspecified.
    ///
    /// \return Number of samples that can still be received.
    std::size_t GetFreeSampleCount() const noexcept { return tracker_->GetNumAvailableSamples(); }

    /// \brief Returns the number of new samples a call to GetNewSamples() (given parameter max_num_samples
    /// doesn't restrict it) would currently provide.
    ///
    /// \details This is a proprietary extension to the official ara::com API. It is useful in resource sensitive
    ///          setups, where the user wants to work in polling mode only without registered async receive-handlers.
    ///          For further details see //platform/aas/mw/com/design/extensions/README.md.
    ///
    /// \return Either 0 if no new samples are available (and GetNewSamples() wouldn't return any) or N, where 1 <= N <=
    /// actual new samples. I.e. an implementation is allowed to report a lower number than actual new samples, which
    /// would be provided by a call to GetNewSamples().
    Result<std::size_t> GetNumNewSamplesAvailable() const noexcept;

    ResultBlank SetReceiveHandler(EventReceiveHandler handler) noexcept;

    ResultBlank UnsetReceiveHandler() noexcept;

    bool IsBindingValid() const noexcept { return binding_base_ != nullptr; }

  protected:
    std::unique_ptr<ProxyEventBindingBase> binding_base_;
    std::unique_ptr<SampleReferenceTracker> tracker_;
    tracing::ProxyEventTracingData tracing_data_;
    std::unique_ptr<EventBindingRegistrationGuard> event_binding_registration_guard_;

  private:
    safecpp::Scope<> receive_handler_scope_;
};

}  // namespace bmw::mw::com::impl

#endif  // PLATFORM_AAS_MW_COM_IMPL_PROXY_EVENT_BASE_H
