// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#ifndef PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_MESSAGECOMMON_H
#define PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_MESSAGECOMMON_H

#include "platform/aas/mw/com/impl/bindings/lola/element_fq_id.h"
#include "platform/aas/mw/com/message_passing/message.h"

#include <cstdint>

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

using CallSeqCounterType = std::uint16_t;

enum class MessageType : message_passing::MessageId
{
    kRegisterEventNotifier = 1,  //< event notifier registration message sent by proxy_events
    kUnregisterEventNotifier,    //< event notifier un-registration message sent by proxy_events
    kNotifyEvent,                //< event update notification message sent by skeleton_events
    kOutdatedNodeId,  //< outdated node id message (sent from a LoLa process in the role as consumer to the producer)
};

/// \brief deserializes a short-message-payload (std::uint32) containing a serialized event fq id into a ElementFqId
/// \details we do have several different messages, which contain as payload a condensed representation of a
///          ElementFqId (serialized to an uint32). Therefore these (de)serialization methods are extracted here for
///          reuse
/// \param msg_payload short message payload containing serialized ElementFqId
/// \return deserialized ElementFqId
ElementFqId ShortMsgPayloadToElementFqId(const message_passing::ShortMessagePayload msg_payload) noexcept;

/// \brief serializes an ElementFqId into a short message payload.
/// \param element_fq_id ElementFqId that will be serialized.
/// \return
message_passing::ShortMessagePayload ElementFqIdToShortMsgPayload(const ElementFqId& element_fq_id) noexcept;

}  // namespace lola
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_MESSAGECOMMON_H
