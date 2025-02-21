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


#include "platform/aas/mw/com/impl/bindings/lola/messaging/message_passing_facade.h"

#include "platform/aas/lib/os/errno_logging.h"
#include "platform/aas/mw/com/impl/bindings/lola/messaging/thread_abstraction.h"
#include "platform/aas/mw/com/message_passing/receiver_factory.h"
#include "platform/aas/mw/log/logging.h"

#include <amp_utility.hpp>

#include <utility>

bmw::mw::com::impl::lola::MessagePassingFacade::MessagePassingFacade(IMessagePassingControl& msgpass_ctrl,
                                                                     const AsilSpecificCfg config_asil_qm,
                                                                     const amp::optional<AsilSpecificCfg> config_asil_b)
    : bmw::mw::com::impl::lola::IMessagePassingService{},
      message_passing_ctrl_(msgpass_ctrl),
      asil_b_capability_{config_asil_b.has_value()},
      stop_source_{},
      notify_event_handler_{msgpass_ctrl, asil_b_capability_, stop_source_.get_token()}
{
    amp::ignore = asil_b_capability_;

    InitializeMessagePassingReceiver(
        QualityType::kASIL_QM, config_asil_qm.allowed_user_ids_, config_asil_qm.message_queue_rx_size_);
    if (asil_b_capability_)
    {
        InitializeMessagePassingReceiver(QualityType::kASIL_B,
                                         config_asil_b.value().allowed_user_ids_,
                                         config_asil_b.value().message_queue_rx_size_);
    }
}

bmw::mw::com::impl::lola::MessagePassingFacade::~MessagePassingFacade() noexcept
{
    // This function call will always return true.
    // As stop is requested only once.
    AMP_ASSERT_PRD_MESSAGE(stop_source_.request_stop(), "unexpected return value");
}

void bmw::mw::com::impl::lola::MessagePassingFacade::InitializeMessagePassingReceiver(
    const QualityType asil_level,
    const std::vector<uid_t>& allowed_user_ids,
    const std::int32_t min_num_messages)
{
    const auto receiverName =
        message_passing_ctrl_.CreateMessagePassingName(asil_level, message_passing_ctrl_.GetNodeIdentifier());

    auto& receiver = (asil_level == QualityType::kASIL_QM ? msg_receiver_qm_ : msg_receiver_asil_b_);

    // \todo Maybe we should make thread pool size configurable via configuration (deployment). Then we can decide how
    // many threads to spend over all and if we should have different number of threads for ASIL-B/QM receivers!
    auto hw_conc = ThreadHWConcurrency::hardware_concurrency();
    if (hw_conc == 0U)
    {
        hw_conc = 2U;  // we fall back to 2 threads, if we can't read out hw_conc.
    }
    const std::string thread_pool_name =
        (asil_level == QualityType::kASIL_QM) ? "mw::com MessageReceiver QM" : "mw::com MessageReceiver ASIL-B";
    receiver.thread_pool_ = std::make_unique<bmw::concurrency::ThreadPool>(hw_conc, thread_pool_name);
    const bmw::mw::com::message_passing::ReceiverConfig receiver_config{min_num_messages};
    receiver.receiver_ = message_passing::ReceiverFactory::Create(
        receiverName, *receiver.thread_pool_, allowed_user_ids, receiver_config);

    notify_event_handler_.RegisterMessageReceivedCallbacks(asil_level, *receiver.receiver_);

    auto result = receiver.receiver_->StartListening();
    if (result.has_value() == false)
    {
        bmw::mw::log::LogFatal("lola")
            << "MessagePassingFacade: Failed to start listening on message_passing receiver with following error: "
            << result.error();
        std::terminate();
    }
}

void bmw::mw::com::impl::lola::MessagePassingFacade::NotifyEvent(const QualityType asil_level,
                                                                 const ElementFqId event_id)
{
    notify_event_handler_.NotifyEvent(asil_level, event_id);
}

bmw::mw::com::impl::lola::IMessagePassingService::HandlerRegistrationNoType
bmw::mw::com::impl::lola::MessagePassingFacade::RegisterEventNotification(const QualityType asil_level,
                                                                          const ElementFqId event_id,
                                                                          BindingEventReceiveHandler callback,
                                                                          const pid_t target_node_id)
{
    return notify_event_handler_.RegisterEventNotification(asil_level, event_id, std::move(callback), target_node_id);
}

void bmw::mw::com::impl::lola::MessagePassingFacade::ReregisterEventNotification(QualityType asil_level,
                                                                                 ElementFqId event_id,
                                                                                 pid_t target_node_id)
{
    notify_event_handler_.ReregisterEventNotification(asil_level, event_id, target_node_id);
}

void bmw::mw::com::impl::lola::MessagePassingFacade::UnregisterEventNotification(
    const QualityType asil_level,
    const ElementFqId event_id,
    const IMessagePassingService::HandlerRegistrationNoType registration_no,
    const pid_t target_node_id)
{
    notify_event_handler_.UnregisterEventNotification(asil_level, event_id, registration_no, target_node_id);
}

void bmw::mw::com::impl::lola::MessagePassingFacade::NotifyOutdatedNodeId(QualityType asil_level,
                                                                          pid_t outdated_node_id,
                                                                          pid_t target_node_id)
{
    notify_event_handler_.NotifyOutdatedNodeId(asil_level, outdated_node_id, target_node_id);
}
