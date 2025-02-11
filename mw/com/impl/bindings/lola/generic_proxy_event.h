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

#ifndef PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_GENERIC_PROXY_EVENT_H
#define PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_GENERIC_PROXY_EVENT_H

#include "platform/aas/mw/com/impl/bindings/lola/proxy_event_common.h"
#include "platform/aas/mw/com/impl/generic_proxy_event_binding.h"

#include <amp_string_view.hpp>

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

/// \brief Generic proxy event binding implementation for the Lola IPC binding.
///
/// All subscription operations are implemented in the separate class SubscriptionStateMachine and the associated
/// states. All type agnostic proxy event operations are dispatched to the class ProxyEventCommon. This class is the
/// generic analogue for a lola ProxyEvent
class GenericProxyEvent final : public GenericProxyEventBinding
{
    friend class GenericProxyEventAttorney;

  public:
    using typename GenericProxyEventBinding::Callback;

    GenericProxyEvent() = delete;
    /// Create a new instance that is bound to the specified ShmBindingInformation and ElementId.
    ///
    /// \param parent Parent proxy of the proxy event.
    /// \param element_fq_id The ID of the event inside the proxy type.
    /// \param event_name The name of the event inside the proxy type.
    GenericProxyEvent(Proxy& parent, const ElementFqId element_fq_id, const amp::string_view event_name);

    GenericProxyEvent(const GenericProxyEvent&) = delete;
    GenericProxyEvent(GenericProxyEvent&&) noexcept = delete;
    GenericProxyEvent& operator=(const GenericProxyEvent&) = delete;
    GenericProxyEvent& operator=(GenericProxyEvent&&) noexcept = delete;

    ~GenericProxyEvent() noexcept = default;

    ResultBlank Subscribe(const std::size_t max_sample_count) noexcept override;
    void Unsubscribe() noexcept override;

    SubscriptionState GetSubscriptionState() const noexcept override;
    Result<std::size_t> GetNumNewSamplesAvailable() const noexcept override;
    Result<std::size_t> GetNewSamples(Callback&& receiver, TrackerGuardFactory& tracker) noexcept override;
    std::size_t GetSampleSize() const noexcept override;
    bool HasSerializedFormat() const noexcept override;

    ResultBlank SetReceiveHandler(BindingEventReceiveHandler handler) noexcept override;
    ResultBlank UnsetReceiveHandler() noexcept override;
    pid_t GetEventSourcePid() const noexcept;
    ElementFqId GetElementFQId() const noexcept;
    amp::optional<std::uint16_t> GetMaxSampleCount() const noexcept override;
    BindingType GetBindingType() const noexcept override { return BindingType::kLoLa; }
    void NotifyServiceInstanceChangedAvailability(bool is_available, pid_t new_event_source_pid) noexcept override;

  private:
    Result<std::size_t> GetNewSamplesImpl(Callback&& receiver, TrackerGuardFactory& tracker) noexcept;
    Result<std::size_t> GetNumNewSamplesAvailableImpl() const noexcept;

    ProxyEventCommon proxy_event_common_;
};

}  // namespace lola
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_GENERIC_PROXY_EVENT_H
