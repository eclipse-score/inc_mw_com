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


#include "platform/aas/mw/com/message_passing/non_blocking_sender.h"

#include <amp_assert.hpp>
#include <amp_expected.hpp>

#include <exception>
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


/* wrapped_sender and executor aren't modified in the ctor, but stores as non-const members as later non-const methods
 * will get called. */
NonBlockingSender::NonBlockingSender(amp::pmr::unique_ptr<ISender> wrapped_sender,
                                     const std::size_t max_queue_size,
                                     bmw::concurrency::Executor& executor)
    
    : ISender{},
      queue_{max_queue_size, amp::pmr::get_default_resource()},
      queue_mutex_{},
      wrapped_sender_{std::move(wrapped_sender)},
      executor_{executor},
      current_send_task_result_{}
{
    
    AMP_ASSERT_PRD_MESSAGE((wrapped_sender_ != nullptr), "Wrapped sender must not be null.");
    
    if (max_queue_size > QUEUE_SIZE_UPPER_LIMIT)
    {
        
        /* This is an operator overload and no bit manipulation */
        std::cerr << "NonBlockingSender: Given max_queue_size: " << max_queue_size
                  << " exceeds built-in QUEUE_SIZE_UPPER_LIMIT." << std::endl;
        
        
        /* Terminate call tolerated.See Assumptions of Use in mw/com/design/README.md*/
        std::terminate();
        
    }
}

amp::expected_blank<bmw::os::Error> NonBlockingSender::Send(const ShortMessage& message) noexcept
{
    return SendInternal(amp::variant<ShortMessage, MediumMessage>(message));
}

amp::expected_blank<bmw::os::Error> NonBlockingSender::Send(const MediumMessage& message) noexcept
{
    return SendInternal(amp::variant<ShortMessage, MediumMessage>(message));
}

void NonBlockingSender::SendQueueElements(const amp::stop_token token)
{
    while (not token.stop_requested())
    {
        amp::expected_blank<bmw::os::Error> send_result{};
        if (amp::holds_alternative<ShortMessage>(queue_.front()))
        {
            send_result = wrapped_sender_->Send(amp::get<ShortMessage>(queue_.front()));
        }
        else
        {
            send_result = wrapped_sender_->Send(amp::get<MediumMessage>(queue_.front()));
        }

        if (send_result.operator bool() == false)
        {
            std::cerr << "NonBlockingSender: SendQueueElements failed with error: " << send_result.error() << std::endl;
        }

        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            queue_.pop_front();
            if (queue_.empty())
            {
                break;
            }
        }
    }
}

amp::expected_blank<bmw::os::Error> NonBlockingSender::SendInternal(amp::variant<ShortMessage, MediumMessage>&& message)
{
    std::lock_guard<std::mutex> guard(queue_mutex_);
    if ((queue_.full()) || executor_.ShutdownRequested())
    {
        
        return amp::make_unexpected(bmw::os::Error::createFromErrno(EAGAIN));
        
    }
    else
    {
        queue_.emplace_back(std::move(message));
        if (queue_.size() == 1U)
        {
            current_send_task_result_ =
                executor_.Submit([this](const amp::stop_token token) { SendQueueElements(token); });
        }
        return amp::expected_blank<bmw::os::Error>();
    }
}

bool NonBlockingSender::HasNonBlockingGuarantee() const noexcept
{
    return true;
}

NonBlockingSender::~NonBlockingSender()
{
    if (current_send_task_result_.Valid())
    {
        // we aren't interested in the task result
        current_send_task_result_.Abort();

        // to avoid race-conditions, we still wait for the result here.
        amp::ignore = current_send_task_result_.Wait();
    }
}

}  // namespace message_passing
}  // namespace com
}  // namespace mw
}  // namespace bmw
