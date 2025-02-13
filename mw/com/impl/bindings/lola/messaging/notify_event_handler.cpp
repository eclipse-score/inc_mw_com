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


#include "platform/aas/mw/com/impl/bindings/lola/messaging/notify_event_handler.h"

#include "platform/aas/lib/os/errno_logging.h"
#include "platform/aas/mw/com/impl/bindings/lola/messaging/messages/message_element_fq_id.h"
#include "platform/aas/mw/com/impl/bindings/lola/messaging/messages/message_outdated_nodeid.h"
#include "platform/aas/mw/com/impl/bindings/lola/messaging/thread_abstraction.h"

#include "platform/aas/mw/log/logging.h"

#include <amp_assert.hpp>

#include <algorithm>
#include <array>
#include <iterator>
#include <memory>
#include <thread>
#include <utility>

bmw::mw::com::impl::lola::NotifyEventHandler::NotifyEventHandler(
    bmw::mw::com::impl::lola::IMessagePassingControl& mp_control,
    const bool asil_b_capability,
    const amp::stop_token& token)
    : HandlerBase{}, token_{token}, mp_control_{mp_control}, asil_b_capability_{asil_b_capability}
{
    auto hw_conc = ThreadHWConcurrency::hardware_concurrency();
    if (hw_conc == 0U)
    {
        hw_conc = 2U;  // we fall back to 2 threads, if we can't read out hw_conc.
    }
    
    control_data_qm_.thread_pool_ =
        std::make_unique<bmw::concurrency::ThreadPool>(hw_conc, "mw::com NotifyEventHandler QM");
    if (asil_b_capability)
    {
        control_data_asil_.thread_pool_ =
            std::make_unique<bmw::concurrency::ThreadPool>(hw_conc, "mw::com NotifyEventHandler ASIL-B");
    }
    
}

void bmw::mw::com::impl::lola::NotifyEventHandler::RegisterMessageReceivedCallbacks(
    const QualityType asil_level,
    bmw::mw::com::message_passing::IReceiver& receiver)
{
    AMP_ASSERT_PRD_MESSAGE(
        (asil_level == QualityType::kASIL_QM) || ((asil_level == QualityType::kASIL_B) && asil_b_capability_),
        "Invalid asil level.");

    // Note, that it's safe here to register a callback at the receiver capturing "this" as the lifetime of the
    // handler is longer/at least as long as the lifetime of this receiver instance:
    // Both - receiver and NotifyEventHandler are members of the enclosing MessagePassingFacade instance and
    // correct destruction order will be cared of.
    receiver.Register(
        static_cast<message_passing::MessageId>(MessageType::kRegisterEventNotifier),
        amp::callback<void(const message_passing::ShortMessagePayload, const pid_t)>(
            [this, asil_level](const message_passing::ShortMessagePayload payload, const pid_t sender_pid) {
                this->HandleRegisterNotificationMsg(payload, asil_level, sender_pid);
            }));
    receiver.Register(
        static_cast<message_passing::MessageId>(MessageType::kUnregisterEventNotifier),
        amp::callback<void(const message_passing::ShortMessagePayload, const pid_t)>(
            [this, asil_level](const message_passing::ShortMessagePayload payload, const pid_t sender_pid) {
                this->HandleUnregisterNotificationMsg(payload, asil_level, sender_pid);
            }));
    receiver.Register(
        static_cast<message_passing::MessageId>(MessageType::kNotifyEvent),
        amp::callback<void(const message_passing::ShortMessagePayload, const pid_t)>(
            [this, asil_level](const message_passing::ShortMessagePayload payload, const pid_t sender_pid) {
                this->HandleNotifyEventMsg(payload, asil_level, sender_pid);
            }));
    receiver.Register(
        static_cast<message_passing::MessageId>(MessageType::kOutdatedNodeId),
        amp::callback<void(const message_passing::ShortMessagePayload, const pid_t)>(
            [this, asil_level](const message_passing::ShortMessagePayload payload, const pid_t sender_pid) {
                this->HandleOutdatedNodeIdMsg(payload, asil_level, sender_pid);
            }));
}


void bmw::mw::com::impl::lola::NotifyEventHandler::NotifyEvent(const QualityType asil_level, const ElementFqId event_id)

