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


#ifndef PLATFORM_AAS_MW_COM_MESSAGE_PASSING_SENDER_MOCK_H
#define PLATFORM_AAS_MW_COM_MESSAGE_PASSING_SENDER_MOCK_H

#include "platform/aas/mw/com/message_passing/i_sender.h"

#include "gmock/gmock.h"

namespace bmw
{
namespace mw
{
namespace com
{
namespace message_passing
{

class SenderMock : public ISender
{
  public:
    MOCK_METHOD(amp::expected_blank<bmw::os::Error>, Send, (const ShortMessage&), (noexcept, override));
    MOCK_METHOD(amp::expected_blank<bmw::os::Error>, Send, (const MediumMessage&), (noexcept, override));
    MOCK_METHOD(bool, HasNonBlockingGuarantee, (), (const, noexcept, override));
};

}  // namespace message_passing
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_MESSAGE_PASSING_SENDER_MOCK_H
