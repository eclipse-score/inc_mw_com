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


#ifndef PLATFORM_AAS_MW_COM_MESSAGE_PASSING_RECEIVER_CONFIG_H
#define PLATFORM_AAS_MW_COM_MESSAGE_PASSING_RECEIVER_CONFIG_H

#include <amp_optional.hpp>
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

/// \brief Configuration parameters for Receiver
/// \details Separated into its own data type due to code complexity requirements
struct ReceiverConfig
{
    /// \brief specifies the maximum number of messages kept in the receiver queue
    std::int32_t max_number_message_in_queue = 10;
    /// \brief artificially throttles the receiver message loop to limit the processing rate of incoming messages
    amp::optional<std::chrono::milliseconds> message_loop_delay = amp::nullopt;
};

}  // namespace message_passing
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_MESSAGE_PASSING_RECEIVER_CONFIG_H
