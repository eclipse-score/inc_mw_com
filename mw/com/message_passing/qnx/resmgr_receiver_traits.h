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


#ifndef PLATFORM_AAS_MW_COM_MESSAGE_PASSING_RESMGR_RECEIVER_TRAITS_H
#define PLATFORM_AAS_MW_COM_MESSAGE_PASSING_RESMGR_RECEIVER_TRAITS_H

#include "platform/aas/mw/com/message_passing/message.h"

#include "platform/aas/lib/memory/pmr_ring_buffer.h"

#include "platform/aas/lib/os/qnx/channel.h"
#include "platform/aas/lib/os/qnx/dispatch.h"
#include "platform/aas/lib/os/qnx/iofunc.h"
#include "platform/aas/lib/os/unistd.h"

#include <amp_expected.hpp>
#include <amp_string_view.hpp>
#include <amp_utility.hpp>
#include <amp_vector.hpp>

#include <cstdint>
#include <type_traits>

namespace bmw
{
namespace mw
{
namespace com
{
namespace message_passing
{

class ResmgrReceiverTraits
{
  private:
    struct ResmgrReceiverState;

  public:
    static constexpr std::size_t kConcurrency{2};
    using file_descriptor_type = ResmgrReceiverState*;
    static constexpr file_descriptor_type INVALID_FILE_DESCRIPTOR{nullptr};

    struct OsResources
    {
        amp::pmr::unique_ptr<bmw::os::Dispatch> dispatch{};
        amp::pmr::unique_ptr<bmw::os::Channel> channel{};
        amp::pmr::unique_ptr<bmw::os::IoFunc> iofunc{};
        amp::pmr::unique_ptr<bmw::os::Unistd> unistd{};
    };

    static OsResources GetDefaultOSResources(amp::pmr::memory_resource* memory_resource) noexcept
    {
        return {bmw::os::Dispatch::Default(memory_resource),
                bmw::os::Channel::Default(memory_resource),
                bmw::os::IoFunc::Default(memory_resource),
                bmw::os::Unistd::Default(memory_resource)};
    }

    using FileDescriptorResourcesType = OsResources;

    static amp::expected<file_descriptor_type, bmw::os::Error> open_receiver(
        const amp::string_view identifier,
        const amp::pmr::vector<uid_t>& allowed_uids,
        const std::int32_t max_number_message_in_queue,
        const FileDescriptorResourcesType& os_resources) noexcept;

    static void close_receiver(const file_descriptor_type file_descriptor,
                               const amp::string_view /*identifier*/,
                               const FileDescriptorResourcesType& os_resources) noexcept;

    static void stop_receive(const file_descriptor_type file_descriptor,
                             const FileDescriptorResourcesType& os_resources) noexcept;

