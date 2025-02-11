// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#ifndef PLATFORM_AAS_MW_COM_MESSAGE_PASSING_RECEIVER_MOCK_H
#define PLATFORM_AAS_MW_COM_MESSAGE_PASSING_RECEIVER_MOCK_H

#include "platform/aas/mw/com/message_passing/i_receiver.h"

#include "gmock/gmock.h"

namespace bmw
{
namespace mw
{
namespace com
{
namespace message_passing
{

class ReceiverMock : public IReceiver
{
  public:
    MOCK_METHOD(void, Register, (const MessageId id, ShortMessageReceivedCallback callback), (override));
    MOCK_METHOD(void, Register, (const MessageId id, MediumMessageReceivedCallback callback), (override));
    MOCK_METHOD(amp::expected_blank<bmw::os::Error>, StartListening, (), (override));
};

}  // namespace message_passing
}  // namespace com
}  // namespace mw
}  // namespace bmw
#endif  // PLATFORM_AAS_MW_COM_MESSAGE_PASSING_RECEIVER_MOCK_H
