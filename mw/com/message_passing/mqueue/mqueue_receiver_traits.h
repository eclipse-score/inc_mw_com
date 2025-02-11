// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#ifndef PLATFORM_AAS_MW_COM_MESSAGE_PASSING_MQUEUE_RECEIVER_TRAITS_H
#define PLATFORM_AAS_MW_COM_MESSAGE_PASSING_MQUEUE_RECEIVER_TRAITS_H

#include "platform/aas/mw/com/message_passing/message.h"
#include "platform/aas/mw/com/message_passing/serializer.h"
#include "platform/aas/mw/com/message_passing/shared_properties.h"

#include "platform/aas/lib/os/errno.h"
#include "platform/aas/lib/os/mqueue.h"
#include "platform/aas/lib/os/stat.h"
#include "platform/aas/lib/os/unistd.h"

#include <amp_assert.hpp>
#include <amp_expected.hpp>
#include <amp_string_view.hpp>
#include <amp_vector.hpp>

#include <cstdint>

namespace bmw
{
namespace mw
{
namespace com
{
namespace message_passing
{

class MqueueReceiverTraits
{
  public:
    static constexpr std::size_t kConcurrency{1U};
    using file_descriptor_type = mqd_t;
    // 
    static constexpr file_descriptor_type INVALID_FILE_DESCRIPTOR{-1};

    struct OsResources
    {
        amp::pmr::unique_ptr<bmw::os::Unistd> unistd{};
        // 
        amp::pmr::unique_ptr<bmw::os::Mqueue> mqueue{};
        // 
        amp::pmr::unique_ptr<bmw::os::Stat> os_stat{};
    };

    
    static OsResources GetDefaultOSResources(amp::pmr::memory_resource* memory_resource) noexcept
    
    {
        return {bmw::os::Unistd::Default(memory_resource),
                bmw::os::Mqueue::Default(memory_resource),
                bmw::os::Stat::Default(memory_resource)};
    }

    using FileDescriptorResourcesType = OsResources;

    static amp::expected<file_descriptor_type, bmw::os::Error> open_receiver(
        const amp::string_view identifier,
        const amp::pmr::vector<uid_t>& allowed_uids,
        const std::int32_t max_number_message_in_queue,
        const FileDescriptorResourcesType& os_resources) noexcept;

    static void close_receiver(const file_descriptor_type file_descriptor,
                               const amp::string_view identifier,
                               const FileDescriptorResourcesType& os_resources) noexcept;

    static void stop_receive(const file_descriptor_type file_descriptor,
                             const FileDescriptorResourcesType& os_resources) noexcept;

    template <typename ShortMessageProcessor, typename MediumMessageProcessor>
    static amp::expected<bool, bmw::os::Error> receive_next(const file_descriptor_type file_descriptor,
                                                            std::size_t thread,
                                                            ShortMessageProcessor fShort,
                                                            MediumMessageProcessor fMedium,
                                                            const FileDescriptorResourcesType& os_resources)
    {
        
        AMP_ASSERT_MESSAGE(IsOsResourcesValid(os_resources), "OS resources are not valid!");
        
        amp::ignore = thread;  // Ignoring for now to avoid MISRA:FUNC:UNUSEDPAR.UNNAMED

        std::uint32_t message_priority{0U};
        RawMessageBuffer buffer{};
        const auto received =
            os_resources.mqueue->mq_receive(file_descriptor, buffer.begin(), buffer.size(), &message_priority);
        if (received.has_value())
        {
            switch (buffer.at(GetMessageTypePosition()))
            {
                case static_cast<std::underlying_type_t<MessageType>>(MessageType::kStopMessage):
                    return false;
                // 
                case static_cast<std::underlying_type_t<MessageType>>(MessageType::kShortMessage):
                {
                    const auto message = DeserializeToShortMessage(buffer);
                    fShort(message);
                    return true;
                }
                // 
                case static_cast<std::underlying_type_t<MessageType>>(MessageType::kMediumMessage):
                {
                    const auto message = DeserializeToMediumMessage(buffer);
                    fMedium(message);
                    return true;
                }
                // 
                default:
                    // ignore request from a misbehaving client
                    return true;
            }
        }
        else
        {
            return amp::make_unexpected(received.error());
        }
    }

  private:
    static bool IsOsResourcesValid(const FileDescriptorResourcesType& os_resources) noexcept;
};

}  // namespace message_passing
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_MESSAGE_PASSING_MQUEUE_RECEIVER_TRAITS_H
