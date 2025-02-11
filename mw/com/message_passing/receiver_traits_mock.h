// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#ifndef PLATFORM_AAS_MW_COM_MESSAGE_PASSING_RECEIVER_TRAITS_MOCK_H
#define PLATFORM_AAS_MW_COM_MESSAGE_PASSING_RECEIVER_TRAITS_MOCK_H

#include "platform/aas/mw/com/message_passing/receiver.h"

#include <amp_expected.hpp>
#include <amp_string_view.hpp>
#include <amp_vector.hpp>

#include "gmock/gmock.h"

namespace bmw
{
namespace mw
{
namespace com
{
namespace message_passing
{

/// \brief This interface class is for testing purposes only.
///        In fact it helps to mock template parameter of \c Receiver, which utilizes static methods of the "trait"
///        parameter. Since GTest doesn't support mocking of static methods the passed
///        \c ForwardingReceiverChannelTraits as a template trait parameter utilizes its mockable implementation.

class IForwardingReceiverChannelTraits
{
  public:
    IForwardingReceiverChannelTraits() = default;
    virtual ~IForwardingReceiverChannelTraits() = default;

    using file_descriptor_type = int;
    using FileDescriptorResourcesType = int;

    virtual amp::expected<file_descriptor_type, bmw::os::Error> open_receiver(
        const amp::string_view identifier,
        const amp::pmr::vector<uid_t>& allowed_uids,
        const std::int32_t max_number_message_in_queue,
        const FileDescriptorResourcesType& os_resources) noexcept = 0;

    virtual void close_receiver(const file_descriptor_type file_descriptor,
                                const amp::string_view identifier,
                                const FileDescriptorResourcesType& os_resources) noexcept = 0;

    virtual void stop_receive(const file_descriptor_type file_descriptor,
                              const FileDescriptorResourcesType& os_resources) noexcept = 0;

    virtual amp::expected<bool, bmw::os::Error> receive_next(
        const file_descriptor_type file_descriptor,
        const std::size_t thread,
        std::function<void(ShortMessage)> fShort,
        std::function<void(MediumMessage)> fMedium,
        const FileDescriptorResourcesType& os_resources) noexcept = 0;
};

class ForwardingReceiverChannelTraits
{
  public:
    ForwardingReceiverChannelTraits() = default;
    ~ForwardingReceiverChannelTraits() = default;

    static IForwardingReceiverChannelTraits* impl_;
    static const std::size_t kConcurrency;

    using file_descriptor_type = IForwardingReceiverChannelTraits::file_descriptor_type;
    using FileDescriptorResourcesType = IForwardingReceiverChannelTraits::FileDescriptorResourcesType;
    static constexpr file_descriptor_type INVALID_FILE_DESCRIPTOR{-1};

    static FileDescriptorResourcesType GetDefaultOSResources(amp::pmr::memory_resource* memory_resource)
    {
        amp::ignore = memory_resource;
        return {};
    }

    static amp::expected<file_descriptor_type, bmw::os::Error> open_receiver(
        const amp::string_view identifier,
        const amp::pmr::vector<uid_t>& allowed_uids,
        const std::int32_t max_number_message_in_queue,
        const FileDescriptorResourcesType& os_resources) noexcept
    {
        return Impl()->open_receiver(identifier, allowed_uids, max_number_message_in_queue, os_resources);
    }

    static void close_receiver(const file_descriptor_type file_descriptor,
                               const amp::string_view identifier,
                               const FileDescriptorResourcesType& os_resources) noexcept
    {
        Impl()->close_receiver(file_descriptor, identifier, os_resources);
    }

    static void stop_receive(const file_descriptor_type file_descriptor,
                             const FileDescriptorResourcesType& os_resources) noexcept
    {
        Impl()->stop_receive(file_descriptor, os_resources);
    }

    template <typename ShortMessageCallback, typename MediumMessageCallback>
    static amp::expected<bool, bmw::os::Error> receive_next(const file_descriptor_type file_descriptor,
                                                            const std::size_t thread,
                                                            ShortMessageCallback fShort,
                                                            MediumMessageCallback fMedium,
                                                            const FileDescriptorResourcesType& os_resources) noexcept
    {
        return Impl()->receive_next(file_descriptor, thread, fShort, fMedium, os_resources);
    }

  private:
    static void CheckImpl()
    {
        ASSERT_NE(impl_, nullptr)
            << "unset implementation, please set `ForwardingReceiverChannelTraits::impl_` before calling methods";
    }

    static IForwardingReceiverChannelTraits* Impl()
    {
        CheckImpl();
        return impl_;
    }
};

const std::size_t ForwardingReceiverChannelTraits::kConcurrency = 2;
IForwardingReceiverChannelTraits* ForwardingReceiverChannelTraits::impl_ = nullptr;

class ReceiverChannelTraitsMock : public IForwardingReceiverChannelTraits
{
  public:
    ReceiverChannelTraitsMock() = default;
    ~ReceiverChannelTraitsMock() override = default;

    MOCK_METHOD((amp::expected<file_descriptor_type, bmw::os::Error>),
                open_receiver,
                (const amp::string_view,
                 const amp::pmr::vector<uid_t>&,
                 const std::int32_t,
                 const FileDescriptorResourcesType&),
                (noexcept, override));

    MOCK_METHOD(void,
                close_receiver,
                (const file_descriptor_type, const amp::string_view, const FileDescriptorResourcesType&),
                (noexcept, override));

    MOCK_METHOD(void,
                stop_receive,
                (const file_descriptor_type, const FileDescriptorResourcesType&),
                (noexcept, override));

    MOCK_METHOD((amp::expected<bool, bmw::os::Error>),
                receive_next,
                (const file_descriptor_type,
                 const std::size_t thread,
                 std::function<void(ShortMessage)> fShort,
                 std::function<void(MediumMessage)> fMedium,
                 const FileDescriptorResourcesType&),
                (noexcept, override));
};

class ReceiverFactoryMock final
{
  public:
    static amp::pmr::unique_ptr<IReceiver> Create(
        const amp::string_view identifier,
        concurrency::Executor& executor,
        const amp::span<const uid_t> allowed_user_ids,
        const ReceiverConfig& receiver_config = {},
        amp::pmr::memory_resource* memory_resource = amp::pmr::get_default_resource())
    {
        return amp::pmr::make_unique<Receiver<ForwardingReceiverChannelTraits>>(
            memory_resource, identifier, executor, allowed_user_ids, receiver_config);
    }
};

}  // namespace message_passing
}  // namespace com
}  // namespace mw
}  // namespace bmw
#endif  // PLATFORM_AAS_MW_COM_MESSAGE_PASSING_RECEIVER_TRAITS_MOCK_H
