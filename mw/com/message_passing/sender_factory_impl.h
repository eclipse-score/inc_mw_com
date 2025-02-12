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


#ifndef PLATFORM_AAS_MW_COM_MESSAGE_PASSING_SENDER_FACTORY_IMPL_H
#define PLATFORM_AAS_MW_COM_MESSAGE_PASSING_SENDER_FACTORY_IMPL_H

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

/// \brief A platform-specific implementation of ISender factory.
class SenderFactoryImpl final
{
  public:
    static amp::pmr::unique_ptr<ISender> Create(
        const amp::string_view identifier,
        const amp::stop_token& token,
        const SenderConfig& sender_config = {},
        LoggingCallback logging_callback = &DefaultLoggingCallback,
        amp::pmr::memory_resource* const memory_resource = amp::pmr::get_default_resource());
};

}  // namespace message_passing
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_MESSAGE_PASSING_SENDER_FACTORY_IMPL_H
