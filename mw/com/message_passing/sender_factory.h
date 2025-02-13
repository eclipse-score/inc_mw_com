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


#ifndef PLATFORM_AAS_MW_COM_MESSAGE_PASSING_SENDER_FACTORY_H
#define PLATFORM_AAS_MW_COM_MESSAGE_PASSING_SENDER_FACTORY_H

#include "platform/aas/mw/com/message_passing/i_sender.h"
#include "platform/aas/mw/com/message_passing/sender_config.h"

#include <amp_memory.hpp>
#include <amp_stop_token.hpp>
#include <amp_string_view.hpp>

#include <cstdint>
#include <memory>

namespace bmw
{
namespace mw
{
namespace com
{
namespace message_passing
{

/// \brief Factory, which creates instances of ISender.
/// \details Factory pattern serves two purposes here: Testability/mockability of senders and alternative
///          implementations of ISender. We initially have a POSIX MQ based implementation of ISender, but specific
///          implementations e.g. for QNX based on specific IPC mechanisms are expected.
class SenderFactory final
{
  public:
    /// \brief Creates an implementation instance of ISender.
    /// \details This is the factory create method for ISender instances. There a specific implementation of the
    ///          platform or a mock instance (see InjectSenderMock()) is returned.
    /// \todo Eventually extend the signature with some configuration parameter, from which factory can deduce, which
    ///       ISender impl. to create. Currently we only have POSIX MQ based ISender impl..
    /// \param identifier some identifier for the sender being created. Depending on the chosen impl. this might be
    ///        used or not.
    /// \param token stop_token to notify a stop request, in case the sender implementation does some long-running/
    ///        async activity.
    /// \param sender_config additional sender configuration parameters
    /// \param logging_callback output method for error messages since we cannot use regular logging (default: cerr)
    /// \param memory_resource memory resource for allocating the Sender object
    /// \return a platform specific implementation of a ISender or a mock.
    static amp::pmr::unique_ptr<ISender> Create(
        const amp::string_view identifier,
        const amp::stop_token& token,
        const SenderConfig& sender_config = {},
        LoggingCallback logging_callback = &DefaultLoggingCallback,
        amp::pmr::memory_resource* const memory_resource = amp::pmr::get_default_resource());

    /// \brief Inject pointer to a mock instance, which shall be returned by all Create() calls.
    /// \param mock
    /// \param amp::callback - to be set and further called before creation of SenderMockWrapper.
    static void InjectSenderMock(
        ISender* const mock,
        // 
        // 
        amp::callback<void(const amp::stop_token&)>&& = [](const amp::stop_token&) noexcept {});

  private:
    static ISender* sender_mock_;
};

}  // namespace message_passing
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_MESSAGE_PASSING_SENDER_FACTORY_H
