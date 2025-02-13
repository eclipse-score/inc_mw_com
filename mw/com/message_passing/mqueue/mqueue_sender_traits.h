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


#ifndef PLATFORM_AAS_MW_COM_MESSAGE_PASSING_MQUEUE_SENDER_TRAITS_H
#define PLATFORM_AAS_MW_COM_MESSAGE_PASSING_MQUEUE_SENDER_TRAITS_H

#include "platform/aas/mw/com/message_passing/message.h"
#include "platform/aas/mw/com/message_passing/serializer.h"
#include "platform/aas/mw/com/message_passing/shared_properties.h"

#include "platform/aas/lib/os/errno.h"
#include "platform/aas/lib/os/mqueue.h"

#include <amp_assert.hpp>
#include <amp_expected.hpp>
#include <amp_string_view.hpp>
#include <amp_utility.hpp>

#include <cstdint>

namespace bmw
{
namespace mw
{
namespace com
{
namespace message_passing
{

class MqueueSenderTraits
{
  public:
    using file_descriptor_type = mqd_t;
    static constexpr file_descriptor_type INVALID_FILE_DESCRIPTOR{-1};

    struct OsResources
    {
        amp::pmr::unique_ptr<bmw::os::Mqueue> mqueue{};
    };

    using FileDescriptorResourcesType = OsResources;

    static OsResources GetDefaultOSResources(amp::pmr::memory_resource* memory_resource) noexcept;

    static amp::expected<file_descriptor_type, bmw::os::Error> try_open(
        const amp::string_view identifier,
        const FileDescriptorResourcesType& os_resources) noexcept;

    static void close_sender(const file_descriptor_type file_descriptor,
                             const FileDescriptorResourcesType& os_resources) noexcept;

    template <typename MessageFormat>
    static bmw::mw::com::message_passing::RawMessageBuffer prepare_payload(const MessageFormat& message) noexcept
    {
        return SerializeToRawMessage(message);
    }

    static amp::expected_blank<bmw::os::Error> try_send(const file_descriptor_type file_descriptor,
                                                        const bmw::mw::com::message_passing::RawMessageBuffer& buffer,
                                                        const FileDescriptorResourcesType& os_resources) noexcept
    {
        
        AMP_ASSERT_MESSAGE(IsOsResourcesValid(os_resources), "OS resources are not valid!");
        
        
        /* False positive: No implicit conversions which changes signedness performed. */
        return os_resources.mqueue->mq_send(
            file_descriptor, buffer.begin(), buffer.size(), bmw::mw::com::message_passing::GetMessagePriority());
        
    }

    /// \brief For POSIX mqueue, we assume strong non-blocking guarantee.
    ///
    /// \details We use mqueue with bmw::os::Mqueue::OpenFlag::kNonBlocking, therefore we assume strong non
    ///          blocking guarantee. The guarantee could only be violated by the OS, but in this case we are dealing
    ///          obviously with a ASIL-QM grade OS anyways, where the safety related notion of this API is already
    ///          broken!
    /// \return true
    static bool has_non_blocking_guarantee() noexcept { return true; }

  private:
    static bool IsOsResourcesValid(const FileDescriptorResourcesType& os_resources) noexcept;
};

}  // namespace message_passing
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_MESSAGE_PASSING_MQUEUE_SENDER_TRAITS_H
