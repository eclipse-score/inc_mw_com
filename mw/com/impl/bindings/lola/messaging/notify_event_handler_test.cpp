// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#include "platform/aas/mw/com/impl/bindings/lola/messaging/notify_event_handler.h"

#include "platform/aas/mw/com/impl/bindings/lola/messaging/message_passing_control_mock.h"
#include "platform/aas/mw/com/impl/bindings/lola/messaging/messages/message_common.h"
#include "platform/aas/mw/com/impl/bindings/lola/messaging/messages/message_outdated_nodeid.h"
#include "platform/aas/mw/com/impl/bindings/lola/messaging/thread_abstraction.h"
#include "platform/aas/mw/com/message_passing/receiver_mock.h"
#include "platform/aas/mw/com/message_passing/sender_mock.h"

#include "platform/aas/language/safecpp/scoped_function/scope.h"

#include <amp_optional.hpp>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>
#include <cstring>
#include <memory>
#include <thread>
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
namespace
{

using ::testing::_;
using ::testing::An;
using ::testing::AnyNumber;
using ::testing::ByMove;
using ::testing::Invoke;
using ::testing::Matcher;
using ::testing::Return;
using ::testing::ReturnRef;

const ElementFqId SOME_ELEMENT_FQ_ID{1, 1, 1, ElementType::EVENT};
constexpr pid_t LOCAL_NODE_ID = 4444;
constexpr pid_t REMOTE_NODE_ID = 763;
constexpr pid_t NEW_REMOTE_NODE_ID = 764;
constexpr pid_t OUTDATED_REMOTE_NODE_ID = 551;

class ThreadHWConcurrencyMock : public ThreadHWConcurrencyIfc
{
  public:
    MOCK_METHOD(unsigned int, hardware_concurrency, (), (const, noexcept, override));
};

class NotifyEventHandlerFixture : public ::testing::Test
{
  public:
    void SetUp() override
    {
        // expect, that GetNodeIdentifier() will always return the local node id
        EXPECT_CALL(mp_control_mock_, GetNodeIdentifier()).WillRepeatedly(Return(LOCAL_NODE_ID));
    }

    void TearDown() override {}

    /// \brief create NotifyEventHandler uut
    /// \param asil_support false - only QM supported, true - QM and ASIL-B supported
    void PrepareUnit(bool asil_support) { unit_.emplace(mp_control_mock_, asil_support, stop_token_); }

    /// \brief helper to register all needed receive handlers
    /// \param asil_support false - only QM supported, true - QM and ASIL-B supported
    void ReceiveHandlersAreRegistered(bool asil_support)
    {
        // expect, that callbacks are registered for messages kRegisterEventNotifier
        EXPECT_CALL(receiver_mock_,
                    Register(static_cast<std::underlying_type_t<MessageType>>(MessageType::kRegisterEventNotifier),
                             (An<message_passing::IReceiver::ShortMessageReceivedCallback>())))
            .Times(1)
            .WillOnce(Invoke(this, &NotifyEventHandlerFixture::RegisterRegisterEventNotifierReceivedQmCB));
        // ... and kUnregisterEventNotifier
        EXPECT_CALL(receiver_mock_,
                    Register(static_cast<std::underlying_type_t<MessageType>>(MessageType::kUnregisterEventNotifier),
                             (An<message_passing::IReceiver::ShortMessageReceivedCallback>())))
            .Times(1)
            .WillOnce(Invoke(this, &NotifyEventHandlerFixture::RegisterUnregisterEventNotifierReceivedQmCB));
        // ... and kNotifyEvent
        EXPECT_CALL(receiver_mock_,
                    Register(static_cast<std::underlying_type_t<MessageType>>(MessageType::kNotifyEvent),
                             (An<message_passing::IReceiver::ShortMessageReceivedCallback>())))
            .Times(1)
            .WillOnce(Invoke(this, &NotifyEventHandlerFixture::RegisterNotifyEventReceivedQmCB));
        // ... and kOutdatedNodeId
        EXPECT_CALL(receiver_mock_,
                    Register(static_cast<std::underlying_type_t<MessageType>>(MessageType::kOutdatedNodeId),
                             (An<message_passing::IReceiver::ShortMessageReceivedCallback>())))
            .Times(1)
            .WillOnce(Invoke(this, &NotifyEventHandlerFixture::RegisterOutdatedNodeIdReceivedQmCB));

        // when calling RegisterMessageReceivedCallbacks
        unit_.value().RegisterMessageReceivedCallbacks(QualityType::kASIL_QM, receiver_mock_);

        if (asil_support)
        {
            // expect, that callbacks for messages kRegisterEventNotifier
            EXPECT_CALL(receiver_mock_,
                        Register(static_cast<std::underlying_type_t<MessageType>>(MessageType::kRegisterEventNotifier),
                                 (An<message_passing::IReceiver::ShortMessageReceivedCallback>())))
                .Times(1);
            // ... and kUnregisterEventNotifier
            EXPECT_CALL(
                receiver_mock_,
                Register(static_cast<std::underlying_type_t<MessageType>>(MessageType::kUnregisterEventNotifier),
                         (An<message_passing::IReceiver::ShortMessageReceivedCallback>())))
                .Times(1);
            // ... and kNotifyEvent
            EXPECT_CALL(receiver_mock_,
                        Register(static_cast<std::underlying_type_t<MessageType>>(MessageType::kNotifyEvent),
                                 (An<message_passing::IReceiver::ShortMessageReceivedCallback>())))
                .Times(1);
            // ... and kOutdatedNodeId
            EXPECT_CALL(receiver_mock_,
                        Register(static_cast<std::underlying_type_t<MessageType>>(MessageType::kOutdatedNodeId),
                                 (An<message_passing::IReceiver::ShortMessageReceivedCallback>())))
                .Times(1);

            // when calling RegisterMessageReceivedCallbacks
            unit_.value().RegisterMessageReceivedCallbacks(QualityType::kASIL_B, receiver_mock_);
        }
    }

    void RegisterRegisterEventNotifierReceivedQmCB(message_passing::MessageId id,
                                                   message_passing::IReceiver::ShortMessageReceivedCallback callback)
    {
        EXPECT_EQ(id, static_cast<message_passing::MessageId>(MessageType::kRegisterEventNotifier));
        register_event_notifier_message_received_ = std::move(callback);
    }

    void RegisterUnregisterEventNotifierReceivedQmCB(message_passing::MessageId id,
                                                     message_passing::IReceiver::ShortMessageReceivedCallback callback)
    {
        EXPECT_EQ(id, static_cast<message_passing::MessageId>(MessageType::kUnregisterEventNotifier));
        unregister_event_notifier_message_received_ = std::move(callback);
    }

