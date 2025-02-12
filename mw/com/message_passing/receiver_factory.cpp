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


#include "platform/aas/mw/com/message_passing/receiver_factory.h"

#include "platform/aas/mw/com/message_passing/receiver_factory_impl.h"

#include <amp_string_view.hpp>

#include <memory>
#include <utility>
#include <vector>

namespace
{

/// \brief Small wrapper class around a gmock of IReceiver.
/// \details gmock instances aren't copyable
class ReceiverMockWrapper final : public bmw::mw::com::message_passing::IReceiver
{
  public:
    
    explicit ReceiverMockWrapper(bmw::mw::com::message_passing::IReceiver* const mock)
        : IReceiver{}, wrapped_mock_{mock}
    {
    }
    

    
    
    
    void Register(const bmw::mw::com::message_passing::MessageId id, ShortMessageReceivedCallback callback) override
    
    
    {
        return wrapped_mock_->Register(id, std::move(callback));
    }

    
    
    
    void Register(const bmw::mw::com::message_passing::MessageId id, MediumMessageReceivedCallback callback) override
    
    
    {
        return wrapped_mock_->Register(id, std::move(callback));
    }

    
    
    amp::expected_blank<bmw::os::Error> StartListening() override
    
    

  private:
    bmw::mw::com::message_passing::IReceiver* wrapped_mock_;
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

IReceiver* ReceiverFactory::receiver_mock_{nullptr};


amp::pmr::unique_ptr<IReceiver> ReceiverFactory::Create(const amp::string_view identifier,
                                                        concurrency::Executor& executor,
                                                        const amp::span<const uid_t> allowed_user_ids,
                                                        const ReceiverConfig& receiver_config,
                                                        amp::pmr::memory_resource* memory_resource)

{
    if (receiver_mock_ == nullptr)
    {
        return ReceiverFactoryImpl::Create(identifier, executor, allowed_user_ids, receiver_config, memory_resource);
    }
    else
    {
        return amp::pmr::make_unique<ReceiverMockWrapper>(memory_resource, receiver_mock_);
    }
}


/* (1) False positive: Function uses non-static class members. (2) False positive: Function is declared in header file.
 */
void ReceiverFactory::InjectReceiverMock(IReceiver* const mock)
{
    receiver_mock_ = mock;
}


}  // namespace message_passing
}  // namespace com
}  // namespace mw
}  // namespace bmw
