// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#ifndef PLATFORM_AAS_MW_COM_MESSAGE_PASSING_SENDER_TRAITS_MOCK_H
#define PLATFORM_AAS_MW_COM_MESSAGE_PASSING_SENDER_TRAITS_MOCK_H

#include "platform/aas/mw/com/message_passing/message.h"
#include "platform/aas/mw/com/message_passing/sender.h"

#include <amp_expected.hpp>
#include <amp_memory.hpp>
#include <amp_string_view.hpp>

#include "gmock/gmock.h"

#include <iostream>

namespace bmw
{
namespace mw
{
namespace com
{
namespace message_passing
{
/// \brief This interface class is for testing purposes only.
///        In fact it helps to mock template parameter of \c Sender, which utilizes static methods of the "trait"
///        parameter. Since GTest doesn't support mocking of static methods the passed
///        \c ForwardingSenderChannelTraits as a template trait parameter utilizes its mockable implementation.

class IForwardingSenderChannelTraits
{
  public:
    IForwardingSenderChannelTraits() {}
    virtual ~IForwardingSenderChannelTraits() {}

    using file_descriptor_type = int;
    using FileDescriptorResourcesType = int;

    virtual amp::expected<file_descriptor_type, bmw::os::Error> try_open(
        const amp::string_view identifier,
        const FileDescriptorResourcesType& os_resources) noexcept = 0;

    virtual void close_sender(const file_descriptor_type file_descriptor,
                              const FileDescriptorResourcesType& os_resources) noexcept = 0;

    virtual RawMessageBuffer prepare_payload(const ShortMessage& message) noexcept = 0;

    virtual RawMessageBuffer prepare_payload(const MediumMessage& message) noexcept = 0;

    virtual amp::expected_blank<bmw::os::Error> try_send(const file_descriptor_type file_descriptor,
                                                         const RawMessageBuffer& buffer,
                                                         const FileDescriptorResourcesType& os_resources) noexcept = 0;

    virtual bool has_non_blocking_guarantee() noexcept = 0;
};

class ForwardingSenderChannelTraits
{
  public:
    ForwardingSenderChannelTraits() {}
    ~ForwardingSenderChannelTraits() {}

    static IForwardingSenderChannelTraits* impl_;

    using file_descriptor_type = IForwardingSenderChannelTraits::file_descriptor_type;
    using FileDescriptorResourcesType = IForwardingSenderChannelTraits::FileDescriptorResourcesType;
    static constexpr file_descriptor_type INVALID_FILE_DESCRIPTOR{-1};

    static FileDescriptorResourcesType GetDefaultOSResources(amp::pmr::memory_resource* memory_resource)
    {
        amp::ignore = memory_resource;
        return {};
    }

    static amp::expected<file_descriptor_type, bmw::os::Error> try_open(
        const amp::string_view identifier,
        const FileDescriptorResourcesType& os_resources) noexcept
    {
        return Impl()->try_open(identifier, os_resources);
    }

    static void close_sender(const file_descriptor_type file_descriptor,
                             const FileDescriptorResourcesType& os_resources) noexcept
    {
        Impl()->close_sender(file_descriptor, os_resources);
    }

    template <typename FormatMessage>
    static RawMessageBuffer prepare_payload(const FormatMessage& message) noexcept
    {
        return Impl()->prepare_payload(message);
    }

    static amp::expected_blank<bmw::os::Error> try_send(const file_descriptor_type file_descriptor,
                                                        const RawMessageBuffer& buffer,
                                                        const FileDescriptorResourcesType& os_resources) noexcept
    {
        return Impl()->try_send(file_descriptor, buffer, os_resources);
    }

    static bool has_non_blocking_guarantee() noexcept { return Impl()->has_non_blocking_guarantee(); }

  private:
    static void CheckImpl()
    {
        ASSERT_NE(impl_, nullptr)
            << "unset implementation, please set `ForwardingSenderChannelTraits::impl_` before calling methods";
    }

    static IForwardingSenderChannelTraits* Impl()
    {
        CheckImpl();
        return impl_;
    }
};

IForwardingSenderChannelTraits* ForwardingSenderChannelTraits::impl_ = nullptr;

class SenderChannelTraitsMock : public IForwardingSenderChannelTraits
{
  public:
    SenderChannelTraitsMock() {}
    ~SenderChannelTraitsMock() override {}

    MOCK_METHOD((amp::expected<file_descriptor_type, bmw::os::Error>),
                try_open,
                (const amp::string_view identifier, const FileDescriptorResourcesType&),
                (noexcept, override));

    MOCK_METHOD(void,
                close_sender,
                (const file_descriptor_type file_descriptor, const FileDescriptorResourcesType&),
                (noexcept, override));

    MOCK_METHOD(RawMessageBuffer, prepare_payload, (const ShortMessage& message), (noexcept, override));
    MOCK_METHOD(RawMessageBuffer, prepare_payload, (const MediumMessage& message), (noexcept, override));

    MOCK_METHOD((amp::expected_blank<bmw::os::Error>),
                try_send,
                (const file_descriptor_type file_descriptor,
                 const RawMessageBuffer& buffer,
                 const FileDescriptorResourcesType&),
                (noexcept, override));

    MOCK_METHOD(bool, has_non_blocking_guarantee, (), (noexcept, override));
};

class SenderFactoryImplMock final
{
  public:
    static amp::pmr::unique_ptr<bmw::mw::com::message_passing::ISender> Create(
        const amp::string_view identifier,
        const amp::stop_token& token,
        const SenderConfig& sender_config = {},
        LoggingCallback logging_callback = &DefaultLoggingCallback,
        amp::pmr::memory_resource* const memory_resource = amp::pmr::get_default_resource())
    {
        return amp::pmr::make_unique<bmw::mw::com::message_passing::Sender<ForwardingSenderChannelTraits>>(
            memory_resource, identifier, token, sender_config, std::move(logging_callback));
    }
};

}  // namespace message_passing
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_MESSAGE_PASSING_SENDER_TRAITS_MOCK_H