    template <typename ShortMessageProcessor, typename MediumMessageProcessor>
    static amp::expected<bool, bmw::os::Error> receive_next(const file_descriptor_type file_descriptor,
                                                            const std::size_t thread,
                                                            ShortMessageProcessor fShort,
                                                            MediumMessageProcessor fMedium,
                                                            const FileDescriptorResourcesType&) noexcept
    {
        ResmgrReceiverState& receiver_state = *file_descriptor;
        dispatch_context_t* const context_pointer = receiver_state.context_pointers_.at(thread);

        // pre-initialize our context_data
        ResmgrContextData context_data{false, receiver_state};
        context_pointer->resmgr_context.extra->data = &context_data;

        auto& dispatch = context_data.receiver_state.os_resources_.dispatch;

        // tell the framework to wait for the message
        // This function in a banned list, however according to the requirement
        //  it's allowed for IPC API that is mw::com (aka LoLa)
        // NOLINTNEXTLINE(bmw-banned-function): See above
        const auto block_result = dispatch->dispatch_block(context_pointer);
        if (!block_result.has_value())
        {
            // shall not be a critical error; skip the dispatch_handler() but allow the next iteration
            return true;
        }

        // tell the framework to process the incoming message (and maybe to call one of our callbacks)
        // This function in a banned list, however according to the requirement
        //  it's allowed for IPC API that is mw::com (aka LoLa)
        // NOLINTNEXTLINE(bmw-banned-function): See above
        const auto handler_result = dispatch->dispatch_handler(context_pointer);

        if (!handler_result.has_value())
        {
            // shall not be a critical error, but there were no valid message to handle
            return true;
        }

        if (context_data.to_terminate)
        {
            // we were asked to stop; do it in this thread
            return false;
        }

        {
            std::lock_guard<std::mutex> guard(receiver_state.message_queue_mutex_);
            if (receiver_state.message_queue_.empty())
            {
                // nothing to process yet
                return true;
            }
            if (receiver_state.message_queue_owned_)
            {
                // will be processed in another thread
                return true;
            }
            // Only one thread,  () explicitly fulfilled for Resmgr implementation
            receiver_state.message_queue_owned_ = true;
        }

        while (true)
        {
            MessageData message_data{};
            {
                std::lock_guard<std::mutex> guard(receiver_state.message_queue_mutex_);
                if (receiver_state.message_queue_.empty())
                {
                    // nothing to process already
                    receiver_state.message_queue_owned_ = false;
                    return true;
                }
                message_data = receiver_state.message_queue_.front();
                receiver_state.message_queue_.pop_front();
            }

            if (message_data.type_ == MessageType::kShortMessage)
            {
                // The unions are strictly prohibited by AUTOSAR Rule A9-5-1. But, in this case what we need to do
                // is serialize two type alternatives in most efficient way possible. The union fits this purpose
                // perfectly as long as we use Tagged union in proper manner.
                // Also the union is implementation detail of outer class, so cannot be potentially misused externally.
                // The Tagged union is implemented in accordence with CppCoreGuidelines to minimize the risk.
                // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access): Serialization optimization necessity
                fShort(message_data.payload_.short_);
            }
            else
            {
                // The unions are strictly prohibited by AUTOSAR Rule A9-5-1. But, in this case what we need to do
                // is serialize two type alternatives in most efficient way possible. The union fits this purpose
                // perfectly as long as we use Tagged union in proper manner.
                // Also the union is implementation detail of outer class, so cannot be potentially misused externally.
                // The Tagged union is implemented in accordence with CppCoreGuidelines to minimize the risk.
                // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access): Serialization optimization necessity
                fMedium(message_data.payload_.medium_);
            }
        }
    }

  private:
    static constexpr std::uint16_t kPrivateMessageTypeFirst{_IO_MAX + 1};
    static constexpr std::uint16_t kPrivateMessageTypeLast{kPrivateMessageTypeFirst};
    static constexpr std::uint16_t kPrivateMessageStop{kPrivateMessageTypeFirst};

    enum class MessageType : std::size_t
    {
        kNone,
        kShortMessage,
        kMediumMessage
    };

    union MessagePayload
    {
        ShortMessage short_;
        MediumMessage medium_;
    };

    static_assert(std::is_trivially_copyable<MessagePayload>::value, "MessagePayload union must be TriviallyCopyable");

    // The unions are strictly prohibited by AUTOSAR Rule A9-5-1. But, in this case what we need to do
    // is serialize two type alternatives in most efficient way possible. The union fits this purpose
    // perfectly as long as we use Tagged union in proper manner.
    // Also the union is implementation detail of outer class, so cannot be potentially misused externally.
    // The Tagged union is implemented in accordence with CppCoreGuidelines to minimize the risk.
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access): Serialization optimization necessity
    struct MessageData
    {
        MessageType type_{MessageType::kNone};
        MessagePayload payload_{};
    };

    static_assert(std::is_trivially_copyable<MessageData>::value, "MessageData must be TriviallyCopyable");

    // common attributes for all services of the process
    
    /* Semantically an initialized struct, private nested class */
    // AUTOSAR Rule A11-0-1 prohibits non-POD structs as well as Rule A11-0-2 prohibits special member
    // functions (such as constructor) for structs. But, in this case struct is implementation detail
    // of outer class and this violation have no side effect. Instead, a semantic struct is much better
    // choice to implement Setup info as data object, as long as there are only member access operations.
    // WARNING: ResmgrSetup is struct but it is neither PODType nor TrivialType nor TriviallyCopyable!
    // Note that it is still StandardLayoutType.
    // NOLINTNEXTLINE(bmw-struct-usage-compliance): Constructor implements data initialization logic
    struct ResmgrSetup
    {
        resmgr_attr_t resmgr_attr_{};
        resmgr_connect_funcs_t connect_funcs_{};
        resmgr_io_funcs_t io_funcs_{};
        extended_dev_attr_t extended_attr_{};
        std::int32_t (*open_default)(resmgr_context_t* ctp, io_open_t* msg, RESMGR_HANDLE_T* handle, void* extra){
            nullptr};

