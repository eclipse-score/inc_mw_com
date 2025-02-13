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


#ifndef PLATFORM_AAS_MW_COM_MESSAGE_PASSING_SHORT_MESSAGE_H
#define PLATFORM_AAS_MW_COM_MESSAGE_PASSING_SHORT_MESSAGE_H

#include <array>
#include <cstdint>
#include <type_traits>

namespace bmw
{
namespace mw
{
namespace com
{
namespace message_passing
{

/// \brief Identifies a message
using MessageId = std::int8_t;

/// \brief Represents the payload that is transmitted via a ShortMessage
using ShortMessagePayload = std::uint64_t;

/// \brief Represents the payload that is transmitted via a MediumMessage
using MediumMessagePayload = std::array<uint8_t, 16>;

static_assert(std::is_trivially_copyable<pid_t>::value, "pid_t must be TriviallyCopyable");
static_assert(std::is_trivially_copyable<MessageId>::value, "MessageId must be TriviallyCopyable");
static_assert(std::is_trivially_copyable<ShortMessagePayload>::value, "ShortMessagePayload must be TriviallyCopyable");
static_assert(std::is_trivially_copyable<MediumMessagePayload>::value,
              "MediumMessagePayload must be TriviallyCopyable");

/// \brief Base class of all messages, containing common members.
///
/// \details The pid value depends on the context. If a message is sent, it shall contain the PID of the target process.
/// I.e. the caller of Sender::Send() shall fill in the PID before. In case of reception of a message, the Receiver
/// shall fill in the PID from where he received the message, before calling the registered handler.
///
/// It depends on the OS specific implementation/optimization of Sender/Receiver, whether the PID has to be transmitted
/// explicitly (therefore extending the payload) or whether it is transmitted implicitly. E.g., when using QNX messaging
/// mechanisms, the PID gets transmitted implicitly!
/// In case a message gets received
struct BaseMessage
{
    MessageId id{};
    pid_t pid{-1};
};

/// \brief A ShortMessage shall be used for Inter Process Communication that is Asynchronous and acts as control
/// mechanism.
///
/// \details Different operating systems can implement short messages very efficiently. This is given by the fact that
/// such messages fit into register spaces on the CPU and copying the information is highly efficient. On QNX this is
/// for example the case with Pulses. By providing such an interface, we make it easy for our applications to exchange
/// data as efficient as possible. It shall be noted that no real data shall be transferred over this communication
/// method, it shall rather act as a way to control or notify another process. The efficiency is gained by not providing
/// strong typing, meaning the payload needs to be serialized by the user of the API. If you are searching for a strong
/// typed interface, please consider our mw::com implementation (or ARA::COM).

// The AUTOSAR Rule A11-0-2 prohibits struct inheritence, and rule is violated here in order to create a hierarchy
// where ShortMessage and MediumMessage are siblings with common BaseMessage parent to organize common data.
// ShortMessage type provides public independent access to its members in order to implement deserialization as
// std::memcpy() into data members, which requires type to be a semantic struct.
// WARNING: ShortMessage is struct but it is neither PODType nor TrivialType nor StandardLayoutType! Do not apply
// std::memcpy() on it or any other operations that are not member access! Note, that it is still TriviallyCopyable.
// NOLINTNEXTLINE(bmw-struct-usage-compliance): Inheritence is intended while ShortMessage has to be struct
struct ShortMessage : BaseMessage
{
    ShortMessagePayload payload{};
};


static_assert(std::is_trivially_copyable<ShortMessage>::value, "ShortMessage type must be TriviallyCopyable");

/// \brief A MediumMessage shall be used for Inter Process Communication that is Asynchronous and acts as control
/// mechanism.
///
/// \details Opposed to short messages, the medium size message might not be implemented that efficiently on various OS.
/// Its size being double that of short-messages might hinder solutions, where message payload gets exchanged only via
/// registers after a context switch. Still the payload size is small enough, that no (heap) memory allocation takes
/// place.
/// Introduction of medium messages was driven by some LoLa needs, where short messages weren't sufficient.

// The AUTOSAR Rule A11-0-2 prohibits struct inheritence, and rule is violated here in order to create a hierarchy
// where ShortMessage and MediumMessage are siblings with common BaseMessage parent to organize common data.
// MediumMessage type provides public independent access to its members in order to implement deserialization as
// std::memcpy() into data members, which requires type to be a semantic struct.
// WARNING: MediumMessage is struct but it is neither PODType nor TrivialType nor StandardLayoutType! Do not apply
// std::memcpy() on it or any other operations that are not member access! Note, that it is still TriviallyCopyable.
// NOLINTNEXTLINE(bmw-struct-usage-compliance): Inheritence is intended while MediumMessage has to be struct
struct MediumMessage : BaseMessage
{
    MediumMessagePayload payload{};
};


static_assert(std::is_trivially_copyable<MediumMessage>::value, "MediumMessage type must be TriviallyCopyable");

}  // namespace message_passing
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_MESSAGE_PASSING_SHORT_MESSAGE_H