{
    AMP_ASSERT_PRD_MESSAGE(
        (asil_level == QualityType::kASIL_QM) || ((asil_level == QualityType::kASIL_B) && asil_b_capability_),
        "Invalid asil level.");
    auto& control_data = asil_level == QualityType::kASIL_QM ? control_data_qm_ : control_data_asil_;

    // first we forward notification of event update to other LoLa processes, which are interested in this notification.
    // we do this first as message-sending is done synchronous/within the calling thread as it has "short"/deterministic
    // runtime.
    NotifyEventRemote(asil_level, event_id, control_data);

    // Notification of local proxy_events/user receive handlers is decoupled via worker-threads, as user level receive
    // handlers may have an unknown/non-deterministic long runtime.
    
    std::shared_lock<std::shared_mutex> read_lock(control_data.event_update_handlers_mutex_);
    
    auto search = control_data.event_update_handlers_.find(event_id);
    if ((search != control_data.event_update_handlers_.end()) && (search->second.empty() == false))
    {
        read_lock.unlock();
        control_data.thread_pool_->Post(
            [this](const amp::stop_token& token, const QualityType asilLevel, const ElementFqId element_id) {
                // ignoring the result (number of actually notified local proxy-events),
                // as we don't have any expectation, how many are there.
                amp::ignore = this->NotifyEventLocally(token, asilLevel, element_id);
            },
            asil_level,
            event_id);
    }
}


/* RegisterEventNotification implements IMessagePassingService::RegisterEventNotification */
bmw::mw::com::impl::lola::IMessagePassingService::HandlerRegistrationNoType
bmw::mw::com::impl::lola::NotifyEventHandler::RegisterEventNotification(
    
    const QualityType asil_level,
    const ElementFqId event_id,
    
    BindingEventReceiveHandler callback,
    
    const pid_t target_node_id)

{
    AMP_ASSERT_PRD_MESSAGE(
        (asil_level == QualityType::kASIL_QM) || ((asil_level == QualityType::kASIL_B) && asil_b_capability_),
        "Invalid asil level.");
    auto& control_data = asil_level == QualityType::kASIL_QM ? control_data_qm_ : control_data_asil_;

    std::unique_lock<std::shared_mutex> write_lock(control_data.event_update_handlers_mutex_);

    // The rule against increment and decriment operators mixed with others on the same line sems to be more applicable
    // to arithmetic operators. Here the only other operator is the member access operator (i.e. the dot operator) and
    // it in no way confusing to combine it with the increment operator.
    // This rule is also absent from the new MISRA 2023 standart, thus it is in general reasonable to deviate from it.
    // 
    const IMessagePassingService::HandlerRegistrationNoType registration_no = control_data.cur_registration_no_++;
    
    RegisteredNotificationHandler newHandler{std::move(callback), registration_no};
    
    auto search = control_data.event_update_handlers_.find(event_id);
    if (search != control_data.event_update_handlers_.end())
    {
        search->second.push_back(std::move(newHandler));
    }
    else
    {
        auto result =
            control_data.event_update_handlers_.emplace(event_id, std::vector<RegisteredNotificationHandler>{});
        result.first->second.push_back(std::move(newHandler));
    }
    write_lock.unlock();

    if (target_node_id != mp_control_.GetNodeIdentifier())
    {
        RegisterEventNotificationRemote(asil_level, event_id, target_node_id);
    }

    return registration_no;
}