        explicit ResmgrSetup(const FileDescriptorResourcesType& os_resources) noexcept;

        static ResmgrSetup& instance(const FileDescriptorResourcesType& os_resources) noexcept;
    };
    

    static_assert(std::is_standard_layout<ResmgrSetup>::value, "ResmgrSetup type must be StandardLayoutType");

    
    /* Semantically an initialized struct, private nested class */
    // AUTOSAR Rule A11-0-1 prohibits non-POD structs as well as Rule A11-0-2 prohibits special member
    // functions (such as constructor) for structs. But, in this case struct is implementation detail
    // of outer class and this violation have no side effect. Instead, a semantic struct is much better
    // choice to implement State as data object, as long as there are only member access operations.
    // WARNING: ResmgrReceiverState is struct but it is neither PODType nor StandardLayoutType
    // nor TrivialType nor TriviallyCopyable!
    // NOLINTNEXTLINE(bmw-struct-usage-compliance): Constructor is used for struct members initialization
    struct ResmgrReceiverState
    {
        std::array<dispatch_context_t*, kConcurrency> context_pointers_;
        const std::int32_t side_channel_coid_;

        std::mutex message_queue_mutex_;
        bmw::memory::PmrRingBuffer<MessageData> message_queue_;
        bool message_queue_owned_;
        const amp::pmr::vector<uid_t>& allowed_uids_;
        const FileDescriptorResourcesType& os_resources_;

        ResmgrReceiverState(const std::size_t max_message_queue_size,
                            const std::int32_t side_channel_coid,
                            const amp::pmr::vector<uid_t>& allowed_uids,
                            const FileDescriptorResourcesType& os_resources)
            : context_pointers_(),
              side_channel_coid_(side_channel_coid),
              message_queue_(max_message_queue_size, allowed_uids.get_allocator()),
              message_queue_owned_(false),
              allowed_uids_(allowed_uids),
              os_resources_(os_resources)
        {
        }
    };
    

    
    /* Semantically an initialized struct, private nested class */
    struct ResmgrContextData
    {
        bool to_terminate;
        ResmgrReceiverState& receiver_state;
    };
    

    static ResmgrContextData& GetContextData(const resmgr_context_t* const ctp)
    {
        return *static_cast<ResmgrContextData*>(ctp->extra->data);
    }

    static amp::expected<dispatch_t*, bmw::os::Error> CreateAndAttachChannel(
        const amp::string_view identifier,
        ResmgrSetup& setup,
        const FileDescriptorResourcesType& os_resources) noexcept;

    static amp::expected<std::int32_t, bmw::os::Error> CreateTerminationMessageSideChannel(
        dispatch_t* const dispatch_pointer,
        const FileDescriptorResourcesType& os_resources) noexcept;

    static void Stop(const std::int32_t side_channel_coid, const FileDescriptorResourcesType& os_resources);

    static bool IsOsResourcesValid(const FileDescriptorResourcesType& os_resources) noexcept;

    
    static std::int32_t io_open(resmgr_context_t* const ctp,
                                io_open_t* const msg,
                                RESMGR_HANDLE_T* const handle,
                                void* const extra) noexcept;

    static std::int32_t CheckWritePreconditions(resmgr_context_t* const ctp,
                                                io_write_t* const msg,
                                                RESMGR_OCB_T* const ocb) noexcept;

    static std::int32_t GetMessageData(resmgr_context_t* const ctp,
                                       io_write_t* const msg,
                                       const std::size_t nbytes,
                                       MessageData& message_data) noexcept;

    static std::int32_t io_write(resmgr_context_t* const ctp, io_write_t* const msg, RESMGR_OCB_T* const ocb) noexcept;
    

    static std::int32_t private_message_handler(message_context_t* const ctp,
                                                const std::int32_t /*code*/,
                                                const std::uint32_t /*flags*/,
                                                void* const /*handle*/) noexcept;
};

}  // namespace message_passing
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_MESSAGE_PASSING_RESMGR_RECEIVER_TRAITS_H