    void RegisterNotifyEventReceivedQmCB(message_passing::MessageId id,
                                         message_passing::IReceiver::ShortMessageReceivedCallback callback)
    {
        EXPECT_EQ(id, static_cast<message_passing::MessageId>(MessageType::kNotifyEvent));
        event_notify_message_received_ = std::move(callback);
    }

    void RegisterOutdatedNodeIdReceivedQmCB(message_passing::MessageId id,
                                            message_passing::IReceiver::ShortMessageReceivedCallback callback)
    {
        EXPECT_EQ(id, static_cast<message_passing::MessageId>(MessageType::kOutdatedNodeId));
        outdated_node_id_message_received_ = std::move(callback);
    }

    IMessagePassingService::HandlerRegistrationNoType LocalEventNotificationForLocalEventIsRegistered(
        QualityType asil_level,
        ElementFqId element_id)
    {
        auto eventUpdateNotificationHandler = BindingEventReceiveHandler(
            event_receive_handler_scope_, [this]() noexcept { notify_event_callback_counter_++; });

        return unit_.value().RegisterEventNotification(
            asil_level, element_id, std::move(eventUpdateNotificationHandler), LOCAL_NODE_ID);
    }

    IMessagePassingService::HandlerRegistrationNoType LocalEventNotificationForRemoteEventIsRegistered(
        QualityType asil_level,
        ElementFqId element_id)
    {
        auto eventUpdateNotificationHandler = BindingEventReceiveHandler(
            event_receive_handler_scope_, [this]() noexcept { notify_event_callback_counter_++; });

        // expect that GetMessagePassingSender() to be called
        EXPECT_CALL(mp_control_mock_, GetMessagePassingSender(QualityType::kASIL_QM, REMOTE_NODE_ID))
            .WillOnce(Return(getSenderMock()));

        // expect, that one Send-call takes place at the Sender as the register-event-notifier message is sent to a
        // different process.
        // ... where we extract the element_fq_id contained in the sent message.
        EXPECT_CALL(*sender_mock_, Send(An<const message_passing::ShortMessage&>()))
            .WillOnce(Invoke([element_id](const message_passing::ShortMessage& msg) {
                amp::expected_blank<bmw::os::Error> blank{};
                EXPECT_EQ(msg.id, static_cast<message_passing::MessageId>(MessageType::kRegisterEventNotifier));
                EXPECT_EQ(msg.payload, ElementFqIdToShortMsgPayload(element_id));
                return blank;
            }));

        return unit_.value().RegisterEventNotification(
            asil_level, element_id, std::move(eventUpdateNotificationHandler), REMOTE_NODE_ID);
    }

    void RemoteEventNotificationIsRegistered(QualityType, ElementFqId element_id, pid_t remote_node_id = REMOTE_NODE_ID)
    {
        // when a RegisterEventNotification message has been received
        message_passing::ShortMessagePayload payload = ElementFqIdToShortMsgPayload(element_id);
        register_event_notifier_message_received_(payload, remote_node_id);
    }

    void CreateRemoteNodeIdentifiers(pid_t start, size_t count)
    {
        remote_node_ids_.clear();
        while (count > 0)
        {
            remote_node_ids_.push_back(start++);
            count--;
        }
    }

    amp::stop_source source_{};
    amp::stop_token stop_token_{source_.get_token()};

    std::shared_ptr<bmw::mw::com::message_passing::ISender> getSenderMock() { return sender_mock_; }

    bmw::mw::com::message_passing::ReceiverMock receiver_mock_{};
    std::shared_ptr<bmw::mw::com::message_passing::SenderMock> sender_mock_{
        std::make_shared<bmw::mw::com::message_passing::SenderMock>()};
    bmw::mw::com::impl::lola::MessagePassingControlMock mp_control_mock_{};
    ThreadHWConcurrencyMock concurrency_mock_{};

    amp::optional<NotifyEventHandler> unit_;

    std::atomic<size_t> notify_event_callback_counter_{};
    QualityType last_asil_level_{QualityType::kInvalid};

    message_passing::IReceiver::ShortMessageReceivedCallback register_event_notifier_message_received_;
    message_passing::IReceiver::ShortMessageReceivedCallback unregister_event_notifier_message_received_;
    message_passing::IReceiver::ShortMessageReceivedCallback event_notify_message_received_;
    message_passing::IReceiver::ShortMessageReceivedCallback outdated_node_id_message_received_;

