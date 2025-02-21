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


#ifndef PLATFORM_AAS_MW_COM_MESSAGE_PASSING_NONBLOCKINGSENDER_H
#define PLATFORM_AAS_MW_COM_MESSAGE_PASSING_NONBLOCKINGSENDER_H

#include "platform/aas/lib/concurrency/executor.h"
#include "platform/aas/lib/concurrency/task_result.h"
#include "platform/aas/lib/memory/pmr_ring_buffer.h"
#include "platform/aas/mw/com/message_passing/i_sender.h"

#include <amp_expected.hpp>
#include <amp_memory.hpp>
#include <amp_variant.hpp>

#include <mutex>
#include <vector>

namespace bmw
{
namespace mw
{
namespace com
{
namespace message_passing
{

/// \brief This class provides a wrapper around any ISender implementation, which assures a non/never blocking behaviour
///        on Send() calls.
/// \attention It makes no sense to use it wrapping an ISender implementation, which already assures non-blocking!
///
/// \details Because of safety (ASIL-B) requirements, it is not acceptable, that a ASIL-B sender gets eventually
///          blocked by a ASIL-QM receiver (at least we want to prevent it, even if the ASIL-B app does its own runtime
///          supervision/watchdog mechanism).
///          The underlying OS specific implementations of ISender/IReceiver vary on their behaviour! Even if they all
///          need to be async to fulfil ISender contract, there is still a major difference between "async" and a
///          "non-blocking-guarantee"!
///          E.g. in QNX we currently use a ISender/IReceiver implementation based on QNX IPC-messaging. Since in QNX
///          (microkernel) there are no kernel buffers, which decouple ISender/IReceiver, a Send() call in our impl.
///          leads to a transition from sender proc to receiver proc, where our receiver impl. takes the message, queues
///          it in a locally managed queue for deferred processing and directly unblocks the sender again.
///          So in normal operation this is the most efficient solution in QNX and fully async by nature. But in case
///          some untrusted QM code within the receiver process compromises our reception thread (hinders its queueing/
///          quick ack to the sender), we could run into a "blocking" behaviour!
///
class NonBlockingSender final : public ISender
{
  public:
    /// \brief ctor for NonBlockingSender
    /// \param wrapped_sender a potentially blocking sender to be wrapped
    /// \param max_queue_size queue size to be used.
    /// \param executor execution policy to be used to call wrapped sender Send() calls from queue. As only one task at
    ///                 a time will be submitted anyhow, maxConcurrencyLevel of the executor needs only to be 1!
    NonBlockingSender(amp::pmr::unique_ptr<ISender> wrapped_sender,
                      const std::size_t max_queue_size,
                      concurrency::Executor& executor);

    ~NonBlockingSender() override;

    // Since queue_ is based on bmw::memory::PmrRingBuffer, cannot be moved or copied
    NonBlockingSender(const NonBlockingSender&) = delete;
    NonBlockingSender(NonBlockingSender&&) noexcept = delete;
    NonBlockingSender& operator=(const NonBlockingSender&) = delete;
    NonBlockingSender& operator=(NonBlockingSender&&) noexcept = delete;

    /// \brief Sends a ShortMessage with non-blocking guarantee.
    /// \param message message to be sent.
    /// \return See SendInternal()
    amp::expected_blank<bmw::os::Error> Send(const message_passing::ShortMessage& message) noexcept override;

    /// \brief Sends a MediumMessage with non-blocking guarantee.
    /// \param message message to be sent.
    /// \return See SendInternal()
    amp::expected_blank<bmw::os::Error> Send(const message_passing::MediumMessage& message) noexcept override;

    /// \brief Returns true as non-blocking guarantee is the job of tis wrapper.
    /// \return true
    
    bool HasNonBlockingGuarantee() const noexcept override;
    

  private:
    
    static constexpr std::size_t QUEUE_SIZE_UPPER_LIMIT = 100U;
    

    /// \brief Function called by callable posted to executor. Takes message from queue front and calls Send() on the
    ///        wrapped sender and removes queue entry afterwards.
    /// \pre There is at least one element/message in the queue.
    /// \details If stop has been already requested, no Send() call is done. After Send() (independent of outcome) the
    ///          queue element is removed and if there is still a queue element left a new callable calling this very
    ///          same function is posted to the executor.
    /// \param token stop_token provided by executor.
    void SendQueueElements(const amp::stop_token token);

    /// \brief internal Send function taking either a short or medium message to be sent.
    /// \param message
    /// \return only returns an error (bmw::os::Error::Code::kResourceTemporarilyUnavailable) if queue is full or for
    ///         the underlying executor already shutdown was requested!
    ///         Any Send-errors encountered async, when sending internally from the queue will not be returned back.
    amp::expected_blank<bmw::os::Error> SendInternal(amp::variant<ShortMessage, MediumMessage>&& message);

    bmw::memory::PmrRingBuffer<amp::variant<ShortMessage, MediumMessage>> queue_;
    std::mutex queue_mutex_;
    amp::pmr::unique_ptr<ISender> wrapped_sender_;
    concurrency::Executor& executor_;
    /// \brief we store the task result of latest submit call to executor to be able to abort it in case of our
    ///        destruction, to avoid race conditions!
    concurrency::TaskResult<void> current_send_task_result_;
};

}  // namespace message_passing
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_MESSAGE_PASSING_NONBLOCKINGSENDER_H
