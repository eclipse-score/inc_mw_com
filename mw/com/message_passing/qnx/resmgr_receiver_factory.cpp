// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#include "platform/aas/mw/com/message_passing/receiver_factory_impl.h"

#include "platform/aas/mw/com/message_passing/qnx/resmgr_receiver_traits.h"
#include "platform/aas/mw/com/message_passing/receiver.h"



/* *memory_resource is non-const in make_unique() */
amp::pmr::unique_ptr<bmw::mw::com::message_passing::IReceiver>
bmw::mw::com::message_passing::ReceiverFactoryImpl::Create(const amp::string_view identifier,
                                                           concurrency::Executor& executor,
                                                           const amp::span<const uid_t> allowed_uids,
                                                           const ReceiverConfig& receiver_config,
                                                           amp::pmr::memory_resource* const memory_resource)


{
    return amp::pmr::make_unique<Receiver<ResmgrReceiverTraits>>(
        memory_resource, identifier, executor, allowed_uids, receiver_config);
}