void bmw::mw::com::impl::lola::NotifyEventHandler::ReregisterEventNotification(
    bmw::mw::com::impl::QualityType asil_level,
    bmw::mw::com::impl::lola::ElementFqId event_id,
    pid_t target_node_id)
{
    AMP_ASSERT_PRD_MESSAGE(
        (asil_level == QualityType::kASIL_QM) || ((asil_level == QualityType::kASIL_B) && asil_b_capability_),
        "Invalid asil level.");
    auto& control_data = asil_level == QualityType::kASIL_QM ? control_data_qm_ : control_data_asil_;

    std::shared_lock<std::shared_mutex> read_lock(control_data.event_update_handlers_mutex_);
    auto search = control_data.event_update_handlers_.find(event_id);
    if (search == control_data.event_update_handlers_.end())
    {
        read_lock.unlock();
        // no registered handler for given event_id -> log as error
        bmw::mw::log::LogError("lola") << "NotifyEventHandler: ReregisterEventNotification called for event_id"
                                       << event_id.ToString() << ", which had not yet been registered!";
        return;
    }
    read_lock.unlock();

    // we only do re-register activity, if it is a remote node
    const auto is_target_remote_node = (target_node_id != mp_control_.GetNodeIdentifier());
    if (is_target_remote_node)
    {
        std::unique_lock<std::shared_mutex> remote_reg_write_lock(
            control_data.event_update_remote_registrations_mutex_);
        auto registration_count = control_data.event_update_remote_registrations_.find(event_id);
        if (registration_count == control_data.event_update_remote_registrations_.end())
        {
            remote_reg_write_lock.unlock();
            bmw::mw::log::LogError("lola")
                << "NotifyEventHandler: ReregisterEventNotification called with ASIL level " << ToString(asil_level)
                << " for a remote event " << event_id.ToString() << " without current remote registration!";
            return;
        }
        if (registration_count->second.node_id == target_node_id)
        {
            // we aren't the 1st proxy to re-register. Another proxy already re-registered the event with the new remote
            // pid.

            // The rule against increment and decriment operators mixed with others on the same line sems to be more
            // applicable to arithmetic operators. Here the only other operator is the member access operator (i.e. the
            // dot operator) and it in no way confusing to combine it with the increment operator. This rule is also
            // absent from the new MISRA 2023 standart, thus it is in general reasonable to deviate from it.
            // 
            registration_count->second.counter++;
            remote_reg_write_lock.unlock();
        }
        else
        {
            // we are the 1st proxy to re-register.
            registration_count->second.node_id = target_node_id;
            registration_count->second.counter = 1U;
            remote_reg_write_lock.unlock();
            SendRegisterEventNotificationMessage(asil_level, event_id, target_node_id);
        }
    }
}

void bmw::mw::com::impl::lola::NotifyEventHandler::RegisterEventNotificationRemote(const QualityType asil_level,
                                                                                   const ElementFqId event_id,
                                                                                   const pid_t target_node_id)
{
    auto& control_data = asil_level == QualityType::kASIL_QM ? control_data_qm_ : control_data_asil_;
    std::unique_lock<std::shared_mutex> remote_reg_write_lock(control_data.event_update_remote_registrations_mutex_);
    const auto registration_count_inserted =
        control_data.event_update_remote_registrations_.emplace(std::piecewise_construct,
                                                                std::forward_as_tuple(event_id),
                                                                std::forward_as_tuple(NodeCounter{target_node_id, 1U}));
    if (registration_count_inserted.second == false)
    {
        if (registration_count_inserted.first->second.node_id != target_node_id)
        {
            bmw::mw::log::LogError("lola")
                << "NotifyEventHandler: RegisterEventNotificationRemote called for event" << event_id.ToString()
                << "and node_id" << target_node_id << "although event is "
                << " currently located at node" << registration_count_inserted.first->second.node_id;
            registration_count_inserted.first->second.node_id = target_node_id;
            registration_count_inserted.first->second.counter = 1U;
        }
        else
        {
            // The rule against increment and decriment operators mixed with others on the same line sems to be more
            // applicable to arithmetic operators. Here the only other operator is the member access operator (i.e. the
            // dot operator) and it in no way confusing to combine it with the increment operator. This rule is also
            // absent from the new MISRA 2023 standart, thus it is in general reasonable to deviate from it.
            // 
            registration_count_inserted.first->second.counter++;
        }
    }
    const std::uint16_t reg_counter{registration_count_inserted.first->second.counter};
    remote_reg_write_lock.unlock();
    // only if the counter of registrations switched to 1, we send a message to the remote node.
    if (reg_counter == 1U)
    {
        SendRegisterEventNotificationMessage(asil_level, event_id, target_node_id);
    }
}

