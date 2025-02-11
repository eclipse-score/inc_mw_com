// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#ifndef PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_MESSAGING_MESSAGE_PASSING_SERVICE_MOCK_H
#define PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_MESSAGING_MESSAGE_PASSING_SERVICE_MOCK_H

#include "platform/aas/mw/com/impl/bindings/lola/messaging/i_message_passing_service.h"

#include <gmock/gmock.h>

namespace bmw
{
namespace mw
{
namespace com
{
namespace impl
{
namespace lola
{

class MessagePassingServiceMock : public IMessagePassingService
{
  public:
    MOCK_METHOD(void, NotifyEvent, (QualityType, ElementFqId), (override));
    MOCK_METHOD(HandlerRegistrationNoType,
                RegisterEventNotification,
                (QualityType, ElementFqId, BindingEventReceiveHandler, pid_t),
                (override));
    MOCK_METHOD(void, ReregisterEventNotification, (QualityType, ElementFqId, pid_t), (override));
    MOCK_METHOD(void,
                UnregisterEventNotification,
                (QualityType, ElementFqId, HandlerRegistrationNoType, pid_t),
                (override));
    MOCK_METHOD(void, NotifyOutdatedNodeId, (QualityType, pid_t, pid_t), (override));
};

}  // namespace lola
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_MESSAGING_MESSAGE_PASSING_SERVICE_MOCK_H
