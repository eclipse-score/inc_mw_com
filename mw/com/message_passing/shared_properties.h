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


#ifndef PLATFORM_AAS_MW_COM_MESSAGE_PASSING_SHARED_PROPERTIES_H
#define PLATFORM_AAS_MW_COM_MESSAGE_PASSING_SHARED_PROPERTIES_H

#include "platform/aas/mw/com/message_passing/message.h"
#include <amp_callback.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <ostream>

namespace bmw
{
namespace mw
{
namespace com
{
namespace message_passing
{

using Byte = char; 

enum class MessageType : Byte
{
    kStopMessage = 0x00,
    kShortMessage = 0x42,
    kMediumMessage = 0x43,
};

constexpr std::size_t GetMaxMessageSize()
{
    constexpr std::size_t medium_message_size =
        sizeof(MessageType) + sizeof(BaseMessage::id) + sizeof(BaseMessage::pid) + sizeof(MediumMessage::payload);

    return medium_message_size;
}

using RawMessageBuffer = std::array<Byte, GetMaxMessageSize()>;

inline constexpr std::uint32_t GetMessagePriority()
{
    return std::uint32_t{0};
}

/// \details The serialization format for our short message on the queue looks like this:
/// \note serialization format of our medium message looks the same, except, that the payload length is 8.
///
/// +------------+----------+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+
/// |   Byte 0   |  Byte 1  | Byte 2 | Byte 3 | Byte 4 | Byte 5 | Byte 6 | Byte 7 | Byte 8 | Byte 9 | Byte 10| Byte 11|
/// +------------+----------+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+
/// | Msg. Type  | Mesg. ID |          PID of Sender            |       Message Payload             |        N/A      |
/// +------------+----------+-----------------------------------+-----------------------------------+-----------------+

inline constexpr std::size_t GetMessageTypePosition()
{
    return std::size_t{0};
}

inline constexpr std::size_t GetMessageIdPosition()
{
    return std::size_t{1};
}

inline constexpr std::size_t GetMessagePidPosition()
{
    return std::size_t{2};
}

inline constexpr std::size_t GetMessageStartPayload()
{
    return GetMessagePidPosition() + sizeof(pid_t);
}

using LogFunction = amp::callback<void(std::ostream&), 128UL>;

using LoggingCallback = amp::callback<void(const LogFunction)>;

void DefaultLoggingCallback(const LogFunction);

}  // namespace message_passing
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_MESSAGE_PASSING_SHARED_PROPERTIES_H