void bmw::mw::com::impl::lola::NotifyEventHandler::SendRegisterEventNotificationMessage(
    const QualityType asil_level,
    const ElementFqId event_id,
    const pid_t target_node_id) const
{
    const auto message = RegisterEventNotificationMessage{event_id, mp_control_.GetNodeIdentifier()};
    auto sender = mp_control_.GetMessagePassingSender(asil_level, target_node_id);
    AMP_ASSERT_PRD_MESSAGE(sender != nullptr,
                           "sender is  a nullpointer. This should not have happend. GetMessagePassingSender should "
                           "allways return a valid shared pointer.");
    const auto result = sender->Send(message.SerializeToShortMessage());
    if (!result.has_value())
    {
        bmw::mw::log::LogError("lola") << "NotifyEventHandler: Sending RegisterEventNotificationMessage to node_id "
                                       << target_node_id << " with asil_level " << ToString(asil_level)
                                       << " failed with error: " << result.error();
    }
}


/* UnregisterEventNotification implements IMessagePassingService::UnregisterEventNotification */
void bmw::mw::com::impl::lola::NotifyEventHandler::UnregisterEventNotification(
    
    const QualityType asil_level,
    const ElementFqId event_id,
    const IMessagePassingService::HandlerRegistrationNoType registration_no,
    const pid_t target_node_id)
{

    bool found = SearchHandlerForNotification(asil_level, event_id, registration_no);

    if (!found)
    {
        
        bmw::mw::log::LogWarn("lola")
            << "NotifyEventHandler: Couldn't find handler for UnregisterEventNotification call with ASIL level "
            << ToString(asil_level) << " and register_no " << registration_no;
        
        // since we didn't find a handler with the given registration_no, we directly return as we have to assume,
        // that this simply is a bogus/wrong unregister call from application level.
        return;
    }

    if (target_node_id != mp_control_.GetNodeIdentifier())
    {
        UnregisterEventNotificationRemote(asil_level, event_id, registration_no, target_node_id);
    }
}

bool bmw::mw::com::impl::lola::NotifyEventHandler::SearchHandlerForNotification(
    const QualityType asil_level,
    const ElementFqId event_id,
    const IMessagePassingService::HandlerRegistrationNoType registration_no) noexcept
{
    AMP_ASSERT_PRD_MESSAGE(
        (asil_level == QualityType::kASIL_QM) || ((asil_level == QualityType::kASIL_B) && asil_b_capability_),
        "Invalid asil level.");
    auto& control_data = asil_level == QualityType::kASIL_QM ? control_data_qm_ : control_data_asil_;
    bool found{false};

    std::unique_lock<std::shared_mutex> write_lock(control_data.event_update_handlers_mutex_);
    auto search = control_data.event_update_handlers_.find(event_id);
    if (search != control_data.event_update_handlers_.end())
    {
        // we can do a binary search here, as the registered handlers in this vector are inherently sorted as we emplace
        // always back with monotonically increasing registration number.
        auto result = std::lower_bound(search->second.begin(),
                                       search->second.end(),
                                       registration_no,
                                       [](const RegisteredNotificationHandler& regHandler,
                                          const IMessagePassingService::HandlerRegistrationNoType value) -> bool {
                                           return regHandler.register_no < value;
                                       });
        if ((result != search->second.end()) && (result->register_no == registration_no))
        {
            amp::ignore = search->second.erase(result);
            found = true;
        }
    }
    write_lock.unlock();
    return found;
}

void bmw::mw::com::impl::lola::NotifyEventHandler::NotifyOutdatedNodeId(QualityType asil_level,
                                                                        pid_t outdated_node_id,
                                                                        pid_t target_node_id)
{
    AMP_ASSERT_PRD_MESSAGE(
        (asil_level == QualityType::kASIL_QM) || ((asil_level == QualityType::kASIL_B) && asil_b_capability_),
        "Invalid asil level.");
    auto message = OutdatedNodeIdMessage{outdated_node_id, target_node_id};

    auto sender = mp_control_.GetMessagePassingSender(asil_level, target_node_id);
    AMP_ASSERT_PRD_MESSAGE(sender != nullptr,
                           "sender is  a nullpointer. This should not have happend. GetMessagePassingSender should "
                           "allways return a valid shared pointer.");
    const auto result = sender->Send(SerializeToShortMessage(message));
    if (!result.has_value())
    {
        bmw::mw::log::LogError("lola") << "NotifyEventHandler: Sending OutdatedNodeIdMessage to node_id "
                                       << target_node_id << " with asil_level " << ToString(asil_level)
                                       << " failed with error: " << result.error();
    }
}

