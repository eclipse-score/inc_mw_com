// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#include "platform/aas/mw/com/message_passing/qnx/resmgr_traits_common.h"

#include <amp_assert.hpp>
#include <amp_utility.hpp>

namespace bmw
{
namespace mw
{
namespace com
{
namespace message_passing
{

QnxResourcePath::QnxResourcePath(const amp::string_view identifier) noexcept
    : buffer_{GetQnxPrefix().cbegin(), GetQnxPrefix().cend()}
{
    
    AMP_PRECONDITION(identifier.size() <= kMaxIdentifierLen);
    
    amp::ignore = buffer_.insert(buffer_.cend(), identifier.cbegin(), identifier.cend());
    buffer_.push_back(0);
}

}  // namespace message_passing
}  // namespace com
}  // namespace mw
}  // namespace bmw
