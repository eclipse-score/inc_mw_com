// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#ifndef PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_MESSAGE_OUTDATED_NODEID_H
#define PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_MESSAGE_OUTDATED_NODEID_H

#include "platform/aas/lib/os/unistd.h"
#include "platform/aas/mw/com/impl/bindings/lola/element_fq_id.h"
#include "platform/aas/mw/com/impl/bindings/lola/messaging/messages/message_common.h"
#include "platform/aas/mw/com/message_passing/message.h"

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

/// \brief Message sent from the consumer/proxy side to the provider/skeleton side, to notify provider/skeleton side
///        about the given pid/node id is outdated (was from a previous run of the consumer/proxy side application).
struct OutdatedNodeIdMessage
{
    pid_t pid_to_unregister{};
    // 
    // 
    pid_t sender_node_id{};
};

/// \brief Creates a OutdatedNodeIdMessage from a serialized short message payload.
/// \param message_payload payload from where to deserialize
/// \param sender_node_id node id of the sender, which sends this message.
/// \return OutdatedNodeIdMessage created from payload
OutdatedNodeIdMessage DeserializeToOutdatedNodeIdMessage(const message_passing::ShortMessagePayload& message_payload,
                                                         const pid_t sender_node_id);
/// \brief Serialize message to short message payload.
/// \param outdated_node_id_message OutdatedNodeIdMessage to be serialized.
/// \return short message representing OutdatedNodeIdMessage
message_passing::ShortMessage SerializeToShortMessage(const OutdatedNodeIdMessage& outdated_node_id_message);

/// \brief comparison op for OutdatedNodeIdMessage
/// \param lhs
/// \param rhs
/// \return true in case all members compare equal, false else.
bool operator==(const OutdatedNodeIdMessage& lhs, const OutdatedNodeIdMessage& rhs) noexcept;

}  // namespace lola
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_MESSAGE_OUTDATED_NODEID_H
