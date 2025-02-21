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


///
/// @file
/// 
///
#include "platform/aas/mw/com/impl/proxy_event_base.h"

#include "platform/aas/language/safecpp/scoped_function/scope.h"
#include "platform/aas/mw/com/impl/binding_event_receive_handler.h"
#include "platform/aas/mw/com/impl/com_error.h"
#include "platform/aas/mw/com/impl/tracing/proxy_event_tracing.h"

#include "platform/aas/lib/result/result.h"
#include "platform/aas/mw/log/logging.h"

#include <amp_assert.hpp>

#include <exception>
#include <memory>
#include <utility>

namespace bmw::mw::com::impl
{

/// \brief Helper class which registers the ProxyEventBase with its parent proxy and unregisters on destruction
///
/// Since ProxyBase is moveable, we must ensure that this class does not store a reference or pointer to it. As if the
/// Proxy is moved, then the pointer or reference would be invalidated. However, the ProxyBinding is not moveable, so
/// storing a pointer to the ProxyBinding is safe.
class EventBindingRegistrationGuard final
{
  public:
    EventBindingRegistrationGuard(ProxyBase& proxy_base,
                                  ProxyEventBindingBase* proxy_event_binding_base,
                                  amp::string_view event_name) noexcept
        : proxy_binding_{ProxyBaseView{proxy_base}.GetBinding()},
          proxy_event_binding_base_{proxy_event_binding_base},
          event_name_{event_name}
    {
        if (proxy_event_binding_base_ == nullptr)
        {
            ProxyBaseView{proxy_base}.MarkServiceElementBindingInvalid();
            return;
        }
        if (proxy_binding_ != nullptr)
        {
            proxy_binding_->RegisterEventBinding(event_name, *proxy_event_binding_base);
        }
    }

    ~EventBindingRegistrationGuard() noexcept
    {
        if (proxy_binding_ != nullptr && proxy_event_binding_base_ != nullptr)
        {
            proxy_binding_->UnregisterEventBinding(event_name_);
        }
    }

    EventBindingRegistrationGuard(const EventBindingRegistrationGuard&) = delete;
    EventBindingRegistrationGuard& operator=(const EventBindingRegistrationGuard&) = delete;
    EventBindingRegistrationGuard(EventBindingRegistrationGuard&& other) = delete;
    EventBindingRegistrationGuard& operator=(EventBindingRegistrationGuard&& other) = delete;

  private:
    ProxyBinding* proxy_binding_;
    ProxyEventBindingBase* proxy_event_binding_base_;
    amp::string_view event_name_;
};

ProxyEventBase::ProxyEventBase(ProxyBase& proxy_base,
                               std::unique_ptr<ProxyEventBindingBase> proxy_event_binding,
                               amp::string_view event_name) noexcept
    : binding_base_{std::move(proxy_event_binding)},
      tracker_{std::make_unique<SampleReferenceTracker>()},
      tracing_data_{},
      event_binding_registration_guard_{
          std::make_unique<EventBindingRegistrationGuard>(proxy_base, binding_base_.get(), event_name)},
      receive_handler_scope_{}
{
}

ProxyEventBase::~ProxyEventBase() noexcept
{
    // If the ProxyEventBase has been moved, then tracker_ will be a nullptr
    if (tracker_ != nullptr && tracker_->IsUsed())
    {
        bmw::mw::log::LogFatal("lola")
            << "Proxy event instance destroyed while still holding SamplePtr instances, terminating.";
        std::terminate();
    }
}

ProxyEventBase::ProxyEventBase(ProxyEventBase&&) noexcept = default;
ProxyEventBase& ProxyEventBase::operator=(ProxyEventBase&&) noexcept = default;

ResultBlank ProxyEventBase::Subscribe(const std::size_t max_sample_count) noexcept
{
    tracing::TraceSubscribe(tracing_data_, *binding_base_, max_sample_count);

    const auto current_state = GetSubscriptionState();
    if (current_state == SubscriptionState::kNotSubscribed)
    {
        tracker_->Reset(max_sample_count);
        const auto subscribe_result = binding_base_->Subscribe(max_sample_count);
        if (!subscribe_result.has_value())
        {
            return MakeUnexpected(ComErrc::kBindingFailure);
        }
    }
    else if ((current_state == SubscriptionState::kSubscribed) ||
             (current_state == SubscriptionState::kSubscriptionPending))
    {
        const auto current_max_sample_count = binding_base_->GetMaxSampleCount();
        AMP_ASSERT_MESSAGE(current_max_sample_count.has_value(), "Current MaxSampleCount must be set when subscribed.");
        if (max_sample_count != current_max_sample_count.value())
        {
            return MakeUnexpected(ComErrc::kMaxSampleCountNotRealizable);
        }
    }
    return {};
}

void ProxyEventBase::Unsubscribe() noexcept
{
    tracing::TraceUnsubscribe(tracing_data_, *binding_base_);

    if (GetSubscriptionState() != SubscriptionState::kNotSubscribed)
    {
        binding_base_->Unsubscribe();
        if (tracker_->IsUsed())
        {
            bmw::mw::log::LogFatal("lola")
                << "Called unsubscribe while still holding SamplePtr instances, terminating.";
            std::terminate();
        }
    }
}

Result<std::size_t> ProxyEventBase::GetNumNewSamplesAvailable() const noexcept
{
    const auto get_num_new_samples_available_result = binding_base_->GetNumNewSamplesAvailable();
    if (!get_num_new_samples_available_result.has_value())
    {
        if (get_num_new_samples_available_result.error() == ComErrc::kNotSubscribed)
        {
            return get_num_new_samples_available_result;
        }
        else
        {
            return MakeUnexpected(ComErrc::kBindingFailure);
        }
    }
    return get_num_new_samples_available_result;
}

ResultBlank ProxyEventBase::SetReceiveHandler(EventReceiveHandler handler) noexcept
{
    tracing::TraceSetReceiveHandler(tracing_data_, *binding_base_);
    auto tracing_handler = tracing::CreateTracingReceiveHandler(tracing_data_, *binding_base_, std::move(handler));

    // Create a new scope for the provided callable. This will also expire the scope of any previously registered
    // callable.
    receive_handler_scope_ = safecpp::Scope<>{};
    BindingEventReceiveHandler scoped_tracing_handler{receive_handler_scope_, std::move(tracing_handler)};
    const auto set_receive_handler_result = binding_base_->SetReceiveHandler(std::move(scoped_tracing_handler));
    if (!set_receive_handler_result.has_value())
    {
        return MakeUnexpected(ComErrc::kSetHandlerNotSet);
    }
    return {};
}

ResultBlank ProxyEventBase::UnsetReceiveHandler() noexcept
{
    tracing::TraceUnsetReceiveHandler(tracing_data_, *binding_base_);

    receive_handler_scope_.Expire();

    const auto unset_receive_handler_result = binding_base_->UnsetReceiveHandler();
    if (!unset_receive_handler_result.has_value())
    {
        return MakeUnexpected(ComErrc::kUnsetFailure);
    }
    return {};
}

}  // namespace bmw::mw::com::impl
