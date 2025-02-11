// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#ifndef PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_SUBSCRIPTION_STATE_MACHINE_H
#define PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_SUBSCRIPTION_STATE_MACHINE_H

#include "platform/aas/mw/com/impl/binding_event_receive_handler.h"
#include "platform/aas/mw/com/impl/bindings/lola/event_control.h"
#include "platform/aas/mw/com/impl/bindings/lola/event_subscription_control.h"
#include "platform/aas/mw/com/impl/bindings/lola/messaging/i_message_passing_service.h"
#include "platform/aas/mw/com/impl/bindings/lola/slot_collector.h"
#include "platform/aas/mw/com/impl/bindings/lola/subscription_helpers.h"
#include "platform/aas/mw/com/impl/bindings/lola/subscription_state_base.h"
#include "platform/aas/mw/com/impl/bindings/lola/subscription_state_machine_states.h"
#include "platform/aas/mw/com/impl/bindings/lola/transaction_log_id.h"
#include "platform/aas/mw/com/impl/bindings/lola/transaction_log_registration_guard.h"

#include "platform/aas/lib/result/result.h"

#include <amp_callback.hpp>
#include <amp_optional.hpp>
#include <amp_string_view.hpp>

#include <array>
#include <cstdint>
#include <memory>
#include <mutex>
#include <queue>
#include <utility>

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

class Proxy;
class SlotCollector;

// Forward declare states
class NotSubscribedState;
class SubscriptionPendingState;
class SubscribedState;

/// \brief State machine that manages subscriptions to a ProxyEvent.
///
/// The state machine handles the user facing calls (SubscribeEvent(), UnsubscribeEvent(), SetReceiveHandler() etc.)
/// as well as the callbacks triggered by the IMessagePassingService.
///
/// The state machine conforms to the Run-to-completion execution model, meaning that each event or state machine method
/// completes before another can be called. An "Event" is a public member function which is modelled by the state
/// machine UML and causes a transition within the state machine. A "State Machine Method" is a function which depends
/// on the state of the state machine, but is not modelled by the UML and does not cause transitions in the state
/// machine (e.g. SetReceiveHandler, UnsetReceiveHandler).
///
/// A diagram of the state machine can be found in aas/mw/com/design/events_fields/proxy_event_state_machine.uxf
class SubscriptionStateMachine : public std::enable_shared_from_this<SubscriptionStateMachine>
{
    friend NotSubscribedState;
    // 
    friend SubscriptionPendingState;
    // 
    friend SubscribedState;

  public:
    SubscriptionStateMachine(const QualityType quality_type,
                             const ElementFqId element_fq_id,
                             const pid_t event_source_pid,
                             EventControl& event_control,
                             const TransactionLogId& transaction_log_id) noexcept;

    SubscriptionStateMachine(SubscriptionStateMachine&&) noexcept = delete;
    SubscriptionStateMachine& operator=(SubscriptionStateMachine&&) noexcept = delete;
    SubscriptionStateMachine(const SubscriptionStateMachine&) noexcept = delete;
    SubscriptionStateMachine& operator=(const SubscriptionStateMachine&) noexcept = delete;
    virtual ~SubscriptionStateMachine() noexcept;

    SubscriptionStateMachineState GetCurrentState() const noexcept;

    // State Machine Events. These are modelled by the state machine UML and cause transitions between states. The
    // thread currently processing an event will block until all queued events are processed. All other calls will be
    // non-blocking.
    [[nodiscard]] ResultBlank SubscribeEvent(const std::size_t max_sample_count) noexcept;
    void UnsubscribeEvent() noexcept;
    void StopOfferEvent() noexcept;
    void ReOfferEvent(const pid_t new_event_source_pid) noexcept;

    // State Machine Methods. These are not modelled by the state machine UML and do not cause transitions between
    // states. Blocking calls which wait until
    void SetReceiveHandler(BindingEventReceiveHandler handler) noexcept;
    void UnsetReceiveHandler() noexcept;

    amp::optional<std::uint16_t> GetMaxSampleCount() const noexcept;

    /// \brief Getter which returns an optional SlotCollector lock-free as long as SubscribeEvent, UnsubscribeEvent and
    /// GetSlotCollectorLockFree are called single-threaded.
    ///
    /// The SlotCollector is created when we succesfully subscribe (i.e. transition to Subscribed state) and is
    /// destroyed when we unsubscribe (i.e. transition to Not Subscribed state). IMPORTANT: These getters can only be
    /// called if SubscribeEvent and UnsubscribeEvent are called single-threaded. If they're called multi-threaded, then
    /// creating / destroying / accessing the SlotCollector must be protected by a mutex.
    ///
    /// Since calls to a single ProxyEvent must be called single-threaded according to our AOUs, we can take advantage
    /// of this lock-free optimisation.
    amp::optional<SlotCollector>& GetSlotCollectorLockFree() noexcept;
    const amp::optional<SlotCollector>& GetSlotCollectorLockFree() const noexcept;
    amp::optional<TransactionLogSet::TransactionLogIndex> GetTransactionLogIndex() const noexcept;
    [[nodiscard]] const ElementFqId& GetElementFqId() const noexcept;

  private:
    SubscriptionStateMachineState GetCurrentStateNoLock() const noexcept;

    // Private member methods should be called under lock
    SubscriptionStateBase& GetCurrentEventState() noexcept;
    SubscriptionStateBase& GetCurrentEventState() const noexcept;
    void TransitionToState(const SubscriptionStateMachineState newState) noexcept;

    // State machine variables
    mutable std::mutex state_mutex_;
    std::array<std::unique_ptr<SubscriptionStateBase>,
               static_cast<std::uint8_t>(SubscriptionStateMachineState::STATE_COUNT)>
        states_;
    SubscriptionStateMachineState current_state_idx_;

    // Data used by states
    SubscriptionData subscription_data_;
    amp::optional<BindingEventReceiveHandler> event_receiver_handler_;
    EventReceiveHandlerManager event_receive_handler_manager_;
    EventControl& event_control_;
    bool provider_service_instance_is_available_;

    const TransactionLogId& transaction_log_id_;
    amp::optional<TransactionLogRegistrationGuard> transaction_log_registration_guard_;

    // used for logging purposes
    const ElementFqId element_fq_id_;
};

}  // namespace lola
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_SUBSCRIPTION_STATE_MACHINE_H
