// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#ifndef PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_NOTIFYEVENTHANDLER_H
#define PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_NOTIFYEVENTHANDLER_H

#include "platform/aas/lib/concurrency/thread_pool.h"
#include "platform/aas/mw/com/impl/binding_event_receive_handler.h"
#include "platform/aas/mw/com/impl/bindings/lola/element_fq_id.h"
#include "platform/aas/mw/com/impl/bindings/lola/messaging/handler_base.h"
#include "platform/aas/mw/com/impl/bindings/lola/messaging/i_message_passing_control.h"
#include "platform/aas/mw/com/impl/bindings/lola/messaging/i_message_passing_service.h"
#include "platform/aas/mw/com/impl/bindings/lola/messaging/messages/message_common.h"
#include "platform/aas/mw/com/impl/bindings/lola/messaging/messages/message_element_fq_id.h"
#include "platform/aas/mw/com/impl/configuration/quality_type.h"
#include "platform/aas/mw/com/message_passing/i_receiver.h"
#include "platform/aas/mw/com/message_passing/message.h"

#include <amp_callback.hpp>
#include <amp_stop_token.hpp>

#include <atomic>
#include <memory>
#include <set>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

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
/// \brief Handles event-notification functionality of MessagePassingFacade.
///
/// \details Functional aspects, which MessagePassingFacade provides are split into different composites/handlers. This
///          class implements the handling of event-notification functionality:
///          It gets (Un)RegisterEventNotification() calls from proxy instances
class NotifyEventHandler final : public HandlerBase
{
    using NotifyEventUpdateMessage = ElementFqIdMessage<MessageType::kNotifyEvent>;
    using RegisterEventNotificationMessage = ElementFqIdMessage<MessageType::kRegisterEventNotifier>;
    using UnregisterEventNotificationMessage = ElementFqIdMessage<MessageType::kUnregisterEventNotifier>;

  public:
    /// \brief ctor of NotifyEventHandler
    /// \param mp_control
    /// \param asil_b_capability shall ASIL_B supported beside QM or not?
    /// \param token stop_token to preempt async/long-running activities of this handler.
    NotifyEventHandler(IMessagePassingControl& mp_control, const bool asil_b_capability, const amp::stop_token& token);

    /// \brief Registers message received callbacks for messages handled by NotifyEventHandler at _receiver_
    /// \param asil_level asil level of given _receiver_
    /// \param receiver receiver, where to register
    void RegisterMessageReceivedCallbacks(const QualityType asil_level, message_passing::IReceiver& receiver) override;

    /// \brief Notify that event _event_id_ has been updated.
    ///
    /// \details This API is used by process local instances of LoLa skeleton-event in its implementation of ara::com
    /// event update functionality.
    ///
    /// \param asil_level needed/intended ASIL level. ASIL_B can only be used, if calling process is ASIL_B qualified
    ///                   and target provides service/event ASIL_B qualified.
    /// \param event_id identification of event to notify
    /// \param max_samples maximum number of event samples, which shall be used/buffered from caller perspective
    
    void NotifyEvent(const QualityType asil_level, const ElementFqId event_id);
    

    /// \brief Add event update notification callback
    /// \details This API is used by process local LoLa proxy-events.
    /// \param asil_level needed/intended ASIL level. ASIL_B can only be used, if calling process is ASIL_B qualified
    ///                   and target provides service/event ASIL_B qualified.
    /// \param event_id fully qualified event id, for which event notification shall be registered
    /// \param callback callback to be registered
    /// \param target_node_id node id (pid) of providing LoLa process.
    
    /* RegisterEventNotification implements IMessagePassingService::RegisterEventNotification */
    IMessagePassingService::HandlerRegistrationNoType RegisterEventNotification(
        
        const QualityType asil_level,
        const ElementFqId event_id,
        BindingEventReceiveHandler callback,
        const pid_t target_node_id);

    /// \brief Re-registers an event update notifications for event _event_id_ in case target_node_id is a remote pid.
    /// \details see see IMessagePassingService::ReregisterEventNotification
    void ReregisterEventNotification(QualityType asil_level, ElementFqId event_id, pid_t target_node_id);

