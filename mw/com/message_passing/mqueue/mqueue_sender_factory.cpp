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


#include "platform/aas/mw/com/message_passing/sender_factory_impl.h"

#include "platform/aas/mw/com/message_passing/mqueue/mqueue_sender_traits.h"
#include "platform/aas/mw/com/message_passing/sender.h"


/* (1) Parameters are used
 *  (2) amp::pmr::make_unique takes a non const amp::pmr::memory_resource hence,
 *  it is not possible to mark amp::pmr::memory_resource as const variable */
amp::pmr::unique_ptr<bmw::mw::com::message_passing::ISender> bmw::mw::com::message_passing::SenderFactoryImpl::Create(
    const amp::string_view identifier,
    const amp::stop_token& token,
    const SenderConfig& sender_config,
    LoggingCallback logging_callback,
    amp::pmr::memory_resource* const memory_resource)

{
    return amp::pmr::make_unique<Sender<MqueueSenderTraits>>(
        memory_resource, identifier, token, sender_config, std::move(logging_callback));
}
