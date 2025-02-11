// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#include "platform/aas/mw/com/message_passing/sender_factory.h"

#include "platform/aas/mw/com/message_passing/sender_factory_impl.h"

#include <memory>
#include <utility>

namespace
{

/// \brief Small wrapper class around a gmock of ISender.
/// \details gmock instances aren't copyable
class SenderMockWrapper : public bmw::mw::com::message_passing::ISender
{
  public:
    
    explicit SenderMockWrapper(bmw::mw::com::message_passing::ISender* const mock) : ISender{}, wrapped_mock_{mock} {}
    

    amp::expected_blank<bmw::os::Error> Send(
        const bmw::mw::com::message_passing::ShortMessage& message) noexcept override
    {
        return wrapped_mock_->Send(message);
    }

    amp::expected_blank<bmw::os::Error> Send(
        const bmw::mw::com::message_passing::MediumMessage& message) noexcept override
    {
        return wrapped_mock_->Send(message);
    }

    bool HasNonBlockingGuarantee() const noexcept override { return wrapped_mock_->HasNonBlockingGuarantee(); }

  private:
    bmw::mw::com::message_passing::ISender* wrapped_mock_;
};

}  // namespace

namespace bmw
{
namespace mw
{
namespace com
{
namespace message_passing
{

ISender* SenderFactory::sender_mock_ = nullptr;

amp::callback<void(const amp::stop_token&)> callback_;


amp::pmr::unique_ptr<ISender> SenderFactory::Create(const amp::string_view identifier,
                                                    const amp::stop_token& token,
                                                    const SenderConfig& sender_config,
                                                    LoggingCallback logging_callback,
                                                    amp::pmr::memory_resource* const memory_resource)
{
    if (sender_mock_ == nullptr)
    {
        return SenderFactoryImpl::Create(
            identifier, token, sender_config, std::move(logging_callback), memory_resource);
    }
    else
    {
        callback_(token);
        return amp::pmr::make_unique<SenderMockWrapper>(memory_resource, sender_mock_);
    }
}

void SenderFactory::InjectSenderMock(ISender* const mock, amp::callback<void(const amp::stop_token&)>&& callback)
{
    sender_mock_ = mock;
    callback_ = std::move(callback);
}

}  // namespace message_passing
}  // namespace com
}  // namespace mw
}  // namespace bmw