    /// \brief Unregister an event update notification callback, which has been registered with
    ///        RegisterEventNotification()
    /// \param asil_level needed/intended ASIL level. ASIL_B can only be used, if calling process is ASIL_B qualified
    //                    and target provides service/event ASIL_B qualified.
    /// \param event_id fully qualified event id, for which event notification shall be unregistered
    /// \param registration_no registration handle, which has been returned by
    ///        RegisterEventNotification.
    /// \param target_node_id node id (pid) of event providing LoLa process.
    
    /* UnregisterEventNotification implements IMessagePassingService::UnregisterEventNotification */
    void UnregisterEventNotification(
        
        const QualityType asil_level,
        const ElementFqId event_id,
        const IMessagePassingService::HandlerRegistrationNoType registration_no,
        const pid_t target_node_id);

    void NotifyOutdatedNodeId(QualityType asil_level, pid_t outdated_node_id, pid_t target_node_id);

  private:
    bool SearchHandlerForNotification(const QualityType asil_level,
                                      const ElementFqId event_id,
                                      const IMessagePassingService::HandlerRegistrationNoType registration_no) noexcept;

    struct RegisteredNotificationHandler
    {
        BindingEventReceiveHandler handler;
        // 
        IMessagePassingService::HandlerRegistrationNoType register_no{};
    };

    /// \brief Counter for registered event receive notifications for the given (target) node.
    struct NodeCounter
    {
        // While true that pid_t is not a fixed width integer it is required by the POSIX standard here.
        // 
        pid_t node_id;
        std::uint16_t counter;
    };

    using EventUpdateNotifierMapType = std::unordered_map<ElementFqId, std::vector<RegisteredNotificationHandler>>;
    using EventUpdateNodeIdMapType = std::unordered_map<ElementFqId, std::set<pid_t>>;
    using EventUpdateRegistrationCountMapType = std::unordered_map<ElementFqId, NodeCounter>;
    /// \brief tmp buffer for copying ids under lock.
    /// \todo Make its size configurable?
    using NodeIdTmpBufferType = std::array<pid_t, 20>;

    struct EventNotificationControlData
    {
        /// \brief map holding per event_id a list of notification/receive handlers registered by local proxy-event
        ///        instances, which need to be called, when the event with given _event_id_ is updated.
        EventUpdateNotifierMapType event_update_handlers_;

        // 
        std::shared_mutex event_update_handlers_mutex_;

        /// \brief map holding per event_id a list of remote LoLa nodes, which need to be informed, when the event with
        ///        given _event_id_ is updated.
        /// \note This is the symmetric data structure to event_update_handlers_, in case the proxy-event registering
        ///       a receive handler is located in a different LoLa process.
        // 
        EventUpdateNodeIdMapType event_update_interested_nodes_;

        // 
        std::shared_mutex event_update_interested_nodes_mutex_;

        /// \brief map holding per event_id a node counter, how many local proxy-event instances have registered a
        ///       receive-handler for this event at the given node. This map only contains events provided by remote
        ///       LoLa processes.
        /// \note we maintain this data structure for performance reasons: We do NOT send for every
        ///       RegisterEventNotification() call for a "remote" event X by a local proxy-event-instance a message
        ///       to the given node redundantly! We rather do a smart (de)multiplexing here by counting the local
        ///       registrars! If the counter goes from 0 to 1, we send a RegisterNotificationMessage to the remote node
        ///       and we send an UnregisterNotificationMessage to the remote node, when the counter gets decremented to
        ///       0 again.
        // 
        EventUpdateRegistrationCountMapType event_update_remote_registrations_;

        // 
        std::shared_mutex event_update_remote_registrations_mutex_;

        // 
        std::atomic<IMessagePassingService::HandlerRegistrationNoType> cur_registration_no_{0U};

        /// \brief thread pool for processing local event update notification.
        /// \detail local update notification leads to a user provided receive handler callout, whose
        ///         runtime is unknown, so we decouple with worker threads.
        // 
        std::unique_ptr<bmw::concurrency::ThreadPool> thread_pool_;
    };

    /// \brief Registers event notification at a remote node.
    /// \param asil_level asil level of event, for which notification shall be registered.
    /// \param event_id full qualified event id
    /// \param target_node_id node id of the event provider
    void RegisterEventNotificationRemote(const QualityType asil_level,
                                         const ElementFqId event_id,
                                         const pid_t target_node_id);

