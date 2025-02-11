// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#ifndef PLATFORM_AAS_MW_COM_MESSAGE_PASSING_SENDER_CONFIG_H
#define PLATFORM_AAS_MW_COM_MESSAGE_PASSING_SENDER_CONFIG_H

#include <chrono>
#include <cstdint>

namespace bmw
{
namespace mw
{
namespace com
{
namespace message_passing
{

/// \brief Configuration parameters for Sender
/// \details Separated into its own data type due to code complexity requirements
struct SenderConfig
{
    /// \brief specifies the maximum number of attempts to send a message (default: 5)
    std::int32_t max_numbers_of_retry = 5;
    /// \brief specifies the delay before retrying to send a message (default: 0 ms)
    std::chrono::milliseconds send_retry_delay = {};
    /// \brief specifies the delay before retrying to connect to the receiver (default: 5 ms)
    std::chrono::milliseconds connect_retry_delay = std::chrono::milliseconds{5};
};

}  // namespace message_passing
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_MESSAGE_PASSING_SENDER_CONFIG_H
