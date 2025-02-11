// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#ifndef PLATFORM_AAS_MW_COM_MESSAGE_PASSING_RECEIVER_FACTORY_IMPL_H
#define PLATFORM_AAS_MW_COM_MESSAGE_PASSING_RECEIVER_FACTORY_IMPL_H

#include "platform/aas/lib/concurrency/executor.h"
#include "platform/aas/mw/com/message_passing/i_receiver.h"
#include "platform/aas/mw/com/message_passing/receiver_config.h"

#include <amp_span.hpp>
#include <amp_string_view.hpp>

#include <cstdint>
#include <memory>
#include <vector>

namespace bmw
{
namespace mw
{
namespace com
{
namespace message_passing
{

/// \brief A platform-specific implementation of IReceiver factory.
class ReceiverFactoryImpl final
{
  public:
    static amp::pmr::unique_ptr<IReceiver> Create(const amp::string_view identifier,
                                                  concurrency::Executor& executor,
                                                  const amp::span<const uid_t> allowed_uids,
                                                  const ReceiverConfig& receiver_config,
                                                  amp::pmr::memory_resource* const memory_resource);
};

}  // namespace message_passing
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_MESSAGE_PASSING_RECEIVER_FACTORY_IMPL_H
