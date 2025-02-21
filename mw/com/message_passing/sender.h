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


#ifndef PLATFORM_AAS_MW_COM_MESSAGE_PASSING_SENDER_H
#define PLATFORM_AAS_MW_COM_MESSAGE_PASSING_SENDER_H

#include "platform/aas/mw/com/message_passing/i_sender.h"
#include "platform/aas/mw/com/message_passing/message.h"
#include "platform/aas/mw/com/message_passing/sender_config.h"
#include "platform/aas/mw/com/message_passing/shared_properties.h"

#include "platform/aas/lib/concurrency/interruptible_wait.h"
#include "platform/aas/lib/os/errno.h"

#include <amp_expected.hpp>
#include <amp_memory.hpp>
#include <amp_stop_token.hpp>
#include <amp_string_view.hpp>
#include <amp_utility.hpp>

#include <chrono>
#include <cstdint>
#include <iostream>
#include <utility>

namespace bmw
{
namespace mw
{
namespace com
{
namespace message_passing
{

/// \brief Generic implementation of ISender parametrized with ChannelTraits.
/// \tparam ChannelTraits The type that describes how to do the actual sending.
///         Shall behave as if it contains the following members:
///         \code{.cpp}
///             using file_descriptor_type = sometype; /* behaves like a POSIX handle */
///             static constexpr file_descriptor_type INVALID_FILE_DESCRIPTOR;
///             static amp::expected<file_descriptor_type, bmw::os::Error>
///                 try_open(amp::string_view identifier) noexcept;
///             static void close_sender(file_descriptor_type file_descriptor) noexcept;
///             static <sometype_short prepare_payload(ShortMessage const& message) noexcept;
///             static sometype_medium prepare_payload(MediumMessage const& message) noexcept;
///             static amp::expected_blank<bmw::os::Error>
///                 try_send(file_descriptor_type file_descriptor,
///                          sometype_short const& buffer) noexcept;
///             static amp::expected_blank<bmw::os::Error>
///                 try_send(file_descriptor_type file_descriptor,
///                          sometype_medium const& buffer) noexcept;
///         \endcode
///         sometype_short and sometype_medium objects may contain references to the corresponding
///         ShortMessage and MediumMessage objects (or their subobjects) and will have shorter lifetimes.
///         Their purpose is to avoid redundant preparatory steps when the same message needs to be resent.
template <typename ChannelTraits>
class Sender final : public ISender
{
  public:
    using allocator_type = amp::pmr::polymorphic_allocator<Sender<ChannelTraits>>;
    using FDResourcesType = typename ChannelTraits::FileDescriptorResourcesType;

    /// \brief Construct a Sender (not move or copyable). Will wait until respective Receiver is available.
    ///
    /// \param identifier The common identifier between Sender and Receiver (maps to a path in the filesystem)
    /// \param token The amp::stop_token to ensure that waiting for the respective receiver is aborted, once stop is
    ///        requested
    /// \param sender_config additional sender configuration parameters
    /// \param logging_callback provides an output for error messages since we cannot use regular logging
    explicit Sender(const amp::string_view identifier,
                    const amp::stop_token& token,
                    const SenderConfig& sender_config,
                    LoggingCallback logging_callback,
                    const allocator_type& allocator) noexcept;

    ~Sender() noexcept override;
    Sender(Sender&) = delete;
    Sender& operator=(Sender&) = delete;
    Sender(Sender&&) = delete;
    Sender& operator=(Sender&&) = delete;

    amp::expected_blank<bmw::os::Error> Send(const ShortMessage& message) noexcept override;
    amp::expected_blank<bmw::os::Error> Send(const MediumMessage& message) noexcept override;
    bool HasNonBlockingGuarantee() const noexcept override;

  private:
    void OpenOrWaitForChannel(const amp::string_view, const amp::stop_token&) noexcept;
    template <typename Payload>
    amp::expected_blank<bmw::os::Error> SendPrepared(const Payload&) const noexcept;

