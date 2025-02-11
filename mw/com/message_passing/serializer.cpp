// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#include "platform/aas/mw/com/message_passing/serializer.h"

#include "platform/aas/mw/com/message_passing/shared_properties.h"

#include <amp_utility.hpp>

#include <cstring>

namespace
{
void SerializeMessageId(bmw::mw::com::message_passing::RawMessageBuffer& buffer,
                        const bmw::mw::com::message_passing::MessageId& message_id)
{
    // Serialization implies operations with raw memory using std::memcpy(). The operand being serialized
    // must comply with TriviallyCopyable which it does. The destination buffer is just a place for bytes.
    // No overlapping is guaranteed by fact that we take addresses of two different identifiers.
    // We ignore the return of memcpy because we do not need it and does not carry any error information
    // NOLINTNEXTLINE(bmw-banned-function): std::memcpy() of TriviallyCopyable type and no overlap
    amp::ignore = std::memcpy(static_cast<void*>(&buffer.at(bmw::mw::com::message_passing::GetMessageIdPosition())),
                              static_cast<const void*>(&message_id),
                              sizeof(message_id));
}
bmw::mw::com::message_passing::MessageId DeserializeMessageId(
    const bmw::mw::com::message_passing::RawMessageBuffer& buffer)
{
    bmw::mw::com::message_passing::MessageId message_id{};
    // Serialization implies operations with raw memory using std::memcpy(). The operand being deserialized
    // must comply with TriviallyCopyable which it does. The source buffer is just raw bytes.
    // No overlapping is guaranteed by fact that we take addresses of two different identifiers.
    // We ignore the return of memcpy because we do not need it and does not carry any error information
    amp::ignore =
        // NOLINTNEXTLINE(bmw-banned-function): std::memcpy() of TriviallyCopyable type and no overlap
        std::memcpy(static_cast<void*>(&message_id),
                    static_cast<const void*>(&buffer.at(bmw::mw::com::message_passing::GetMessageIdPosition())),
                    sizeof(message_id));
    return message_id;
}
}  // namespace

auto bmw::mw::com::message_passing::SerializeToRawMessage(const ShortMessage& message) noexcept -> RawMessageBuffer
{
    constexpr auto MESSAGE_END_PAYLOAD = GetMessageStartPayload() + sizeof(ShortMessage::payload);
    static_assert(MESSAGE_END_PAYLOAD <= std::tuple_size<RawMessageBuffer>::value,
                  "RawMessageBuffer to small for short message, unsafe memory operation!");

    RawMessageBuffer buffer{};
    
    /* We explicitly neither want signed/unsigned byte for the RawMessageBuffer type. */
    buffer.at(GetMessageTypePosition()) =
        static_cast<std::underlying_type<MessageType>::type>(MessageType::kShortMessage);
    

    SerializeMessageId(buffer, message.id);

    // Serialization implies operations with raw memory using std::memcpy(). The operand being serialized
    // must comply with TriviallyCopyable which it does. The destination buffer is just a place for bytes.
    // No overlapping is guaranteed by fact that we take addresses of two different identifiers.
    // We ignore the return of memcpy because we do not need it and does not carry any error information
    // NOLINTNEXTLINE(bmw-banned-function): std::memcpy() of TriviallyCopyable type and no overlap
    amp::ignore = std::memcpy(static_cast<void*>(&buffer.at(GetMessagePidPosition())),
                              static_cast<const void*>(&message.pid),
                              sizeof(message.pid));

    // Serialization implies operations with raw memory using std::memcpy(). The operand being serialized
    // must comply with TriviallyCopyable which it does. The destination buffer is just a place for bytes.
    // No overlapping is guaranteed by fact that we take addresses of two different identifiers.
    // NOLINTNEXTLINE(bmw-banned-function): std::memcpy() of TriviallyCopyable type and no overlap
    amp::ignore = std::memcpy(static_cast<void*>(&buffer.at(GetMessageStartPayload())),
                              static_cast<const void*>(&message.payload),
                              sizeof(message.payload));

    return buffer;
}

