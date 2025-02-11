// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#include "platform/aas/mw/com/message_passing/mqueue/mqueue_sender_traits.h"

namespace bmw
{
namespace mw
{
namespace com
{
namespace message_passing
{

amp::expected<MqueueSenderTraits::file_descriptor_type, bmw::os::Error> MqueueSenderTraits::try_open(
    const amp::string_view identifier,
    const FileDescriptorResourcesType& os_resources) noexcept
{
    
    AMP_ASSERT_MESSAGE(IsOsResourcesValid(os_resources), "OS resources are not valid!");
    
    return os_resources.mqueue->mq_open(
        identifier.data(), bmw::os::Mqueue::OpenFlag::kWriteOnly | bmw::os::Mqueue::OpenFlag::kNonBlocking);
}

void MqueueSenderTraits::close_sender(const MqueueSenderTraits::file_descriptor_type file_descriptor,
                                      const FileDescriptorResourcesType& os_resources) noexcept
{
    
    AMP_ASSERT_MESSAGE(IsOsResourcesValid(os_resources), "OS resources are not valid!");
    
    amp::ignore = os_resources.mqueue->mq_close(file_descriptor);
}

bool MqueueSenderTraits::IsOsResourcesValid(const FileDescriptorResourcesType& os_resources) noexcept
{
    return os_resources.mqueue != nullptr;
}


MqueueSenderTraits::OsResources MqueueSenderTraits::GetDefaultOSResources(
    amp::pmr::memory_resource* memory_resource) noexcept

{
    return {bmw::os::Mqueue::Default(memory_resource)};
}

}  // namespace message_passing
}  // namespace com
}  // namespace mw
}  // namespace bmw
