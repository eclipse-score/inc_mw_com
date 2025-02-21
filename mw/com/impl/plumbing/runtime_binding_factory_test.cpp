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


#include "platform/aas/mw/com/impl/plumbing/runtime_binding_factory.h"
#include "platform/aas/mw/com/impl/configuration/config_parser.h"

#include "platform/aas/mw/com/message_passing/receiver_factory.h"
#include "platform/aas/mw/com/message_passing/receiver_mock.h"

#include "platform/aas/lib/concurrency/long_running_threads_container.h"

#include <gtest/gtest.h>

#include <utility>

namespace bmw
{
namespace mw
{
namespace com
{
namespace impl
{

using ::testing::_;
using ::testing::An;
using ::testing::AnyNumber;
using ::testing::Invoke;
using ::testing::Matcher;
using ::testing::Return;

TEST(RuntimeBindingFactoryTest, CanCreateLolaBinding)
{
    auto config_with_lola_binding = R"(
    {
        "serviceTypes": [
            {
            "serviceTypeName": "/bmw/ncar/services/TirePressureService",
            "version": {
                "major": 12,
                "minor": 34
            },
            "bindings": [
                {
                "serviceId": 1234,
                "binding": "SHM",
                "events": [],
                "fields": []
                }
            ]
            }
        ],
        "serviceInstances": [
            {
                "instanceSpecifier": "abc/abc/TirePressurePort",
                "serviceTypeName": "/bmw/ncar/services/TirePressureService",
                "version": {
                    "major": 12,
                    "minor": 34
                },
                "instances": [
                    {
                    "instanceId": 1,
                    "asil-level": "B",
                    "binding": "SHM",
                    "events": [],
                    "fields": []
                    }
                ]
            }
        ],
        "global": {
        "asil-level": "B"
        }
    }
    )";
    const bmw::json::JsonParser json_parser_obj;
    auto json = json_parser_obj.FromBuffer(config_with_lola_binding);

    auto config = configuration::Parse(std::move(json).value());
    concurrency::LongRunningThreadsContainer long_running_threads{};

    // creation of a LoLa runtime will lead to the creation of a message-passing-facade, which will directly from its
    // ctor register some MessageReceiveCallbacks and start listening, so we inject a receiver mock into the factory
    message_passing::ReceiverMock receiver_mock{};
    message_passing::ReceiverFactory::InjectReceiverMock(&receiver_mock);

    // EXPECT that Register is called successfully on created receivers
    EXPECT_CALL(receiver_mock, Register(_, (An<message_passing::IReceiver::MediumMessageReceivedCallback>())))
        .WillRepeatedly(Return());
    EXPECT_CALL(receiver_mock, Register(_, (An<message_passing::IReceiver::ShortMessageReceivedCallback>())))
        .WillRepeatedly(Return());

    // EXPECT that StartListening() is called 2 times (asil_qm and asil_b) successfully on created receivers
    EXPECT_CALL(receiver_mock, StartListening()).Times(2).WillRepeatedly(Return(amp::expected_blank<bmw::os::Error>{}));

    auto runtimes = RuntimeBindingFactory::CreateBindingRuntimes(config, long_running_threads, {});
    EXPECT_EQ(runtimes.size(), 1);

    const auto& lola_runtime = runtimes[BindingType::kLoLa];
    EXPECT_EQ(lola_runtime->GetBindingType(), BindingType::kLoLa);
}

}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
