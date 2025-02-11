// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#include "platform/aas/mw/com/message_passing/qnx/resmgr_sender_traits.h"

#include "platform/aas/mw/com/message_passing/qnx/resmgr_traits_common.h"

namespace bmw
{
namespace mw
{
namespace com
{
namespace message_passing
{

using file_descriptor_type = ResmgrSenderTraits::file_descriptor_type;

amp::expected<file_descriptor_type, bmw::os::Error> ResmgrSenderTraits::try_open(
    const amp::string_view identifier,
    const FileDescriptorResourcesType& os_resources) noexcept
{
    AMP_ASSERT_MESSAGE(IsOsResourcesValid(os_resources), "OS resources are not valid!");
    const QnxResourcePath path{identifier};
    // This function in a banned list, however according to the requirement
    //  it's allowed for IPC API that is mw::com (aka LoLa)
    // NOLINTNEXTLINE(bmw-banned-function): See above
    return os_resources.fcntl->open(path.c_str(), bmw::os::Fcntl::Open::kWriteOnly);
}

void ResmgrSenderTraits::close_sender(const file_descriptor_type file_descriptor,
                                      const FileDescriptorResourcesType& os_resources) noexcept
{
    
    AMP_ASSERT_MESSAGE(IsOsResourcesValid(os_resources), "OS resources are not valid!");
    
    // This function in a banned list, however according to the requirement
    //  it's allowed for IPC API that is mw::com (aka LoLa)
    // NOLINTNEXTLINE(bmw-banned-function): See above
    amp::ignore = os_resources.unistd->close(file_descriptor);
}

bool ResmgrSenderTraits::IsOsResourcesValid(const FileDescriptorResourcesType& os_resources) noexcept
{
    return (os_resources.unistd != nullptr) && (os_resources.fcntl != nullptr);
}

ResmgrSenderTraits::OsResources ResmgrSenderTraits::GetDefaultOSResources(
    amp::pmr::memory_resource* memory_resource) noexcept
{
    return {bmw::os::Unistd::Default(memory_resource), bmw::os::Fcntl::Default(memory_resource)};
}

}  // namespace message_passing
}  // namespace com
}  // namespace mw
}  // namespace bmw