    /// \brief Unregisters event notification from a remote node.
    /// \param asil_level asil level of event, for which notification shall be unregistered.
    /// \param event_id full qualified event id
    /// \param target_node_id node id of the event provider
    void UnregisterEventNotificationRemote(const QualityType asil_level,
                                           const ElementFqId event_id,
                                           const IMessagePassingService::HandlerRegistrationNoType registration_no,
                                           const pid_t target_node_id);

    /// \brief Notifies event update towards other LoLa processes interested in.
    /// \param asil_level asil level of updated event.
    /// \param event_id full qualified event id
    void NotifyEventRemote(const QualityType asil_level,
                           const ElementFqId event_id,
                           EventNotificationControlData& event_notification_ctrl);

    /// \brief Notifies all registered receive handlers (of local proxy events) about an event update.
    /// \param token
    /// \param asil_level
    /// \param event_id
    /// \return count of handlers, that have been called.
    /// \todo: type of return is deduced from max subscriber count, we do support. Right now master seems to be
    ///        EventSlotStatus::SubscriberCount. Include/use here could introduce some ugly dependencies...?
    std::uint32_t NotifyEventLocally(const amp::stop_token& token,
                                     const QualityType asil_level,
                                     const ElementFqId event_id);

    /// \brief internal handler method, when a notify-event message has been received on a receiver.
    /// \details It notifies process local LoLa proxy event instances, which have registered a notification callback for
    ///          the event_id contained in the message.
    ///          It is the analogue to the NotifyEvent() method, which gets called by local skeleton-event instances,
    ///          but gets triggered by skeleton-event-instances from remote LoLa processes.
    /// \param msg_payload payload of notify-event message
    /// \param asil_level asil
    /// level of provider (deduced from receiver instance, where message has been received) \param sender_node_id
    /// node_id of sender (process)
    void HandleNotifyEventMsg(const bmw::mw::com::message_passing::ShortMessagePayload msg_payload,
                              const QualityType asil_level,
                              const pid_t sender_node_id);

    /// \brief internal handler method, when a register-event-notification message has been received.
    /// \param msg_payload payload of register-event-notification message
    /// \param asil_level asil level of consumer (deduced from receiver instance, where message has been received)
    /// \param sender_node_id node_id of sender (process)
    void HandleRegisterNotificationMsg(const bmw::mw::com::message_passing::ShortMessagePayload msg_payload,
                                       const QualityType asil_level,
                                       const pid_t sender_node_id);

    /// \brief internal handler method, when an unregister-event-notification message has been received.
    /// \param msg_payload payload of unregister-event-notification message
    /// \param asil_level asil level of consumer (deduced from receiver instance, where message has been received)
    /// \param sender_node_id node_id of sender (process)
    void HandleUnregisterNotificationMsg(const bmw::mw::com::message_passing::ShortMessagePayload msg_payload,
                                         const QualityType asil_level,
                                         const pid_t sender_node_id);

    /// \brief internal handler method, when an outdated-node-id message has been received.
    /// \param msg_payload payload of outdated-node-id message
    /// \param asil_level asil level of sender (deduced from receiver instance, where message has been received)
    /// \param sender_node_id node_id of sender (process)
    void HandleOutdatedNodeIdMsg(const bmw::mw::com::message_passing::ShortMessagePayload msg_payload,
                                 const QualityType asil_level,
                                 const pid_t sender_node_id);

    void SendRegisterEventNotificationMessage(const QualityType asil_level,
                                              const ElementFqId event_id,
                                              const pid_t target_node_id) const;

    EventNotificationControlData control_data_qm_;
    EventNotificationControlData control_data_asil_;

    /// \brief stop token handed over from parent/facade used to preempt iteration over userland callouts.
    /// \details NotifyEventLocally() is either called from thread_pool owned by this class (see
    //           EventNotificationControlData.thread_pool_) if we have an event-update of a local event or by an
    //           execution context owned by the IReceiver instance, if we have an event-update of a remote event. In the
    //           former case we use the stop_token provided by EventNotificationControlData.thread_pool_. However, in
    //           the latter case we need a different token, where we use this handed over token.
    amp::stop_token token_;

    /// \brief ref to message passing control, which is used to retrieve node_id and get message-passing sender for
    ///        specific target nodes.
    impl::lola::IMessagePassingControl& mp_control_;

    /// \brief do we support ASIL-B comm in addition to QM default?
    const bool asil_b_capability_;
};

}  // namespace lola
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_NOTIFYEVENTHANDLER_H
