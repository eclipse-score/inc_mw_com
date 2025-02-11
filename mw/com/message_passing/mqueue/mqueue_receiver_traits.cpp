// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#include "platform/aas/mw/com/message_passing/mqueue/mqueue_receiver_traits.h"

#include "platform/aas/lib/os/unistd.h"

#include <amp_utility.hpp>

namespace bmw
{
namespace mw
{
namespace com
{
namespace message_passing
{

// Only one thread,  () implicitly fulfilled for Mqueue implementation
constexpr std::size_t MqueueReceiverTraits::kConcurrency;

amp::expected<MqueueReceiverTraits::file_descriptor_type, bmw::os::Error> MqueueReceiverTraits::open_receiver(
    const amp::string_view identifier,
    const amp::pmr::vector<uid_t>& allowed_uids,
    const std::int32_t max_number_message_in_queue,
    const FileDescriptorResourcesType& os_resources) noexcept
{
    
    AMP_ASSERT_MESSAGE(IsOsResourcesValid(os_resources), "OS resources are not valid!");
    
    amp::ignore = allowed_uids;
    /* parameter only used in linux testing */
    using OpenFlag = bmw::os::Mqueue::OpenFlag;
    using Perms = bmw::os::Mqueue::ModeFlag;
    constexpr auto flags = OpenFlag::kCreate | OpenFlag::kReadWrite | OpenFlag::kCloseOnExec;
    // We allow access by all processes in the system since mqueues don't support setting ACLs under Linux.
    constexpr auto perms = Perms::kReadUser | Perms::kWriteUser | Perms::kWriteGroup | Perms::kWriteOthers;
    mq_attr queue_attributes{};
    
    queue_attributes.mq_msgsize = static_cast<long>(GetMaxMessageSize());
    queue_attributes.mq_maxmsg = static_cast<long>(max_number_message_in_queue);
    

    const auto& os_stat = os_resources.os_stat;
    const auto& mq = os_resources.mqueue;
    // Temporarily set the umask to 0 to allow for world-accessible queues.
    const auto old_umask = os_stat->umask(bmw::os::Stat::Mode::kNone).value();
    const auto result = mq->mq_open(identifier.data(), flags, perms, &queue_attributes);
    // no error code is returned. And return value is ignored as it is not needed.
    amp::ignore = os_stat->umask(old_umask).value();
    return result;
}

void MqueueReceiverTraits::close_receiver(const MqueueReceiverTraits::file_descriptor_type file_descriptor,
                                          const amp::string_view identifier,
                                          const FileDescriptorResourcesType& os_resources) noexcept
{
    
    AMP_ASSERT_MESSAGE(IsOsResourcesValid(os_resources), "OS resources are not valid!");
    
    amp::ignore = os_resources.mqueue->mq_close(file_descriptor);
    amp::ignore = os_resources.mqueue->mq_unlink(identifier.data());
    amp::ignore = os_resources.unistd->unlink(identifier.data());
}

void MqueueReceiverTraits::stop_receive(const MqueueReceiverTraits::file_descriptor_type file_descriptor,
                                        const FileDescriptorResourcesType& os_resources) noexcept
{
    
    AMP_ASSERT_MESSAGE(IsOsResourcesValid(os_resources), "OS resources are not valid!");
    
    constexpr auto message = static_cast<std::underlying_type<MessageType>::type>(MessageType::kStopMessage);
    amp::ignore = os_resources.mqueue->mq_send(file_descriptor, &message, sizeof(MessageType), GetMessagePriority());
}

bool MqueueReceiverTraits::IsOsResourcesValid(const FileDescriptorResourcesType& os_resources) noexcept
{
    return ((os_resources.unistd != nullptr) && (os_resources.mqueue != nullptr)) && (os_resources.os_stat != nullptr);
}

}  // namespace message_passing
}  // namespace com
}  // namespace mw
}  // namespace bmw