void bmw::mw::com::impl::lola::NotifyEventHandler::UnregisterEventNotificationRemote(
    const QualityType asil_level,
    const ElementFqId event_id,
    const IMessagePassingService::HandlerRegistrationNoType registration_no,
    const pid_t target_node_id)
{
    bool send_message{false};
    auto& control_data = asil_level == QualityType::kASIL_QM ? control_data_qm_ : control_data_asil_;
    std::unique_lock<std::shared_mutex> remote_reg_write_lock(control_data.event_update_remote_registrations_mutex_);
    auto registration_count = control_data.event_update_remote_registrations_.find(event_id);
    if (registration_count == control_data.event_update_remote_registrations_.end())
    {
        remote_reg_write_lock.unlock();
        bmw::mw::log::LogError("lola") << "NotifyEventHandler: UnregisterEventNotification called with ASIL level "
                                       << ToString(asil_level) << " and register_no " << registration_no
                                       << " for a remote event " << event_id.ToString()
                                       << " without current remote registration!";
        return;
    }
    else
    {
        AMP_ASSERT_PRD_MESSAGE(
            registration_count->second.counter > 0U,
            "NotifyEventHandler: UnregisterEventNotification trying to decrement counter, which is already 0!");
        if (registration_count->second.node_id != target_node_id)
        {
            remote_reg_write_lock.unlock();
            bmw::mw::log::LogError("lola")
                << "NotifyEventHandler: UnregisterEventNotification called with ASIL level " << ToString(asil_level)
                << " and register_no " << registration_no << " for a remote event " << event_id.ToString()
                << " for target_node_id" << target_node_id
                << ", which is not the node_id, by which this event is currently provided:"
                << registration_count->second.node_id;
            return;
        }

        // The rule against increment and decriment operators mixed with others on the same line sems to be more
        // applicable to arithmetic operators. Here the only other operator is the member access operator (i.e. the dot
        // operator) and it in no way confusing to combine it with the decriment operator. This rule is also absent from
        // the new MISRA 2023 standart, thus it is in general reasonable to deviate from it.
        // 
        registration_count->second.counter--;
        // only if the counter of registrations switched back to 0, we send a message to the remote node.
        if (registration_count->second.counter == 0U)
        {
            send_message = true;
            amp::ignore = control_data.event_update_remote_registrations_.erase(registration_count);
        }
        remote_reg_write_lock.unlock();
    }

    if (send_message)
    {
        const auto message = UnregisterEventNotificationMessage{event_id, mp_control_.GetNodeIdentifier()};
        auto sender = mp_control_.GetMessagePassingSender(asil_level, target_node_id);
        AMP_ASSERT_PRD_MESSAGE(sender != nullptr,
                               "sender is  a nullpointer. This should not have happend. GetMessagePassingSender should "
                               "allways return a valid shared pointer.");
        const auto result = sender->Send(message.SerializeToShortMessage());
        if (!result.has_value())
        {
            bmw::mw::log::LogError("lola")
                << "NotifyEventHandler: Sending UnregisterEventNotificationMessage to node_id " << target_node_id
                << " with asil_level " << ToString(asil_level) << " failed with error: " << result.error();
        }
    }
}