auto bmw::mw::com::message_passing::SerializeToRawMessage(const MediumMessage& message) noexcept -> RawMessageBuffer
{
    
    constexpr auto MESSAGE_END_PAYLOAD = GetMessageStartPayload() + sizeof(MediumMessage::payload);
    
    static_assert(MESSAGE_END_PAYLOAD <= std::tuple_size<RawMessageBuffer>::value,
                  "RawMessageBuffer to small for medium message, unsafe memory operation!");

    RawMessageBuffer buffer{};
    
    /* We explicitly neither want signed/unsigned byte for the RawMessageBuffer type. */
    buffer[GetMessageTypePosition()] =
        static_cast<std::underlying_type<MessageType>::type>(MessageType::kMediumMessage);
    

    SerializeMessageId(buffer, message.id);

    
    // memcpy doesn't return error code.
    // Serialization implies operations with raw memory using std::memcpy(). The operand being serialized
    // must comply with TriviallyCopyable which it does. The destination buffer is just a place for bytes.
    // No overlapping is guaranteed by fact that we take addresses of two different identifiers.
    // NOLINTNEXTLINE(bmw-banned-function): std::memcpy() of TriviallyCopyable type and no overlap
    amp::ignore = std::memcpy(static_cast<void*>(&buffer.at(GetMessagePidPosition())),
                              static_cast<const void*>(&message.pid),
                              sizeof(message.pid));

    // Serialization implies operations with raw memory using std::memcpy(). The operand being serialized
    // must comply with TriviallyCopyable which it does. The destination buffer is just a place for bytes.
    // No overlapping is guaranteed by fact that we take addresses of two different identifiers.
    // NOLINTNEXTLINE(bmw-banned-function): std::memcpy() of TriviallyCopyable type and no overlap
    amp::ignore = std::memcpy(static_cast<void*>(&buffer.at(GetMessageStartPayload())),
                              static_cast<const void*>(&message.payload),
                              sizeof(message.payload));
    
    return buffer;
}

auto bmw::mw::com::message_passing::DeserializeToShortMessage(const RawMessageBuffer& buffer) noexcept -> ShortMessage
{
    ShortMessage message{};
    message.id = DeserializeMessageId(buffer);

    
    // memcpy doesn't return error code.
    // Serialization implies operations with raw memory using std::memcpy(). The operand being deserialized
    // must comply with TriviallyCopyable which it does. The source buffer is just raw bytes.
    // No overlapping is guaranteed by fact that we take addresses of two different identifiers.
    // NOLINTNEXTLINE(bmw-banned-function): std::memcpy() of TriviallyCopyable type and no overlap
    amp::ignore = std::memcpy(static_cast<void*>(&message.pid),
                              static_cast<const void*>(&buffer.at(GetMessagePidPosition())),
                              sizeof(ShortMessage::pid));

    // Serialization implies operations with raw memory using std::memcpy(). The operand being deserialized
    // must comply with TriviallyCopyable which it does. The source buffer is just raw bytes.
    // No overlapping is guaranteed by fact that we take addresses of two different identifiers.
    // NOLINTNEXTLINE(bmw-banned-function): std::memcpy() of TriviallyCopyable type and no overlap
    amp::ignore = std::memcpy(static_cast<void*>(&message.payload),
                              static_cast<const void*>(&buffer.at(GetMessageStartPayload())),
                              sizeof(ShortMessage::payload));
    
    return message;
}

auto bmw::mw::com::message_passing::DeserializeToMediumMessage(const RawMessageBuffer& buffer) noexcept -> MediumMessage
{
    MediumMessage message{};
    message.id = DeserializeMessageId(buffer);

    
    // memcpy doesn't return error code.
    // Serialization implies operations with raw memory using std::memcpy(). The operand being deserialized
    // must comply with TriviallyCopyable which it does. The source buffer is just raw bytes.
    // No overlapping is guaranteed by fact that we take addresses of two different identifiers.
    // NOLINTNEXTLINE(bmw-banned-function): std::memcpy() of TriviallyCopyable type and no overlap
    amp::ignore = std::memcpy(static_cast<void*>(&message.pid),
                              static_cast<const void*>(&buffer.at(GetMessagePidPosition())),
                              sizeof(MediumMessage::pid));

    // Serialization implies operations with raw memory using std::memcpy(). The operand being deserialized
    // must comply with TriviallyCopyable which it does. The source buffer is just raw bytes.
    // No overlapping is guaranteed by fact that we take addresses of two different identifiers.
    // NOLINTBEGIN(bmw-banned-function): std::memcpy() of TriviallyCopyable type and no overlap

    amp::ignore = std::memcpy(static_cast<void*>(&message.payload),
                              static_cast<const void*>(&buffer.at(GetMessageStartPayload())),
                              sizeof(MediumMessage::payload));
    // NOLINTEND(bmw-banned-function): see above for detailed explanation
    
    return message;
}
