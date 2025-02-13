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


#ifndef PLATFORM_AAS_MW_COM_MESSAGE_PASSING_I_RECEIVER_H
#define PLATFORM_AAS_MW_COM_MESSAGE_PASSING_I_RECEIVER_H

#include "platform/aas/lib/os/errno.h"
#include "platform/aas/mw/com/message_passing/shared_properties.h"

#include <amp_callback.hpp>
#include <amp_expected.hpp>

namespace bmw
{
namespace mw
{
namespace com
{
namespace message_passing
{

/// \brief Interface of a Message Passing Receiver, which can be used to receive Messages from a uni-directional
///        channel.
/// \details IReceiver foresees overloads for Register() for differently sized messages. For further explanation about
///          message size overloads check explanation in ISender.
/// \see ISender
class IReceiver
{
  public:
    using ShortMessageReceivedCallback = amp::callback<void(const ShortMessagePayload, const pid_t)>;
    using MediumMessageReceivedCallback = amp::callback<void(const MediumMessagePayload, const pid_t)>;
    virtual ~IReceiver() = default;

    /// \brief Registers short messages within the Receiver for reception
    /// \param id The ID of the message, once received respective callback will be invoked. IDs must be unique across
    ///        short and medium messages.
    /// \param callback The callback of the message which will be invoked
    /// \pre Must not be called after StartListing() has been invoked (thread-race!)
    virtual void Register(const MessageId id, ShortMessageReceivedCallback callback) = 0;

    /// \brief Registers medium sized messages within the Receiver for reception
    /// \param id The ID of the message, once received respective callback will be invoked. IDs must be unique across
    //         short and medium messages.
    /// \param callback The callback of the message which will be invoked
    /// \pre Must not be called after StartListing() has been invoked (thread-race!)
    
    /* False positive: type is identical with function signatures in derived classes. */
    virtual void Register(const MessageId id, MediumMessageReceivedCallback callback) = 0;
    

    /// \brief Opens the underlying communication channel and listens for messages
    /// \post Must not call Register() afterwards! (thread-race)
    virtual amp::expected_blank<bmw::os::Error> StartListening() = 0;
};

}  // namespace message_passing
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_MESSAGE_PASSING_I_RECEIVER_H