void bmw::mw::com::impl::lola::NotifyEventHandler::NotifyEventRemote(
    const QualityType asil_level,
    const ElementFqId event_id,
    NotifyEventHandler::EventNotificationControlData& event_notification_ctrl)
{
    NodeIdTmpBufferType nodeIdentifiersTmp;
    pid_t start_node_id{0};
    const NotifyEventUpdateMessage message{event_id, mp_control_.GetNodeIdentifier()};
    const auto serializedMsg = message.SerializeToShortMessage();
    std::pair<std::uint8_t, bool> num_ids_copied;
    std::uint8_t loop_count{0U};
    do
    {
        if (loop_count == 255U)
        {
            bmw::mw::log::LogError("lola") << "An overflow in counting the node identifiers to notifies event update.";
            break;
        }
        else
        {
            loop_count++;
        }

        num_ids_copied = CopyNodeIdentifiers(event_id,
                                             event_notification_ctrl.event_update_interested_nodes_,
                                             event_notification_ctrl.event_update_interested_nodes_mutex_,
                                             nodeIdentifiersTmp,
                                             start_node_id);
        // send NotifyEventUpdateMessage to each node_id in nodeIdentifiersTmp
        for (std::uint8_t i = 0U; i < num_ids_copied.first; i++)
        {
            const pid_t node_identifier = nodeIdentifiersTmp.at(i);
            auto sender = mp_control_.GetMessagePassingSender(asil_level, node_identifier);
            AMP_ASSERT_PRD_MESSAGE(sender != nullptr,
                                   "sender is  a nullpointer. This should not have happend. GetMessagePassingSender "
                                   "should allways return a valid shared pointer.");
            const auto result = sender->Send(serializedMsg);
            if (!result.has_value())
            {
                bmw::mw::log::LogError("lola")
                    << "NotifyEventHandler: Sending NotifyEventUpdateMessage to node_id " << node_identifier
                    << " with asil_level " << ToString(asil_level) << " failed with error: " << result.error();
            }
        }
        if (num_ids_copied.second)
        {
            start_node_id = nodeIdentifiersTmp.back() + 1;
        }
    } while (num_ids_copied.second);

    if (loop_count > 1U)
    {
        bmw::mw::log::LogWarn("lola") << "NotifyEventHandler: NotifyEventRemote did need more than one copy loop for "
                                         "node_identifiers. Think about extending capacity of NodeIdTmpBufferType!";
    }
}

std::uint32_t bmw::mw::com::impl::lola::NotifyEventHandler::NotifyEventLocally(const amp::stop_token& token,
                                                                               const QualityType asil_level,
                                                                               const ElementFqId event_id)
{
    auto& control_data = asil_level == QualityType::kASIL_QM ? control_data_qm_ : control_data_asil_;
    std::uint32_t handlers_called{0U};

    std::shared_lock<std::shared_mutex> read_lock(control_data.event_update_handlers_mutex_);
    auto search = control_data.event_update_handlers_.find(event_id);
    if ((search != control_data.event_update_handlers_.end()) && (!search->second.empty()))
    {
        auto& handlers_for_event = search->second;
        IMessagePassingService::HandlerRegistrationNoType reg_no_start = handlers_for_event.front().register_no;
        // call first handler under current lock
        amp::ignore = handlers_for_event.front().handler();
        handlers_called++;

        // unlock and call remaining handlers one by one in a loop, each time re-acquiring the read lock.
        read_lock.unlock();
        bool handler_left{true};
        
        do
        {
            read_lock.lock();
            auto result = std::upper_bound(handlers_for_event.begin(),
                                           handlers_for_event.end(),
                                           reg_no_start,
                                           [](const IMessagePassingService::HandlerRegistrationNoType value,
                                              const RegisteredNotificationHandler& regHandler) -> bool {
                                               
                                               return (value < regHandler.register_no);
                                               
                                           });
            if (result != handlers_for_event.end())
            {
                amp::ignore = result->handler();
                reg_no_start = result->register_no;
                handlers_called++;
            }
            else
            {
                handler_left = false;
            }
            read_lock.unlock();
        } while (handler_left && (token.stop_requested() == false));
        
    }
    return handlers_called;
}

void bmw::mw::com::impl::lola::NotifyEventHandler::HandleNotifyEventMsg(
    const message_passing::ShortMessagePayload msg_payload,
    const QualityType asil_level,
    const pid_t sender_node_id)
{
    AMP_ASSERT_PRD_MESSAGE(
        (asil_level == QualityType::kASIL_QM) || ((asil_level == QualityType::kASIL_B) && asil_b_capability_),
        "Invalid asil level.");

    const auto message = NotifyEventUpdateMessage::DeserializeToElementFqIdMessage(msg_payload, sender_node_id);

    if (NotifyEventLocally(token_, asil_level, message.GetElementFqId()) == 0U)
    {
        
        
        bmw::mw::log::LogWarn("lola")
            << "NotifyEventHandler: Received NotifyEventUpdateMessage for event: "
            << message.GetElementFqId().ToString() << " from node " << sender_node_id
            << " although we don't have currently any registered handlers. Might be an acceptable "
               "race, if it happens seldom!";
        
        
    }
}

