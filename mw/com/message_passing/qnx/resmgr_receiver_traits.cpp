// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#include "platform/aas/mw/com/message_passing/qnx/resmgr_receiver_traits.h"

#include "platform/aas/lib/os/unistd.h"
#include "platform/aas/mw/com/message_passing/qnx/resmgr_traits_common.h"

#include <amp_assert.hpp>
#include <algorithm>


/* across the whole file, which turns this warning into noise */

namespace bmw
{
namespace mw
{
namespace com
{
namespace message_passing
{

namespace
{

constexpr void* kNoReply{nullptr};
constexpr std::size_t kNoSize{0};

}  // namespace

amp::expected<dispatch_t*, bmw::os::Error> ResmgrReceiverTraits::CreateAndAttachChannel(
    const amp::string_view identifier,
    ResmgrSetup& setup,
    const FileDescriptorResourcesType& os_resources) noexcept
{
    
    AMP_ASSERT_MESSAGE(IsOsResourcesValid(os_resources), "OS resources are not valid!");
    
    const auto dpp_expected =
        // This function in a banned list, however according to the requirement
        //  it's allowed for IPC API that is mw::com (aka LoLa)
        // NOLINTNEXTLINE(bmw-banned-function): See above
        os_resources.dispatch->dispatch_create_channel(-1, static_cast<std::uint32_t>(DISPATCH_FLAG_NOLOCK));
    if (!dpp_expected.has_value())
    {
        return amp::make_unexpected(dpp_expected.error());
    }
    dispatch_t* const dispatch_pointer = dpp_expected.value();

    const QnxResourcePath path{identifier};

    const auto id_expected = os_resources.dispatch->resmgr_attach(dispatch_pointer,
                                                                  &setup.resmgr_attr_,
                                                                  path.c_str(),
                                                                  _FTYPE_ANY,
                                                                  static_cast<std::uint32_t>(_RESMGR_FLAG_SELF),
                                                                  &setup.connect_funcs_,
                                                                  &setup.io_funcs_,
                                                                  &setup.extended_attr_);
    if (!id_expected.has_value())
    {
        return amp::make_unexpected(id_expected.error());
    }

    return dpp_expected;
}

amp::expected<std::int32_t, bmw::os::Error> ResmgrReceiverTraits::CreateTerminationMessageSideChannel(
    dispatch_t* const dispatch_pointer,
    const FileDescriptorResourcesType& os_resources) noexcept
{
    
    AMP_ASSERT_MESSAGE(IsOsResourcesValid(os_resources), "OS resources are not valid!");
    
    // attach a private message handler to process service termination messages
    constexpr _message_attr* noAttr{nullptr};
    constexpr void* noHandle{nullptr};
    const auto attach_expected =
        os_resources.dispatch->message_attach(dispatch_pointer,
                                              noAttr,
                                              static_cast<std::int32_t>(kPrivateMessageTypeFirst),
                                              static_cast<std::int32_t>(kPrivateMessageTypeLast),
                                              &private_message_handler,
                                              noHandle);
    if (!attach_expected.has_value())
    {
        return amp::make_unexpected(attach_expected.error());
    }

    // create a client connection to this channel
    const auto coid_expected = os_resources.dispatch->message_connect(dispatch_pointer, MSG_FLAG_SIDE_CHANNEL);
    return coid_expected;
}

amp::expected<ResmgrReceiverTraits::file_descriptor_type, bmw::os::Error> ResmgrReceiverTraits::open_receiver(
    const amp::string_view identifier,
    const amp::pmr::vector<uid_t>& allowed_uids,
    const std::int32_t max_number_message_in_queue,
    const FileDescriptorResourcesType& os_resources) noexcept
{
    
    AMP_ASSERT_MESSAGE(IsOsResourcesValid(os_resources), "OS resources are not valid!");
    

    ResmgrSetup& setup = ResmgrSetup::instance(os_resources);

    const auto dpp_expected = CreateAndAttachChannel(identifier, setup, os_resources);
    if (!dpp_expected.has_value())
    {
        return amp::make_unexpected(dpp_expected.error());
    }
    dispatch_t* const dispatch_pointer = dpp_expected.value();

    const auto coid_expected = CreateTerminationMessageSideChannel(dispatch_pointer, os_resources);
    if (!coid_expected.has_value())
    {
        return amp::make_unexpected(coid_expected.error());
    }
    const std::int32_t side_channel_coid = coid_expected.value();
    using Alloc = amp::pmr::polymorphic_allocator<ResmgrReceiverState>;
    
    Alloc allocator{allowed_uids.get_allocator()};
    
    std::allocator_traits<Alloc>::pointer state_pointer = std::allocator_traits<Alloc>::allocate(allocator, 1);
    std::allocator_traits<Alloc>::construct(
        allocator, state_pointer, max_number_message_in_queue, side_channel_coid, allowed_uids, os_resources);

    for (auto*& state_context_pointer : state_pointer->context_pointers_)
    {
        // This function in a banned list, however according to the requirement
        //  it's allowed for IPC API that is mw::com (aka LoLa)
        // NOLINTNEXTLINE(bmw-banned-function): See above
        const auto ctp_expected = os_resources.dispatch->dispatch_context_alloc(dispatch_pointer);
        if (!ctp_expected.has_value())
        {
            return amp::make_unexpected(ctp_expected.error());
        }
        dispatch_context_t* const context_pointer = ctp_expected.value();
        state_context_pointer = context_pointer;
    }
    return state_pointer;
}


void ResmgrReceiverTraits::close_receiver(const ResmgrReceiverTraits::file_descriptor_type file_descriptor,
                                          const amp::string_view /*identifier*/,
                                          const FileDescriptorResourcesType& os_resources) noexcept

{
    
    AMP_ASSERT_MESSAGE(IsOsResourcesValid(os_resources), "OS resources are not valid!");
    

    if (file_descriptor == ResmgrReceiverTraits::INVALID_FILE_DESCRIPTOR)
    {
        return;
    }

    dispatch_context_t* const first_context_pointer = file_descriptor->context_pointers_[0];
    const std::int32_t side_channel_coid = file_descriptor->side_channel_coid_;

    dispatch_t* const dispatch_pointer = first_context_pointer->resmgr_context.dpp;
    const std::int32_t id = first_context_pointer->resmgr_context.id;

    amp::ignore = os_resources.channel->ConnectDetach(side_channel_coid);
    amp::ignore =
        os_resources.dispatch->resmgr_detach(dispatch_pointer, id, static_cast<std::uint32_t>(_RESMGR_DETACH_CLOSE));
    // This function in a banned list, however according to the requirement
    //  it's allowed for IPC API that is mw::com (aka LoLa)
    // NOLINTNEXTLINE(bmw-banned-function): See above
    amp::ignore = os_resources.dispatch->dispatch_destroy(dispatch_pointer);
    for (auto* context_pointer : file_descriptor->context_pointers_)
    {
        // This function in a banned list, however according to the requirement
        //  it's allowed for IPC API that is mw::com (aka LoLa)
        // NOLINTNEXTLINE(bmw-banned-function): See above
        os_resources.dispatch->dispatch_context_free(context_pointer);
    }
    using Alloc = amp::pmr::polymorphic_allocator<ResmgrReceiverState>;
    Alloc allocator{file_descriptor->allowed_uids_.get_allocator()};
    std::allocator_traits<Alloc>::destroy(allocator, file_descriptor);
    std::allocator_traits<Alloc>::deallocate(allocator, file_descriptor, 1);
}

void ResmgrReceiverTraits::stop_receive(const ResmgrReceiverTraits::file_descriptor_type file_descriptor,
                                        const FileDescriptorResourcesType& os_resources) noexcept
{
    
    AMP_ASSERT_MESSAGE(IsOsResourcesValid(os_resources), "OS resources are not valid!");
    

    const std::int32_t side_channel_coid = file_descriptor->side_channel_coid_;
    Stop(side_channel_coid, os_resources);
}

ResmgrReceiverTraits::ResmgrSetup::ResmgrSetup(const FileDescriptorResourcesType& os_resources) noexcept
{
    
    AMP_ASSERT_MESSAGE(IsOsResourcesValid(os_resources), "OS resources are not valid!");
    
    resmgr_attr_.nparts_max = 1U;
    resmgr_attr_.msg_max_size = 1024U;

    // pre-configure resmgr callback data
    os_resources.iofunc->iofunc_func_init(_RESMGR_CONNECT_NFUNCS, &connect_funcs_, _RESMGR_IO_NFUNCS, &io_funcs_);
    open_default = connect_funcs_.open;
    connect_funcs_.open = &io_open;
    io_funcs_.write = &io_write;

    
    constexpr mode_t attrMode{S_IFNAM | 0666};
    
    constexpr iofunc_attr_t* noAttr{nullptr};
    constexpr _client_info* noClientInfo{nullptr};

    // pre-configure resmgr access rights data
    os_resources.iofunc->iofunc_attr_init(&extended_attr_.attr, attrMode, noAttr, noClientInfo);
}

ResmgrReceiverTraits::ResmgrSetup& ResmgrReceiverTraits::ResmgrSetup::instance(
    const FileDescriptorResourcesType& os_resources) noexcept
{
    static ResmgrSetup setup(os_resources);  // LCOV_EXCL_BR_LINE false positive
    return setup;
}

void ResmgrReceiverTraits::Stop(const std::int32_t side_channel_coid, const FileDescriptorResourcesType& os_resources)
{
    
    AMP_ASSERT_MESSAGE(IsOsResourcesValid(os_resources), "OS resources are not valid!");
    

    // This function in a banned list, however according to the requirement
    //  it's allowed for IPC API that is mw::com (aka LoLa)
    // NOLINTNEXTLINE(bmw-banned-function): See above
    amp::ignore = os_resources.channel->MsgSend(
        side_channel_coid, &kPrivateMessageStop, sizeof(kPrivateMessageStop), kNoReply, kNoSize);
}

std::int32_t ResmgrReceiverTraits::io_open(resmgr_context_t* const ctp,
                                           io_open_t* const msg,
                                           RESMGR_HANDLE_T* const handle,
                                           void* const extra) noexcept
{
    const auto& context_data = GetContextData(ctp);
    const ResmgrReceiverState& receiver_state = context_data.receiver_state;
    
    AMP_ASSERT_MESSAGE(IsOsResourcesValid(receiver_state.os_resources_), "OS resources are not valid!");
    
    const auto& allowed_uids = receiver_state.allowed_uids_;

    if (!allowed_uids.empty())
    {
        auto& channel = receiver_state.os_resources_.channel;
        _client_info cinfo{};
        const auto result = channel->ConnectClientInfo(ctp->info.scoid, &cinfo, 0);
        if (!result.has_value())
        {
            return EINVAL;
        }
        const uid_t their_uid = cinfo.cred.euid;
        if (std::find(allowed_uids.begin(), allowed_uids.end(), their_uid) == allowed_uids.end())
        {
            return EACCES;
        }
    }

    ResmgrSetup& setup = ResmgrSetup::instance(receiver_state.os_resources_);
    return setup.open_default(ctp, msg, handle, extra);
}

std::int32_t ResmgrReceiverTraits::CheckWritePreconditions(resmgr_context_t* const ctp,
                                                           io_write_t* const msg,
                                                           RESMGR_OCB_T* const ocb) noexcept
{
    const auto& context_data = GetContextData(ctp);
    
    AMP_ASSERT_MESSAGE(IsOsResourcesValid(context_data.receiver_state.os_resources_), "OS resources are not valid!");
    
    const auto& iofunc = context_data.receiver_state.os_resources_.iofunc;

    // check if the write operation is allowed
    auto result = iofunc->iofunc_write_verify(ctp, msg, ocb, nullptr);
    if (!result.has_value())
    {
        return result.error();
    }

    // check if we are requested to do just a plain write
    if ((msg->i.xtype & _IO_XTYPE_MASK) != _IO_XTYPE_NONE)
    {
        return ENOSYS;
    }
    // get the number of bytes we were asked to write, check that there are enough bytes in the message
    
    const std::size_t nbytes = _IO_WRITE_GET_NBYTES(msg);  // LCOV_EXCL_BR_LINE library macro with benign conditonal
    
    const std::size_t nbytes_max = static_cast<std::size_t>(ctp->info.srcmsglen) - ctp->offset - sizeof(io_write_t);
    if (nbytes > nbytes_max)
    {
        return EBADMSG;
    }
    return EOK;
}

std::int32_t ResmgrReceiverTraits::GetMessageData(resmgr_context_t* const ctp,
                                                  io_write_t* const msg,
                                                  const std::size_t nbytes,
                                                  MessageData& message_data) noexcept
{
    const auto& context_data = GetContextData(ctp);
    
    AMP_ASSERT_MESSAGE(IsOsResourcesValid(context_data.receiver_state.os_resources_), "OS resources are not valid!");
    
    const auto& dispatch = context_data.receiver_state.os_resources_.dispatch;

    if ((nbytes != sizeof(ShortMessage)) && (nbytes != sizeof(MediumMessage)))
    {
        return EBADMSG;
    }

    // get the message payload
    if (!dispatch->resmgr_msgget(ctp, &message_data.payload_, nbytes, sizeof(msg->i)).has_value())
    {
        return EBADMSG;
    }

    // check that the sender is what it claims to be
    if (nbytes == sizeof(ShortMessage))
    {
        const pid_t their_pid = ctp->info.pid;
        // The unions are strictly prohibited by AUTOSAR Rule A9-5-1. But, in this case what we need to do
        // is serialize two type alternatives in most efficient way possible. The union fits this purpose
        // perfectly as long as we use Tagged union in proper manner.
        // Also the union is implementation detail of outer class, so cannot be potentially misused externally.
        // The Tagged union is implemented in accordence with CppCoreGuidelines to minimize the risk.
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access): Serialization optimization necessity
        if (their_pid != message_data.payload_.short_.pid)
        {
            return EBADMSG;
        }
        message_data.type_ = MessageType::kShortMessage;
    }
    else
    {
        const pid_t their_pid = ctp->info.pid;
        // The unions are strictly prohibited by AUTOSAR Rule A9-5-1. But, in this case what we need to do
        // is serialize two type alternatives in most efficient way possible. The union fits this purpose
        // perfectly as long as we use Tagged union in proper manner.
        // Also the union is implementation detail of outer class, so cannot be potentially misused externally.
        // The Tagged union is implemented in accordence with CppCoreGuidelines to minimize the risk.
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access): Serialization optimization necessity
        if (their_pid != message_data.payload_.medium_.pid)
        {
            return EBADMSG;
        }
        message_data.type_ = MessageType::kMediumMessage;
    }
    return EOK;
}

std::int32_t ResmgrReceiverTraits::io_write(resmgr_context_t* const ctp,
                                            io_write_t* const msg,
                                            RESMGR_OCB_T* const ocb) noexcept
{
    std::int32_t result = CheckWritePreconditions(ctp, msg, ocb);
    if (result != EOK)
    {
        return result;
    }

    
    const std::size_t nbytes = _IO_WRITE_GET_NBYTES(msg);  // LCOV_EXCL_BR_LINE library macro with benign conditonal
    

    MessageData message_data;
    result = GetMessageData(ctp, msg, nbytes, message_data);
    if (result != EOK)
    {
        return result;
    }

    {
        // try to fit the payload to the message queue
        auto& context_data = GetContextData(ctp);
        ResmgrReceiverState& receiver_state = context_data.receiver_state;
        std::lock_guard<std::mutex> guard(receiver_state.message_queue_mutex_);
        if (receiver_state.message_queue_.full())
        {
            // buffer full; reject the message
            return ENOMEM;
        }
        receiver_state.message_queue_.emplace_back(message_data);
    }

    // mark that we have consumed all the bytes
    
    _IO_SET_WRITE_NBYTES(ctp, static_cast<std::int64_t>(nbytes));
    

    return EOK;
}


std::int32_t ResmgrReceiverTraits::private_message_handler(message_context_t* const ctp,
                                                           const std::int32_t /*code*/,
                                                           const std::uint32_t /*flags*/,
                                                           void* const /*handle*/) noexcept

{
    auto& context_data = GetContextData(ctp);
    
    AMP_ASSERT_MESSAGE(IsOsResourcesValid(context_data.receiver_state.os_resources_), "OS resources are not valid!");
    
    const auto& os_resources = context_data.receiver_state.os_resources_;

    // we only accept private requests from ourselves
    const pid_t their_pid = ctp->info.pid;
    const pid_t our_pid = os_resources.unistd->getpid();
    if (their_pid != our_pid)
    {
        // This function in a banned list, however according to the requirement
        //  it's allowed for IPC API that is mw::com (aka LoLa)
        // NOLINTNEXTLINE(bmw-banned-function): See above
        amp::ignore = os_resources.channel->MsgError(ctp->rcvid, EACCES);
        return EOK;
    }

    context_data.to_terminate = true;
    // This function in a banned list, however according to the requirement
    //  it's allowed for IPC API that is mw::com (aka LoLa)
    // NOLINTNEXTLINE(bmw-banned-function): See above
    amp::ignore = os_resources.channel->MsgReply(ctp->rcvid, EOK, kNoReply, kNoSize);
    return EOK;
}

bool ResmgrReceiverTraits::IsOsResourcesValid(const FileDescriptorResourcesType& os_resources) noexcept
{
    return (os_resources.channel != nullptr) && (os_resources.dispatch != nullptr) &&
           (os_resources.unistd != nullptr) && (os_resources.iofunc != nullptr);
}



}  // namespace message_passing
}  // namespace com
}  // namespace mw
}  // namespace bmw
