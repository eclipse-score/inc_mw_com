// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#ifndef PLATFORM_AAS_MW_COM_MESSAGE_PASSING_SERIALIZER_H
#define PLATFORM_AAS_MW_COM_MESSAGE_PASSING_SERIALIZER_H

#include "platform/aas/mw/com/message_passing/message.h"
#include "platform/aas/mw/com/message_passing/shared_properties.h"

namespace bmw::mw::com::message_passing
{

/// \brief Serializes a ShortMessage into a buffer to transmit it (not considering byte-order)
/// \return RawMessageBuffer that contains the serialized message
RawMessageBuffer SerializeToRawMessage(const ShortMessage& message) noexcept;

/// \brief Serializes a MediumMessage into a buffer to transmit it (not considering byte-order)
/// \return RawMessageBuffer that contains the serialized message
RawMessageBuffer SerializeToRawMessage(const MediumMessage& message) noexcept;

/// \brief Deserializes a buffer into a ShortMessage
/// \return ShortMessage build from provided buffer
ShortMessage DeserializeToShortMessage(const RawMessageBuffer& buffer) noexcept;

/// \brief Deserializes a buffer into a MediumMessage
/// \return MediumMessage build from provided buffer
MediumMessage DeserializeToMediumMessage(const RawMessageBuffer& buffer) noexcept;

}  // namespace bmw::mw::com::message_passing

#endif  // PLATFORM_AAS_MW_COM_MESSAGE_PASSING_SERIALIZER_H