void bmw::mw::com::impl::lola::NotifyEventHandler::HandleRegisterNotificationMsg(
    const message_passing::ShortMessagePayload msg_payload,
    const QualityType asil_level,
    const pid_t sender_node_id)
{
    AMP_ASSERT_PRD_MESSAGE(
        (asil_level == QualityType::kASIL_QM) || ((asil_level == QualityType::kASIL_B) && asil_b_capability_),
        "Invalid asil level.");
    auto& control_data = asil_level == QualityType::kASIL_QM ? control_data_qm_ : control_data_asil_;
    bool already_registered{false};

    const auto message = RegisterEventNotificationMessage::DeserializeToElementFqIdMessage(msg_payload, sender_node_id);

    std::unique_lock<std::shared_mutex> write_lock(control_data.event_update_interested_nodes_mutex_);
    auto search = control_data.event_update_interested_nodes_.find(message.GetElementFqId());
    if (search != control_data.event_update_interested_nodes_.end())
    {
        auto inserted = search->second.insert(sender_node_id);
        already_registered = (inserted.second == false);
    }
    else
    {
        auto emplaced =
            control_data.event_update_interested_nodes_.emplace(message.GetElementFqId(), std::set<pid_t>{});
        amp::ignore = emplaced.first->second.insert(sender_node_id);
    }
    write_lock.unlock();
    if (already_registered)
    {
        
        bmw::mw::log::LogWarn("lola")
            << "NotifyEventHandler: Received redundant RegisterEventNotificationMessage for event: "
            << message.GetElementFqId().ToString() << " from node " << sender_node_id;
        
    }
}

void bmw::mw::com::impl::lola::NotifyEventHandler::HandleUnregisterNotificationMsg(
    const message_passing::ShortMessagePayload msg_payload,
    const QualityType asil_level,
    const pid_t sender_node_id)
{
    AMP_ASSERT_PRD_MESSAGE(
        (asil_level == QualityType::kASIL_QM) || ((asil_level == QualityType::kASIL_B) && asil_b_capability_),
        "Invalid asil level.");
    auto& control_data = asil_level == QualityType::kASIL_QM ? control_data_qm_ : control_data_asil_;

    const auto message =
        UnregisterEventNotificationMessage::DeserializeToElementFqIdMessage(msg_payload, sender_node_id);
    bool registration_found{false};
    std::unique_lock<std::shared_mutex> write_lock(control_data.event_update_interested_nodes_mutex_);
    auto search = control_data.event_update_interested_nodes_.find(message.GetElementFqId());
    if (search != control_data.event_update_interested_nodes_.end())
    {
        registration_found = search->second.erase(sender_node_id) == 1U;
    }
    write_lock.unlock();

    if (!registration_found)
    {
        
        
        bmw::mw::log::LogWarn("lola") << "NotifyEventHandler: Received UnregisterEventNotificationMessage for event: "
                                      << message.GetElementFqId().ToString() << " from node " << sender_node_id
                                      << ", but there was no registration!";
        
        
    }
}

void bmw::mw::com::impl::lola::NotifyEventHandler::HandleOutdatedNodeIdMsg(
    const message_passing::ShortMessagePayload msg_payload,
    const QualityType asil_level,
    const pid_t sender_node_id)
{
    AMP_ASSERT_PRD_MESSAGE(
        (asil_level == QualityType::kASIL_QM) || ((asil_level == QualityType::kASIL_B) && asil_b_capability_),
        "Invalid asil level.");
    auto& control_data = asil_level == QualityType::kASIL_QM ? control_data_qm_ : control_data_asil_;

    const auto message = DeserializeToOutdatedNodeIdMessage(msg_payload, sender_node_id);
    EventUpdateNodeIdMapType::size_type remove_count{0U};
    std::unique_lock<std::shared_mutex> write_lock(control_data.event_update_interested_nodes_mutex_);
    for (auto& element : control_data.event_update_interested_nodes_)
    {
        remove_count += element.second.erase(message.pid_to_unregister);
    }
    write_lock.unlock();

    if (remove_count == 0U)
    {
        bmw::mw::log::LogInfo("lola") << "NotifyEventHandler: HandleOutdatedNodeIdMsg for outdated node id:"
                                      << message.pid_to_unregister << "from node" << sender_node_id
                                      << ". No update notifications for outdated node existed.";
    }

    mp_control_.RemoveMessagePassingSender(asil_level, message.pid_to_unregister);
}