    std::vector<pid_t> remote_node_ids_{};
    safecpp::Scope<> event_receive_handler_scope_{};
};

TEST_F(NotifyEventHandlerFixture, Creation)
{
    ThreadHWConcurrency::injectMock(&concurrency_mock_);
    EXPECT_CALL(concurrency_mock_, hardware_concurrency()).Times(1).WillOnce(Return(0));
    // construction of NotifyEventHandler with ASIL support succeeds, even with a HW concurrency mock returning 0
    NotifyEventHandler unitWithAsil{mp_control_mock_, true, source_.get_token()};
    ThreadHWConcurrency::injectMock(nullptr);

    // construction of NotifyEventHandler without ASIL support succeeds.
    NotifyEventHandler unitWithoutAsil{mp_control_mock_, false, source_.get_token()};
}

TEST_F(NotifyEventHandlerFixture, RegisterQMReceiveCallbacks)
{
    // given a NotifyEventHandler without ASIL support
    PrepareUnit(false);

    // expect, that callbacks for messages kRegisterEventNotifier
    EXPECT_CALL(receiver_mock_,
                Register(static_cast<std::underlying_type_t<MessageType>>(MessageType::kRegisterEventNotifier),
                         (An<message_passing::IReceiver::ShortMessageReceivedCallback>())))
        .Times(1);
    // ... and kUnregisterEventNotifier get registered
    EXPECT_CALL(receiver_mock_,
                Register(static_cast<std::underlying_type_t<MessageType>>(MessageType::kUnregisterEventNotifier),
                         (An<message_passing::IReceiver::ShortMessageReceivedCallback>())))
        .Times(1);
    // ... and kNotifyEvent get registered
    EXPECT_CALL(receiver_mock_,
                Register(static_cast<std::underlying_type_t<MessageType>>(MessageType::kNotifyEvent),
                         (An<message_passing::IReceiver::ShortMessageReceivedCallback>())))
        .Times(1);
    // ... and kOutdatedNodeId get registered
    EXPECT_CALL(receiver_mock_,
                Register(static_cast<std::underlying_type_t<MessageType>>(MessageType::kOutdatedNodeId),
                         (An<message_passing::IReceiver::ShortMessageReceivedCallback>())))
        .Times(1);

    // when calling RegisterMessageReceivedCallbacks
    unit_.value().RegisterMessageReceivedCallbacks(QualityType::kASIL_QM, receiver_mock_);
}

TEST_F(NotifyEventHandlerFixture, RegisterASILReceiveCallbacks)
{
    // given a NotifyEventHandler with ASIL support
    PrepareUnit(true);

    // expect, that callbacks for messages kRegisterEventNotifier
    EXPECT_CALL(receiver_mock_,
                Register(static_cast<std::underlying_type_t<MessageType>>(MessageType::kRegisterEventNotifier),
                         (An<message_passing::IReceiver::ShortMessageReceivedCallback>())))
        .Times(1);
    // ... and kUnregisterEventNotifier get registered
    EXPECT_CALL(receiver_mock_,
                Register(static_cast<std::underlying_type_t<MessageType>>(MessageType::kUnregisterEventNotifier),
                         (An<message_passing::IReceiver::ShortMessageReceivedCallback>())))
        .Times(1);
    // ... and kNotifyEvent get registered
    EXPECT_CALL(receiver_mock_,
                Register(static_cast<std::underlying_type_t<MessageType>>(MessageType::kNotifyEvent),
                         (An<message_passing::IReceiver::ShortMessageReceivedCallback>())))
        .Times(1);
    // ... and kOutdatedNodeId get registered
    EXPECT_CALL(receiver_mock_,
                Register(static_cast<std::underlying_type_t<MessageType>>(MessageType::kOutdatedNodeId),
                         (An<message_passing::IReceiver::ShortMessageReceivedCallback>())))
        .Times(1);

    // when calling RegisterMessageReceivedCallbacks
    unit_.value().RegisterMessageReceivedCallbacks(QualityType::kASIL_B, receiver_mock_);
}

TEST_F(NotifyEventHandlerFixture, RegisterNotification_LocalEvent)
{
    safecpp::Scope<> event_receive_handler_scope{};
    auto eventUpdateNotificationHandler = BindingEventReceiveHandler(
        event_receive_handler_scope, [this]() noexcept { notify_event_callback_counter_++; });

    // given a NotifyEventHandler without ASIL support
    PrepareUnit(false);

    // we expect NO GetMessagePassingSender() calls
    EXPECT_CALL(mp_control_mock_, GetMessagePassingSender(QualityType::kASIL_QM, REMOTE_NODE_ID)).Times(0);

    // when registering a receive-handler for a local event
    unit_.value().RegisterEventNotification(
        QualityType::kASIL_QM, SOME_ELEMENT_FQ_ID, std::move(eventUpdateNotificationHandler), LOCAL_NODE_ID);
}

TEST_F(NotifyEventHandlerFixture, RegisterNotification_RemoteEvent)
{
    safecpp::Scope<> event_receive_handler_scope{};
    auto eventUpdateNotificationHandler = BindingEventReceiveHandler(
        event_receive_handler_scope, [this]() noexcept { notify_event_callback_counter_++; });

    // given a NotifyEventHandler without ASIL support
    PrepareUnit(false);

    // expect that GetMessagePassingSender() to be called
    EXPECT_CALL(mp_control_mock_, GetMessagePassingSender(QualityType::kASIL_QM, REMOTE_NODE_ID))
        .WillOnce(Return(getSenderMock()));

    // expect, that one Send-call of a RegisterEventNotifier message takes place
    EXPECT_CALL(*sender_mock_, Send(An<const message_passing::ShortMessage&>()))
        .WillOnce(Invoke([](const message_passing::ShortMessage& msg) {
            amp::expected_blank<bmw::os::Error> blank{};
            EXPECT_EQ(msg.id, static_cast<message_passing::MessageId>(MessageType::kRegisterEventNotifier));
            EXPECT_EQ(msg.payload, ElementFqIdToShortMsgPayload(SOME_ELEMENT_FQ_ID));
            return blank;
        }));

    // when registering a receive-handler for a event on a remote node
    unit_.value().RegisterEventNotification(
        QualityType::kASIL_QM, SOME_ELEMENT_FQ_ID, std::move(eventUpdateNotificationHandler), REMOTE_NODE_ID);
}

/**
 * \brief Basically same test case as above (RegisterNotification_RemoteEvent), but this time the message sending to
 *        remote node fails. UuT in this case logs a warning, but since ara::log has currently no mock support,
 *        we don't check that explicitly!
 */
TEST_F(NotifyEventHandlerFixture, RegisterNotification_RemoteEvent_SendError)
{
    safecpp::Scope<> event_receive_handler_scope{};
    auto eventUpdateNotificationHandler = BindingEventReceiveHandler(
        event_receive_handler_scope, [this]() noexcept { notify_event_callback_counter_++; });

    // given a NotifyEventHandler without ASIL support
    PrepareUnit(false);

    // expect that GetMessagePassingSender() to be called
    EXPECT_CALL(mp_control_mock_, GetMessagePassingSender(QualityType::kASIL_QM, REMOTE_NODE_ID))
        .WillOnce(Return(getSenderMock()));

    // expect, that one Send-call of a RegisterEventNotifier message takes place, which in this test returns an error.
    EXPECT_CALL(*sender_mock_, Send(An<const message_passing::ShortMessage&>()))
        .WillOnce(Invoke([](const message_passing::ShortMessage& msg) {
            amp::expected_blank<bmw::os::Error> blank{amp::make_unexpected(bmw::os::Error::createFromErrno(10))};
            EXPECT_EQ(msg.id, static_cast<message_passing::MessageId>(MessageType::kRegisterEventNotifier));
            EXPECT_EQ(msg.payload, ElementFqIdToShortMsgPayload(SOME_ELEMENT_FQ_ID));
            return blank;
        }));

    // when registering a receive-handler for a event on a remote node
    unit_.value().RegisterEventNotification(
        QualityType::kASIL_QM, SOME_ELEMENT_FQ_ID, std::move(eventUpdateNotificationHandler), REMOTE_NODE_ID);
}

TEST_F(NotifyEventHandlerFixture, RegisterMultipleNotification_RemoteEvent)
{
    // given a NotifyEventHandler without ASIL support
    PrepareUnit(false);
    // with registered receive-handlers
    ReceiveHandlersAreRegistered(false);
    // and there is already a registered event notification for a remote event
    LocalEventNotificationForRemoteEventIsRegistered(QualityType::kASIL_QM, SOME_ELEMENT_FQ_ID);

    // expect that GetMessagePassingSender() is NOT called
    EXPECT_CALL(mp_control_mock_, GetMessagePassingSender(QualityType::kASIL_QM, REMOTE_NODE_ID)).Times(0);
    // expect, that NO RegisterNotificationMessage is sent
    EXPECT_CALL(*sender_mock_, Send(An<const message_passing::ShortMessage&>())).Times(0);

    // when there is an additional/2nd notification-registration for the same event
    unit_.value().RegisterEventNotification(
        QualityType::kASIL_QM, SOME_ELEMENT_FQ_ID, BindingEventReceiveHandler{}, REMOTE_NODE_ID);
}

TEST_F(NotifyEventHandlerFixture, RegisterMultipleNotificationNewNode_RemoteEvent)
{
    // given a NotifyEventHandler without ASIL support
    PrepareUnit(false);
    // with registered receive-handlers
    ReceiveHandlersAreRegistered(false);
    // and there is already a registered event notification for a remote event
    LocalEventNotificationForRemoteEventIsRegistered(QualityType::kASIL_QM, SOME_ELEMENT_FQ_ID);

    // expect that GetMessagePassingSender() to be called for new node id
    EXPECT_CALL(mp_control_mock_, GetMessagePassingSender(QualityType::kASIL_QM, NEW_REMOTE_NODE_ID))
        .WillOnce(Return(getSenderMock()));
    // expect, that RegisterNotificationMessage is sent
    EXPECT_CALL(*sender_mock_, Send(An<const message_passing::ShortMessage&>()));

    // when there is an additional/2nd notification-registration for the same event but now for a new/different node id
    unit_.value().RegisterEventNotification(
        QualityType::kASIL_QM, SOME_ELEMENT_FQ_ID, BindingEventReceiveHandler{}, NEW_REMOTE_NODE_ID);
}

TEST_F(NotifyEventHandlerFixture, NotifyEvent_LocalReceiverOnly)
{
    RecordProperty("Verifies", ", , ");  // 
    RecordProperty("Description", "Callback is invoked from within messaging thread");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("Priority", "1");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    // given a NotifyEventHandler without ASIL support
    PrepareUnit(false);
    // with a registered event-receive-handler/event-notification
    LocalEventNotificationForLocalEventIsRegistered(QualityType::kASIL_QM, SOME_ELEMENT_FQ_ID);

    // when notifying the event
    unit_.value().NotifyEvent(QualityType::kASIL_QM, SOME_ELEMENT_FQ_ID);

    // expect, that the event-notification has been called
    while (notify_event_callback_counter_ != 1)
    {
        std::this_thread::yield();
    };
}

TEST_F(NotifyEventHandlerFixture, UnregisterNotification_LocalEvent)
{
    // given a NotifyEventHandler without ASIL support
    PrepareUnit(false);
    // with a registered event-receive-handler/event-notification
    auto regNo = LocalEventNotificationForLocalEventIsRegistered(QualityType::kASIL_QM, SOME_ELEMENT_FQ_ID);

    // when unregistering the receive-handler
    unit_.value().UnregisterEventNotification(QualityType::kASIL_QM, SOME_ELEMENT_FQ_ID, regNo, LOCAL_NODE_ID);
    // and then notifying the event
    unit_.value().NotifyEvent(QualityType::kASIL_QM, SOME_ELEMENT_FQ_ID);

    // expect, that the event-notification has NOT been called
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(100ms);
    EXPECT_EQ(notify_event_callback_counter_, 0);
}

TEST_F(NotifyEventHandlerFixture, UnregisterNotification_LocalEvent_Unkown)
{
    IMessagePassingService::HandlerRegistrationNoType unknownRegNo{9999999};
    // given a NotifyEventHandler without ASIL support
    PrepareUnit(false);
    // with a registered event-receive-handler/event-notification
    amp::ignore = LocalEventNotificationForLocalEventIsRegistered(QualityType::kASIL_QM, SOME_ELEMENT_FQ_ID);

    // when unregistering a receive-handler with an unknown/non-existing registration number
    unit_.value().UnregisterEventNotification(QualityType::kASIL_QM, SOME_ELEMENT_FQ_ID, unknownRegNo, LOCAL_NODE_ID);

    // and then notifying the event
    unit_.value().NotifyEvent(QualityType::kASIL_QM, SOME_ELEMENT_FQ_ID);

    // expect, that the event-notification of the registered event (not yet unregistered!) has still been called.
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(100ms);
    EXPECT_EQ(notify_event_callback_counter_, 1);
}

/**
 * \brief Basically same test case as above (UnregisterNotification_LocalEvent), but this time the Unregister call
 *        is done with another (wrong) remote node id as used for the Register call!
 *        UuT in this case logs a warning, but since ara::log has currently no mock support, we don't check that
 *        explicitly!
 */
TEST_F(NotifyEventHandlerFixture, UnregisterNotification_LocalEvent_WrongNodeId)
{
    // given a NotifyEventHandler without ASIL support
    PrepareUnit(false);
    // with a registered event-receive-handler/event-notification for a local event
    auto regNo = LocalEventNotificationForLocalEventIsRegistered(QualityType::kASIL_QM, SOME_ELEMENT_FQ_ID);

    // expect that GetMessagePassingSender() is NOT getting called
    EXPECT_CALL(mp_control_mock_, GetMessagePassingSender(QualityType::kASIL_QM, REMOTE_NODE_ID)).Times(0);

    // expect, that NO Send-call of a UnregisterEventNotifier message takes place
    EXPECT_CALL(*sender_mock_, Send(An<const message_passing::ShortMessage&>())).Times(0);

    // when unregistering the receive-handler with a different (wrong) node id
    unit_.value().UnregisterEventNotification(QualityType::kASIL_QM, SOME_ELEMENT_FQ_ID, regNo, REMOTE_NODE_ID);
}

TEST_F(NotifyEventHandlerFixture, UnregisterNotification_RemoteEvent)
{
    // given a NotifyEventHandler without ASIL support
    PrepareUnit(false);
    // with a registered event-receive-handler/event-notification for a remote event
    auto regNo = LocalEventNotificationForRemoteEventIsRegistered(QualityType::kASIL_QM, SOME_ELEMENT_FQ_ID);

    // expect that GetMessagePassingSender() to be called
    EXPECT_CALL(mp_control_mock_, GetMessagePassingSender(QualityType::kASIL_QM, REMOTE_NODE_ID))
        .WillOnce(Return(getSenderMock()));

    // expect, that one Send-call of a UnregisterEventNotifier message takes place
    EXPECT_CALL(*sender_mock_, Send(An<const message_passing::ShortMessage&>()))
        .WillOnce(Invoke([](const message_passing::ShortMessage& msg) {
            amp::expected_blank<bmw::os::Error> blank{};
            EXPECT_EQ(msg.id, static_cast<message_passing::MessageId>(MessageType::kUnregisterEventNotifier));
            EXPECT_EQ(msg.payload, ElementFqIdToShortMsgPayload(SOME_ELEMENT_FQ_ID));
            return blank;
        }));

    // when unregistering the receive-handler
    unit_.value().UnregisterEventNotification(QualityType::kASIL_QM, SOME_ELEMENT_FQ_ID, regNo, REMOTE_NODE_ID);
}

TEST_F(NotifyEventHandlerFixture, UnregisterNotification_RemoteEvent_UnknownNode)
{
    // given a NotifyEventHandler without ASIL support
    PrepareUnit(false);
    // with a registered event-receive-handler/event-notification for a remote event on node REMOTE_NODE_ID
    auto regNo = LocalEventNotificationForRemoteEventIsRegistered(QualityType::kASIL_QM, SOME_ELEMENT_FQ_ID);

    // expect that GetMessagePassingSender() NOT to be called
    EXPECT_CALL(mp_control_mock_, GetMessagePassingSender(QualityType::kASIL_QM, REMOTE_NODE_ID)).Times(0);

    // expect, that NO Send-call of a UnregisterEventNotifier message takes place
    EXPECT_CALL(*sender_mock_, Send(An<const message_passing::ShortMessage&>())).Times(0);

    // when unregistering the receive-handler for a remote node, for which no receive-handler has been yet registered
    unit_.value().UnregisterEventNotification(QualityType::kASIL_QM, SOME_ELEMENT_FQ_ID, regNo, NEW_REMOTE_NODE_ID);
}

/**
 * \brief Basically same test case as above (UnregisterNotification_RemoteEvent), but this time the message sending to
 *        remote node fails. UuT in this case logs a warning, but since ara::log has currently no mock support,
 *        we don't check that explicitly!
 */
TEST_F(NotifyEventHandlerFixture, UnregisterNotification_RemoteEvent_SendError)
{
    // given a NotifyEventHandler without ASIL support
    PrepareUnit(false);
    // with a registered event-receive-handler/event-notification for a remote event
    auto regNo = LocalEventNotificationForRemoteEventIsRegistered(QualityType::kASIL_QM, SOME_ELEMENT_FQ_ID);

    // expect that GetMessagePassingSender() to be called
    EXPECT_CALL(mp_control_mock_, GetMessagePassingSender(QualityType::kASIL_QM, REMOTE_NODE_ID))
        .WillOnce(Return(getSenderMock()));

    // expect, that one Send-call of a UnregisterEventNotifier message takes place, which in this test fails
    EXPECT_CALL(*sender_mock_, Send(An<const message_passing::ShortMessage&>()))
        .WillOnce(Invoke([](const message_passing::ShortMessage& msg) {
            amp::expected_blank<bmw::os::Error> blank{amp::make_unexpected(bmw::os::Error::createFromErrno(10))};
            EXPECT_EQ(msg.id, static_cast<message_passing::MessageId>(MessageType::kUnregisterEventNotifier));
            EXPECT_EQ(msg.payload, ElementFqIdToShortMsgPayload(SOME_ELEMENT_FQ_ID));
            return blank;
        }));

    // when unregistering the receive-handler
    unit_.value().UnregisterEventNotification(QualityType::kASIL_QM, SOME_ELEMENT_FQ_ID, regNo, REMOTE_NODE_ID);
}

TEST_F(NotifyEventHandlerFixture, ReregisterNotification_RemoteEvent_OK)
{
    // given a NotifyEventHandler without ASIL support
    PrepareUnit(false);
    // with registered receive-handlers
    ReceiveHandlersAreRegistered(false);
    // and there is already a registered event notification for a remote event
    LocalEventNotificationForRemoteEventIsRegistered(QualityType::kASIL_QM, SOME_ELEMENT_FQ_ID);

    // expect that GetMessagePassingSender() to be called
    EXPECT_CALL(mp_control_mock_, GetMessagePassingSender(QualityType::kASIL_QM, NEW_REMOTE_NODE_ID))
        .WillOnce(Return(getSenderMock()));

    // expect, that one Send-call of a RegisterEventNotifier message takes place
    EXPECT_CALL(*sender_mock_, Send(An<const message_passing::ShortMessage&>()))
        .WillOnce(Invoke([](const message_passing::ShortMessage& msg) {
            EXPECT_EQ(msg.id, static_cast<message_passing::MessageId>(MessageType::kRegisterEventNotifier));
            EXPECT_EQ(msg.payload, ElementFqIdToShortMsgPayload(SOME_ELEMENT_FQ_ID));
            return amp::expected_blank<bmw::os::Error>{};
        }));

    // when re-registering the same event for a new remote id
    unit_.value().ReregisterEventNotification(QualityType::kASIL_QM, SOME_ELEMENT_FQ_ID, NEW_REMOTE_NODE_ID);
}

TEST_F(NotifyEventHandlerFixture, ReregisterNotification_RemoteEvent_2nd)
{
    // given a NotifyEventHandler without ASIL support
    PrepareUnit(false);
    // with registered receive-handlers
    ReceiveHandlersAreRegistered(false);
    // and there is already a registered event notification for a remote event
    LocalEventNotificationForRemoteEventIsRegistered(QualityType::kASIL_QM, SOME_ELEMENT_FQ_ID);

    // expect that GetMessagePassingSender() to be called once
    EXPECT_CALL(mp_control_mock_, GetMessagePassingSender(QualityType::kASIL_QM, NEW_REMOTE_NODE_ID))
        .WillOnce(Return(getSenderMock()));

    // expect, that one Send-call of a RegisterEventNotifier message takes place once
    EXPECT_CALL(*sender_mock_, Send(An<const message_passing::ShortMessage&>()))
        .WillOnce(Invoke([](const message_passing::ShortMessage& msg) {
            EXPECT_EQ(msg.id, static_cast<message_passing::MessageId>(MessageType::kRegisterEventNotifier));
            EXPECT_EQ(msg.payload, ElementFqIdToShortMsgPayload(SOME_ELEMENT_FQ_ID));
            return amp::expected_blank<bmw::os::Error>{};
        }));

    // when re-registering the same event for a new remote id the 1st time
    unit_.value().ReregisterEventNotification(QualityType::kASIL_QM, SOME_ELEMENT_FQ_ID, NEW_REMOTE_NODE_ID);

    // and when a 2nd Reregistration happens for the same event/node-id
    unit_.value().ReregisterEventNotification(QualityType::kASIL_QM, SOME_ELEMENT_FQ_ID, NEW_REMOTE_NODE_ID);
}

TEST_F(NotifyEventHandlerFixture, ReregisterNotification_Unregister)
{
    // given a NotifyEventHandler without ASIL support
    PrepareUnit(false);
    // with registered receive-handlers
    ReceiveHandlersAreRegistered(false);
    // and there is already a registered event notification for a remote event
    auto regNo = LocalEventNotificationForRemoteEventIsRegistered(QualityType::kASIL_QM, SOME_ELEMENT_FQ_ID);

    // expect that GetMessagePassingSender() to be called
    EXPECT_CALL(mp_control_mock_, GetMessagePassingSender(QualityType::kASIL_QM, NEW_REMOTE_NODE_ID))
        .WillOnce(Return(getSenderMock()));

    // expect, that one Send-call of a RegisterEventNotifier message takes place
    EXPECT_CALL(*sender_mock_, Send(An<const message_passing::ShortMessage&>()))
        .WillOnce(Invoke([](const message_passing::ShortMessage& msg) {
            EXPECT_EQ(msg.id, static_cast<message_passing::MessageId>(MessageType::kRegisterEventNotifier));
            EXPECT_EQ(msg.payload, ElementFqIdToShortMsgPayload(SOME_ELEMENT_FQ_ID));
            return amp::expected_blank<bmw::os::Error>{};
        }));

    // when re-registering the same event for a new remote id
    unit_.value().ReregisterEventNotification(QualityType::kASIL_QM, SOME_ELEMENT_FQ_ID, NEW_REMOTE_NODE_ID);

    // expect that GetMessagePassingSender() to be called
    EXPECT_CALL(mp_control_mock_, GetMessagePassingSender(QualityType::kASIL_QM, NEW_REMOTE_NODE_ID))
        .WillOnce(Return(getSenderMock()));

    // expect, that one Send-call of a UnregisterEventNotifier message takes place
    EXPECT_CALL(*sender_mock_, Send(An<const message_passing::ShortMessage&>()))
        .WillOnce(Invoke([](const message_passing::ShortMessage& msg) {
            amp::expected_blank<bmw::os::Error> blank{};
            EXPECT_EQ(msg.id, static_cast<message_passing::MessageId>(MessageType::kUnregisterEventNotifier));
            EXPECT_EQ(msg.payload, ElementFqIdToShortMsgPayload(SOME_ELEMENT_FQ_ID));
            return blank;
        }));

    // when Unregister is called again for the new/re-registered node id
    unit_.value().UnregisterEventNotification(QualityType::kASIL_QM, SOME_ELEMENT_FQ_ID, regNo, NEW_REMOTE_NODE_ID);
}

/**
 * \brief This unit test test a redundant RegisterEventNotification of a remote node.
 *        It has no other visible outcome apart from a warn-log-message, but is needed to fulfill coverage.
 */
TEST_F(NotifyEventHandlerFixture, RegisterEventNotificationReceived_Redundant)
{
    // given a NotifyEventHandler without ASIL support
    PrepareUnit(false);
    // with registered receive-handlers
    ReceiveHandlersAreRegistered(false);
    // and a registered event notification of a remote node
    RemoteEventNotificationIsRegistered(QualityType::kASIL_QM, SOME_ELEMENT_FQ_ID);

    // and then a 2nd time the same remote node sends a RegisterEventNotification message for the same event id
    RemoteEventNotificationIsRegistered(QualityType::kASIL_QM, SOME_ELEMENT_FQ_ID);
}

TEST_F(NotifyEventHandlerFixture, NotifyEvent_RemoteReceiverOnly)
{
    // 
    RecordProperty("Verifies", ", , , , ");
    RecordProperty("Description", "Remote receiver is notified via Message Passing.");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("Priority", "1");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    // given a NotifyEventHandler without ASIL support
    PrepareUnit(false);
    // with registered receive-handlers
    ReceiveHandlersAreRegistered(false);
    // and a registered event notification of a remote node
    RemoteEventNotificationIsRegistered(QualityType::kASIL_QM, SOME_ELEMENT_FQ_ID);

    // expect that GetMessagePassingSender() to be called
    EXPECT_CALL(mp_control_mock_, GetMessagePassingSender(QualityType::kASIL_QM, REMOTE_NODE_ID))
        .WillOnce(Return(getSenderMock()));

    // and expect, that a NotifyEventUpdateMessage is sent out for event SOME_ELEMENT_FQ_ID
    EXPECT_CALL(*sender_mock_, Send(An<const message_passing::ShortMessage&>()))
        .WillOnce(Invoke([](const message_passing::ShortMessage& msg) {
            amp::expected_blank<bmw::os::Error> blank{};
            EXPECT_EQ(msg.id, static_cast<message_passing::MessageId>(MessageType::kNotifyEvent));
            EXPECT_EQ(msg.payload, ElementFqIdToShortMsgPayload(SOME_ELEMENT_FQ_ID));
            return blank;
        }));

    // when notifying the event
    unit_.value().NotifyEvent(QualityType::kASIL_QM, SOME_ELEMENT_FQ_ID);
}

/**
 * \brief Basically same test case as above (NotifyEvent_RemoteReceiverOnly), but this time the message sending to
 *        remote node fails. UuT in this case logs a warning, but since ara::log has currently no mock support,
 *        we don't check that explicitly!
 */
TEST_F(NotifyEventHandlerFixture, NotifyEvent_RemoteReceiverOnly_SendError)
{
    // 
    RecordProperty("Verifies", ", , , , ");
    RecordProperty("Description", "Remote receiver is notified via Message Passing, but notification fails.");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("Priority", "1");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    // given a NotifyEventHandler without ASIL support
    PrepareUnit(false);
    // with registered receive-handlers
    ReceiveHandlersAreRegistered(false);
    // and a registered event notification of a remote node
    RemoteEventNotificationIsRegistered(QualityType::kASIL_QM, SOME_ELEMENT_FQ_ID);

    // expect that GetMessagePassingSender() to be called
    EXPECT_CALL(mp_control_mock_, GetMessagePassingSender(QualityType::kASIL_QM, REMOTE_NODE_ID))
        .WillOnce(Return(getSenderMock()));

    // and expect, that a NotifyEventUpdateMessage is sent out for event SOME_ELEMENT_FQ_ID, but sending fails in this
    // test
    EXPECT_CALL(*sender_mock_, Send(An<const message_passing::ShortMessage&>()))
        .WillOnce(Invoke([](const message_passing::ShortMessage& msg) {
            amp::expected_blank<bmw::os::Error> blank{amp::make_unexpected(bmw::os::Error::createFromErrno(10))};
            EXPECT_EQ(msg.id, static_cast<message_passing::MessageId>(MessageType::kNotifyEvent));
            EXPECT_EQ(msg.payload, ElementFqIdToShortMsgPayload(SOME_ELEMENT_FQ_ID));
            return blank;
        }));

    // when notifying the event
    unit_.value().NotifyEvent(QualityType::kASIL_QM, SOME_ELEMENT_FQ_ID);
}

TEST_F(NotifyEventHandlerFixture, NotifyEvent_HighNumberRemoteReceiversOnly)
{
    // given a NotifyEventHandler without ASIL support
    PrepareUnit(false);
    // with registered receive-handlers
    ReceiveHandlersAreRegistered(false);
    // and a high number of registered event notification of different remote nodes
    // Note: Count is 30 here as the impl. internally copies up to 20 node_identifiers into a temp buffer to
    // do the processing later after unlock(). This test with 30 nodes forces code-paths, where tmp-buffer has to be
    // refilled.
    CreateRemoteNodeIdentifiers(REMOTE_NODE_ID, 30);
    for (auto node_id : remote_node_ids_)
    {
        RemoteEventNotificationIsRegistered(QualityType::kASIL_QM, SOME_ELEMENT_FQ_ID, node_id);
    }

    // expect that GetMessagePassingSender() to be called for each remote node id
    for (auto node_id : remote_node_ids_)
    {
        EXPECT_CALL(mp_control_mock_, GetMessagePassingSender(QualityType::kASIL_QM, node_id))
            .WillOnce(Return(getSenderMock()));
    }
    // and expect, that a NotifyEventUpdateMessage is sent out for event SOME_ELEMENT_FQ_ID for each node
    EXPECT_CALL(*sender_mock_, Send(An<const message_passing::ShortMessage&>()))
        .Times(static_cast<int>(remote_node_ids_.size()))
        .WillRepeatedly(Invoke([](const message_passing::ShortMessage& msg) {
            amp::expected_blank<bmw::os::Error> blank{};
            EXPECT_EQ(msg.id, static_cast<message_passing::MessageId>(MessageType::kNotifyEvent));
            EXPECT_EQ(msg.payload, ElementFqIdToShortMsgPayload(SOME_ELEMENT_FQ_ID));
            return blank;
        }));
    // when notifying the event
    unit_.value().NotifyEvent(QualityType::kASIL_QM, SOME_ELEMENT_FQ_ID);
}

TEST_F(NotifyEventHandlerFixture, ReceiveEventNotification_OneNotifier)
{
    // given a NotifyEventHandler without ASIL support
    PrepareUnit(false);
    // with registered receive-handlers
    ReceiveHandlersAreRegistered(false);
    // and there is a locally registered event notification for a remote event
    LocalEventNotificationForRemoteEventIsRegistered(QualityType::kASIL_QM, SOME_ELEMENT_FQ_ID);

    // when a NotifyEventMessage (id = kNotifyEvent) is received for this event id
    message_passing::ShortMessagePayload payload = ElementFqIdToShortMsgPayload(SOME_ELEMENT_FQ_ID);
    event_notify_message_received_(payload, REMOTE_NODE_ID);

    // expect, that notify_event_callback_counter_ is 1
    EXPECT_EQ(notify_event_callback_counter_, 1);
}

TEST_F(NotifyEventHandlerFixture, ReceiveEventNotification_ZeroNotifier)
{
    // given a NotifyEventHandler without ASIL support
    PrepareUnit(false);
    // with registered receive-handlers
    ReceiveHandlersAreRegistered(false);

    // when a NotifyEventMessage (id = kNotifyEvent) is received for this event id, although we don't have any
    // local interested receiver (proxy-event), which is basically unexpected, but can arise because of an acceptable
    // race-condition ...
    message_passing::ShortMessagePayload payload = ElementFqIdToShortMsgPayload(SOME_ELEMENT_FQ_ID);
    event_notify_message_received_(payload, REMOTE_NODE_ID);

    // expect, that notify_event_callback_counter_ is 0
    EXPECT_EQ(notify_event_callback_counter_, 0);
}

TEST_F(NotifyEventHandlerFixture, ReceiveEventNotification_TwoNotifier)
{
    // given a NotifyEventHandler without ASIL support
    PrepareUnit(false);
    // with registered receive-handlers
    ReceiveHandlersAreRegistered(false);
    // and there is already one locally registered event notification for a remote event
    LocalEventNotificationForRemoteEventIsRegistered(QualityType::kASIL_QM, SOME_ELEMENT_FQ_ID);
    // and we register a 2nd one
    safecpp::Scope<> event_receive_handler_scope{};
    auto eventUpdateNotificationHandler = BindingEventReceiveHandler(
        event_receive_handler_scope, [this]() noexcept { notify_event_callback_counter_++; });

    amp::ignore = unit_.value().RegisterEventNotification(
        QualityType::kASIL_QM, SOME_ELEMENT_FQ_ID, std::move(eventUpdateNotificationHandler), REMOTE_NODE_ID);

    // when a NotifyEventMessage (id = kNotifyEvent) is received for this event id
    message_passing::ShortMessagePayload payload = ElementFqIdToShortMsgPayload(SOME_ELEMENT_FQ_ID);
    event_notify_message_received_(payload, REMOTE_NODE_ID);

    // expect, that notify_event_callback_counter_ is 2
    EXPECT_EQ(notify_event_callback_counter_, 2);
}

TEST_F(NotifyEventHandlerFixture, ReceiveUnregisterEventNotification)
{
    // given a NotifyEventHandler without ASIL support
    PrepareUnit(false);
    // with registered receive-handlers
    ReceiveHandlersAreRegistered(false);
    // and a registered event notification of a remote node
    RemoteEventNotificationIsRegistered(QualityType::kASIL_QM, SOME_ELEMENT_FQ_ID);

    // after a UnregisterEventNotificationMessage (id = kUnregisterEventNotifier) is received for this event id
    message_passing::ShortMessagePayload payload = ElementFqIdToShortMsgPayload(SOME_ELEMENT_FQ_ID);
    unregister_event_notifier_message_received_(payload, REMOTE_NODE_ID);

    // expect, that neither GetMessagePassingSender is called
    EXPECT_CALL(mp_control_mock_, GetMessagePassingSender(QualityType::kASIL_QM, REMOTE_NODE_ID)).Times(0);
    // ... nor a notifyEventMessage is sent to remote node
    EXPECT_CALL(*sender_mock_, Send(An<const message_passing::ShortMessage&>())).Times(0);

    // when notifying the event
    unit_.value().NotifyEvent(QualityType::kASIL_QM, SOME_ELEMENT_FQ_ID);
}

/* \brief This test case is the same as above (ReceiveUnregisterEventNotification), but this time we have not even
 *        an active event update notification registered by the remote node. UuT will in this case log a warning and
 *        do nothing, but since ara::log has currently no mock support, we don't check that.
 **/
TEST_F(NotifyEventHandlerFixture, ReceiveUnregisterEventNotification_WithoutActualRegistration)
{
    // given a NotifyEventHandler without ASIL support
    PrepareUnit(false);
    // with registered receive-handlers
    ReceiveHandlersAreRegistered(false);

    // after a UnregisterEventNotificationMessage (id = kUnregisterEventNotifier) is received for this event id
    message_passing::ShortMessagePayload payload = ElementFqIdToShortMsgPayload(SOME_ELEMENT_FQ_ID);
    unregister_event_notifier_message_received_(payload, REMOTE_NODE_ID);

    // expect, that neither GetMessagePassingSender is called
    EXPECT_CALL(mp_control_mock_, GetMessagePassingSender(QualityType::kASIL_QM, REMOTE_NODE_ID)).Times(0);
    // ... nor a notifyEventMessage is sent to remote node
    EXPECT_CALL(*sender_mock_, Send(An<const message_passing::ShortMessage&>())).Times(0);

    // when notifying the event
    unit_.value().NotifyEvent(QualityType::kASIL_QM, SOME_ELEMENT_FQ_ID);
}

TEST_F(NotifyEventHandlerFixture, ReceiveOutdatedNodeIdMessage_ExistingNodeId)
{
    // given a NotifyEventHandler without ASIL support
    PrepareUnit(false);
    // with registered receive-handlers
    ReceiveHandlersAreRegistered(false);

    // and there has been registered an event-notification by a remote node id
    RemoteEventNotificationIsRegistered(QualityType::kASIL_QM, SOME_ELEMENT_FQ_ID, OUTDATED_REMOTE_NODE_ID);

    // when an OutdatedNodeIdMessage (id = kOutdatedNodeId) is received for this OUTDATED_REMOTE_NODE_ID
    message_passing::ShortMessagePayload payload = OUTDATED_REMOTE_NODE_ID;
    outdated_node_id_message_received_(payload, REMOTE_NODE_ID);

    // then NO notification message is sent to OUTDATED_REMOTE_NODE_ID anymore
    // expect that GetMessagePassingSender() NOT to be called
    EXPECT_CALL(mp_control_mock_, GetMessagePassingSender(QualityType::kASIL_QM, OUTDATED_REMOTE_NODE_ID)).Times(0);

    // and expect, that NO NotifyEventUpdateMessage is sent out for event SOME_ELEMENT_FQ_ID
    EXPECT_CALL(*sender_mock_, Send(An<const message_passing::ShortMessage&>())).Times(0);

    // when notifying the event
    unit_.value().NotifyEvent(QualityType::kASIL_QM, SOME_ELEMENT_FQ_ID);
}

TEST_F(NotifyEventHandlerFixture, ReceiveOutdatedNodeIdMessage_NoExistingNodeId)
{
    // given a NotifyEventHandler without ASIL support
    PrepareUnit(false);
    // with registered receive-handlers
    ReceiveHandlersAreRegistered(false);

    // and there has been registered an event-notification by a remote node id
    RemoteEventNotificationIsRegistered(QualityType::kASIL_QM, SOME_ELEMENT_FQ_ID, REMOTE_NODE_ID);

    // when an OutdatedNodeIdMessage (id = kOutdatedNodeId) is received for a different OUTDATED_REMOTE_NODE_ID
    message_passing::ShortMessagePayload payload = OUTDATED_REMOTE_NODE_ID;
    outdated_node_id_message_received_(payload, REMOTE_NODE_ID);

    // then still notification message is sent to REMOTE_NODE_ID
    // expect that GetMessagePassingSender() to be called
    EXPECT_CALL(mp_control_mock_, GetMessagePassingSender(QualityType::kASIL_QM, REMOTE_NODE_ID))
        .WillOnce(Return(getSenderMock()));

    // and expect, that a NotifyEventUpdateMessage is sent out for event SOME_ELEMENT_FQ_ID
    EXPECT_CALL(*sender_mock_, Send(An<const message_passing::ShortMessage&>()))
        .WillOnce(Invoke([](const message_passing::ShortMessage& msg) {
            amp::expected_blank<bmw::os::Error> blank{};
            EXPECT_EQ(msg.id, static_cast<message_passing::MessageId>(MessageType::kNotifyEvent));
            EXPECT_EQ(msg.payload, ElementFqIdToShortMsgPayload(SOME_ELEMENT_FQ_ID));
            return blank;
        }));

    // when notifying the event
    unit_.value().NotifyEvent(QualityType::kASIL_QM, SOME_ELEMENT_FQ_ID);
}

TEST_F(NotifyEventHandlerFixture, SendOutdatedNodeIdMessage)
{
    // given a NotifyEventHandler without ASIL support
    PrepareUnit(false);

    // expect that GetMessagePassingSender() to be called for REMOTE_NODE_ID
    EXPECT_CALL(mp_control_mock_, GetMessagePassingSender(QualityType::kASIL_QM, REMOTE_NODE_ID))
        .WillOnce(Return(getSenderMock()));

    // and expect, that a OutdatedNodeIdMessage is sent with OUTDATED_REMOTE_NODE_ID
    EXPECT_CALL(*sender_mock_, Send(An<const message_passing::ShortMessage&>()))
        .WillOnce(Invoke([](const message_passing::ShortMessage& msg) {
            amp::expected_blank<bmw::os::Error> blank{};
            EXPECT_EQ(msg.id, static_cast<message_passing::MessageId>(MessageType::kOutdatedNodeId));
            EXPECT_EQ(msg.payload, OUTDATED_REMOTE_NODE_ID);
            return blank;
        }));

    // when notifying OUTDATED_REMOTE_NODE_ID as outdated node id towards REMOTE_NODE_ID
    unit_.value().NotifyOutdatedNodeId(QualityType::kASIL_QM, OUTDATED_REMOTE_NODE_ID, REMOTE_NODE_ID);
}

}  // namespace
}  // namespace lola
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
