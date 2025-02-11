// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#ifndef PLATFORM_AAS_MW_COM_MESSAGE_PASSING_RESMGR_SENDER_TRAITS_H
#define PLATFORM_AAS_MW_COM_MESSAGE_PASSING_RESMGR_SENDER_TRAITS_H

#include "platform/aas/lib/os/fcntl.h"
#include "platform/aas/lib/os/unistd.h"

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

class ResmgrSenderTraits
{
  public:
    using file_descriptor_type = int32_t;
    static constexpr file_descriptor_type INVALID_FILE_DESCRIPTOR{-1};

    struct OsResources
    {
        amp::pmr::unique_ptr<bmw::os::Unistd> unistd{};
        amp::pmr::unique_ptr<bmw::os::Fcntl> fcntl{};
    };

    using FileDescriptorResourcesType = OsResources;

    static OsResources GetDefaultOSResources(amp::pmr::memory_resource* memory_resource) noexcept;

    static amp::expected<file_descriptor_type, bmw::os::Error> try_open(
        const amp::string_view identifier,
        const FileDescriptorResourcesType& os_resources) noexcept;

    static void close_sender(const file_descriptor_type file_descriptor,
                             const FileDescriptorResourcesType& os_resources) noexcept;

    template <typename MessageFormat>
    static const MessageFormat& prepare_payload(const MessageFormat& message) noexcept
    {
        return message;
    }

    template <typename MessageFormat>
    static amp::expected_blank<bmw::os::Error> try_send(const file_descriptor_type file_descriptor,
                                                        const MessageFormat& message,
                                                        const FileDescriptorResourcesType& os_resources) noexcept
    {
        
        AMP_ASSERT_MESSAGE(IsOsResourcesValid(os_resources), "OS resources are not valid!");
        
        // This function in a banned list, however according to the requirement
        //  it's allowed for IPC API that is mw::com (aka LoLa)
        // NOLINTNEXTLINE(bmw-banned-function): See above
        const auto result = os_resources.unistd->write(file_descriptor, &message, sizeof(message));
        if (!result.has_value())
        {
            return amp::make_unexpected(result.error());
        }
        return {};
    }

    static constexpr bool has_non_blocking_guarantee() noexcept { return false; }

  private:
    static bool IsOsResourcesValid(const FileDescriptorResourcesType& os_resources) noexcept;
};

}  // namespace message_passing
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_MESSAGE_PASSING_RESMGR_SENDER_TRAITS_H
