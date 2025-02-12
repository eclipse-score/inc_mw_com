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


#ifndef PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_MESSAGEPASSINGCONTROL_H
#define PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_MESSAGEPASSINGCONTROL_H

#include "platform/aas/mw/com/impl/bindings/lola/messaging/i_message_passing_control.h"

#include "platform/aas/lib/concurrency/thread_pool.h"

#include <amp_optional.hpp>
#include <amp_stop_token.hpp>

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>

namespace bmw
{
namespace mw
{
namespace com
{
namespace impl
{
namespace lola
{

/// \brief MessagePassingControl is a facade that handles message-based communication between LoLa proxy/skeleton
/// instances of different processes.
///
/// \details This message-based communication is a side-channel to the shared-memory based interaction between LoLa
/// proxy/skeleton instances. It is used for exchange of control information/notifications, where the shared-memory
/// channel is used rather for data exchange.
/// MessagePassingFacade relies on message_passing::Receiver/Sender for its communication needs.
/// If it detects, that communication partners are located within the same process, it opts for direct function/method
/// call optimization, instead of using message_passing.
///
class MessagePassingControl final : public IMessagePassingControl
{
  public:
    /// \brief ctor for MessagePassingControl
    /// \param asil_b_capability if set to true, this instance of MessagePassingControl will support message sending
    ///        for QM and ASIL-B.
    /// \param sender_queue_size size of te non-blocking sender queue in case of asil_b_capability == true
    explicit MessagePassingControl(const bool asil_b_capability, const std::int32_t sender_queue_size) noexcept;

    ~MessagePassingControl() noexcept override = default;

    MessagePassingControl(MessagePassingControl&&) = delete;
    MessagePassingControl& operator=(MessagePassingControl&&) = delete;
    MessagePassingControl(const MessagePassingControl&) = delete;
    MessagePassingControl& operator=(const MessagePassingControl&) = delete;

    /// \see IMessagePassingControl::GetMessagePassingSender
    std::shared_ptr<bmw::mw::com::message_passing::ISender> GetMessagePassingSender(
        const QualityType asil_level,
        const pid_t target_node_id) override;
    /// \see IMessagePassingControl::RemoveMessagePassingSender
    void RemoveMessagePassingSender(const QualityType asil_level, pid_t target_node_id) override;

    /// \brief see IMessagePassingControl::GetNodeIdentifier
    pid_t GetNodeIdentifier() const noexcept override;

    /// \brief see IMessagePassingControl::CreateMessagePassingName
    std::string CreateMessagePassingName(const QualityType asil_level, const pid_t node_id) override;

  private:
    concurrency::ThreadPool& GetNonBlockingSenderThreadPool();

    std::shared_ptr<message_passing::ISender> CreateNewSender(const QualityType& asil_level,
                                                              const pid_t target_node_id);
    
    /// \brief does our instance support ASIL-B?
    bool asil_b_capability_;
    /// \brief sender queue size for non blocking sender (just used in case of asil_b_capability_ == true)
    std::int32_t sender_queue_size_;
    /// \brief our own node identifier (pid)
    pid_t node_identifier_;
    /// \brief map for ASIL-QM message senders to other processes. Key is node_id (e.g. pid) of target process.
    std::unordered_map<pid_t, std::shared_ptr<bmw::mw::com::message_passing::ISender>> senders_qm_;
    std::mutex senders_qm_mutex_;
    /// \brief map for ASIL-B message senders to other processes. Key is node_id (e.g. pid) of target process.
    std::unordered_map<pid_t, std::shared_ptr<bmw::mw::com::message_passing::ISender>> senders_asil_;
    std::mutex senders_asil_mutex_;
    /// \brief Stop source to control owned child senders, which may block in construction.
    amp::stop_source stop_source_;
    /// \brief optional thread-pool for non blocking senders. (only needed if we are ASIL-B and have to send to ASIL-QM)
    amp::optional<bmw::concurrency::ThreadPool> non_blocking_sender_thread_pool_;
    
};

}  // namespace lola
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_MESSAGEPASSINGCONTROL_H
