// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#ifndef PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_MESSAGEPASSINGFACADE_H
#define PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_MESSAGEPASSINGFACADE_H

#include "platform/aas/lib/concurrency/thread_pool.h"
#include "platform/aas/lib/os/unistd.h"
#include "platform/aas/mw/com/impl/binding_event_receive_handler.h"
#include "platform/aas/mw/com/impl/bindings/lola/element_fq_id.h"
#include "platform/aas/mw/com/impl/bindings/lola/messaging/i_message_passing_control.h"
#include "platform/aas/mw/com/impl/bindings/lola/messaging/i_message_passing_service.h"
#include "platform/aas/mw/com/impl/bindings/lola/messaging/notify_event_handler.h"
#include "platform/aas/mw/com/impl/configuration/quality_type.h"
#include "platform/aas/mw/com/message_passing/i_receiver.h"
#include "platform/aas/mw/com/message_passing/message.h"

#include <amp_callback.hpp>
#include <amp_memory.hpp>
#include <amp_optional.hpp>

#include <sys/types.h>
#include <cstdint>
#include <memory>
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

namespace test
{
class MessagePassingFacadeAttorney;
}

/// \brief MessagePassingFacade handles message-based communication between LoLa proxy/skeleton instances of different
/// processes.
///
/// \details This message-based communication is a side-channel to the shared-memory based interaction between LoLa
/// proxy/skeleton instances. It is used for exchange of control information/notifications, where the shared-memory
/// channel is used rather for data exchange.
/// MessagePassingFacade relies on message_passing::Receiver/Sender for its communication needs.
/// If it detects, that communication partners are located within the same process, it opts for direct function/method
/// call optimization, instead of using message_passing.
///
class MessagePassingFacade final : public IMessagePassingService
{
    friend test::MessagePassingFacadeAttorney;

  public:
    /// \brief Aggregation of ASIL level specific/dependent config properties.
    struct AsilSpecificCfg
    {
        std::int32_t message_queue_rx_size_;
        // 
        std::vector<uid_t> allowed_user_ids_;
    };

    /// \brief Constructs MessagePassingFacade, which handles the whole inter-process messaging needs for a LoLa enabled
    /// process.
    ///
    /// \details Used by com::impl::Runtime and instantiated only once, since we want to have "singleton" behavior,
    /// without applying singleton pattern.
    ///
    /// \param msgpass_ctrl message passing control used for access to node_identifier, etc.
    /// \param config_asil_qm configuration props for ASIL-QM (mandatory) communication path
    /// \param config_asil_b optional (only needed for ASIL-B enabled MessagePassingFacade) configuration props for
    ///                ASIL-B communication path.
    ///                If this optional contains a value, this leads to implicit ASIL-B support of created
    ///                MessagePassingFacade! This optional should only be set, in case the overall
    ///                application/process is implemented according to ASIL_B requirements and there is at least one
    ///                LoLa service deployment (proxy or skeleton) for the process, with asilLevel "ASIL_B".
    MessagePassingFacade(IMessagePassingControl& msgpass_ctrl,
                         const AsilSpecificCfg config_asil_qm,
                         const amp::optional<AsilSpecificCfg> config_asil_b);

    MessagePassingFacade(const MessagePassingFacade&) = delete;
    MessagePassingFacade(MessagePassingFacade&&) = delete;
    MessagePassingFacade& operator=(const MessagePassingFacade&) = delete;
    MessagePassingFacade& operator=(MessagePassingFacade&&) = delete;

    ~MessagePassingFacade() noexcept override;

    /// \brief Notification, that the given _event_id_ with _asil_level_ has been updated.
    /// \details see IMessagePassingService::NotifyEvent
    void NotifyEvent(const QualityType asil_level, const ElementFqId event_id) override;
    /// \brief Registers a callback for event update notifications for event _event_id_
    /// \details see IMessagePassingService::RegisterEventNotification
    HandlerRegistrationNoType RegisterEventNotification(const QualityType asil_level,
                                                        const ElementFqId event_id,
                                                        BindingEventReceiveHandler callback,
                                                        const pid_t target_node_id) override;

    /// \brief Re-registers an event update notifications for event _event_id_ in case target_node_id is a remote pid.
    /// \details see IMessagePassingService::ReregisterEventNotification
    void ReregisterEventNotification(QualityType asil_level, ElementFqId event_id, pid_t target_node_id) override;

    /// \brief Unregister an event update notification callback, which has been registered with
    ///        RegisterEventNotification()
    /// \details see IMessagePassingService::UnregisterEventNotification
    void UnregisterEventNotification(const QualityType asil_level,
                                     const ElementFqId event_id,
                                     const HandlerRegistrationNoType registration_no,
                                     const pid_t target_node_id) override;

    /// \brief Notifies target node about outdated_node_id being an old/outdated node id, not being used anymore.
    /// \details see IMessagePassingService::NotifyOutdatedNodeId
    void NotifyOutdatedNodeId(QualityType asil_level, pid_t outdated_node_id, pid_t target_node_id) override;

  private:
    struct MessageReceiveCtrl
    {
        /// \brief message receiver
        amp::pmr::unique_ptr<bmw::mw::com::message_passing::IReceiver> receiver_;
        /// \brief ... and its thread-pool/execution context.
        // 
        std::unique_ptr<bmw::concurrency::ThreadPool> thread_pool_;
    };

    /// \brief initialize receiver_asil_qm_ resp. receiver_asil_b_
    /// \param asil_level
    /// \param min_num_messages
    /// \param allowed_user_ids
    void InitializeMessagePassingReceiver(const QualityType asil_level,
                                          const std::vector<uid_t>& allowed_user_ids,
                                          const std::int32_t min_num_messages);

    /// \brief message passing control used to acquire node_identifier and senders.
    IMessagePassingControl& message_passing_ctrl_;

    /// \brief does our instance support ASIL-B?
    bool asil_b_capability_;

    amp::stop_source stop_source_;

    /// \brief handler for notify-event-update, register-event-notification and unregister-event-notification messages.
    /// \attention Position of this handler member is important as it shall be destroyed AFTER the upcoming receiver
    ///            members to avoid race conditions, as those receivers are using this handler to dispatch messages.
    NotifyEventHandler notify_event_handler_;

    /// \brief message passing receiver control, where ASIL-QM qualified messages get received
    MessageReceiveCtrl msg_receiver_qm_;

    /// \brief message passing receiver control, where ASIL-B qualified messages get received
    MessageReceiveCtrl msg_receiver_asil_b_;
};

}  // namespace lola
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_MESSAGEPASSINGFACADE_H