    amp::stop_token token_;
    typename ChannelTraits::file_descriptor_type file_descriptor_;
    const std::int32_t max_numbers_of_send_retry_;
    const std::chrono::milliseconds send_retry_delay_;
    const std::chrono::milliseconds connect_retry_delay_;
    LoggingCallback logging_callback_;
    bool is_connect_failed_msg_printed_;
    FDResourcesType fd_resources_;
};

template <typename ChannelTraits>
Sender<ChannelTraits>::Sender(const amp::string_view identifier,
                              const amp::stop_token& token,
                              const SenderConfig& sender_config,
                              LoggingCallback logging_callback,
                              const allocator_type& allocator) noexcept
    : ISender{},
      token_{token},
      file_descriptor_{ChannelTraits::INVALID_FILE_DESCRIPTOR},
      max_numbers_of_send_retry_{sender_config.max_numbers_of_retry},
      send_retry_delay_{sender_config.send_retry_delay},
      connect_retry_delay_{sender_config.connect_retry_delay},
      logging_callback_{std::move(logging_callback)},
      is_connect_failed_msg_printed_{false},
      fd_resources_{ChannelTraits::GetDefaultOSResources(allocator.resource())}
{
    while ((file_descriptor_ == ChannelTraits::INVALID_FILE_DESCRIPTOR) && (token.stop_requested() == false))
    {
        OpenOrWaitForChannel(identifier, token);
    }
    amp::ignore = allocator;
}

template <typename ChannelTraits>
void Sender<ChannelTraits>::OpenOrWaitForChannel(const amp::string_view identifier,
                                                 const amp::stop_token& token) noexcept
{
    const auto ret = ChannelTraits::try_open(identifier, fd_resources_);

    if (ret.has_value())
    {
        file_descriptor_ = ret.value();
        if (is_connect_failed_msg_printed_)
        {
            logging_callback_([identifier](std::ostream& out) {
                
                out << "channel finally opened " << identifier.data() << std::endl;
                
            });
        }
    }
    else
    {
        if (!is_connect_failed_msg_printed_)
        {
            logging_callback_([identifier, ret](std::ostream& out) {
                
                out << "Could not open channel " << identifier.data() << " with error: " << ret.error() << std::endl;
                
            });
            is_connect_failed_msg_printed_ = true;
        }
        amp::ignore = bmw::concurrency::wait_for(token, connect_retry_delay_);
    }
}

template <typename ChannelTraits>
amp::expected_blank<bmw::os::Error> Sender<ChannelTraits>::Send(const ShortMessage& message) noexcept
{
    if (file_descriptor_ != ChannelTraits::INVALID_FILE_DESCRIPTOR)
    {
        return SendPrepared(ChannelTraits::prepare_payload(message));
    }
    else
    {
        
        return amp::make_unexpected(bmw::os::Error::createFromErrno(ENFILE));
        
    }
}

template <typename ChannelTraits>
amp::expected_blank<bmw::os::Error> Sender<ChannelTraits>::Send(const MediumMessage& message) noexcept
{
    if (file_descriptor_ != ChannelTraits::INVALID_FILE_DESCRIPTOR)
    {
        return SendPrepared(ChannelTraits::prepare_payload(message));
    }
    else
    {
        
        return amp::make_unexpected(bmw::os::Error::createFromErrno(ENFILE));
        
    }
}

template <typename ChannelTraits>
bool Sender<ChannelTraits>::HasNonBlockingGuarantee() const noexcept
{
    return ChannelTraits::has_non_blocking_guarantee();
}

template <typename ChannelTraits>
template <typename MessagePayload>
amp::expected_blank<bmw::os::Error> Sender<ChannelTraits>::SendPrepared(const MessagePayload& payload) const noexcept
{
    std::int32_t retries{};
    amp::expected_blank<bmw::os::Error> error{};
    do
    {
        // We might be opening the file with O_NONBLOCK, so there is the possibility of an intermediate error
        const auto result = ChannelTraits::try_send(file_descriptor_, payload, fd_resources_);
        if (result.has_value() == false)
        {
            ++retries;
            error = amp::make_unexpected(result.error());
            if (send_retry_delay_ > std::chrono::milliseconds{0})
            {
                // no error code is returned. return value is ignored as it is not used here.
                amp::ignore = bmw::concurrency::wait_for(token_, send_retry_delay_);
            }
        }
        else
        {
            return {};  // all good, no need to retry
        }
    } while ((retries < max_numbers_of_send_retry_) && (token_.stop_requested() == false));
    return error;
}

template <typename ChannelTraits>
Sender<ChannelTraits>::~Sender() noexcept
{
    if (file_descriptor_ != ChannelTraits::INVALID_FILE_DESCRIPTOR)
    {
        ChannelTraits::close_sender(file_descriptor_, fd_resources_);
    }
}

}  // namespace message_passing
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_MESSAGE_PASSING_SENDER_H
