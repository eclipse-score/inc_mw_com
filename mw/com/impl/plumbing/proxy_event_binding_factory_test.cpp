// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#include "platform/aas/mw/com/impl/plumbing/proxy_event_binding_factory.h"
#include "platform/aas/mw/com/impl/bindings/lola/i_runtime.h"
#include "platform/aas/mw/com/impl/bindings/lola/messaging/i_message_passing_service.h"
#include "platform/aas/mw/com/impl/bindings/lola/messaging/message_passing_service_mock.h"
#include "platform/aas/mw/com/impl/bindings/lola/runtime_mock.h"
#include "platform/aas/mw/com/impl/bindings/lola/test/proxy_event_test_resources.h"
#include "platform/aas/mw/com/impl/bindings/lola/test_doubles/fake_service_data.h"
#include "platform/aas/mw/com/impl/bindings/mock_binding/proxy.h"
#include "platform/aas/mw/com/impl/bindings/mock_binding/proxy_event.h"
#include "platform/aas/mw/com/impl/configuration/lola_service_instance_deployment.h"
#include "platform/aas/mw/com/impl/configuration/service_identifier_type.h"
#include "platform/aas/mw/com/impl/configuration/service_instance_deployment.h"
#include "platform/aas/mw/com/impl/configuration/service_version_type.h"
#include "platform/aas/mw/com/impl/handle_type.h"
#include "platform/aas/mw/com/impl/instance_identifier.h"
#include "platform/aas/mw/com/impl/plumbing/dummy_instance_identifier_builder.h"
#include "platform/aas/mw/com/impl/runtime.h"
#include "platform/aas/mw/com/impl/runtime_mock.h"
#include "platform/aas/mw/com/impl/traits.h"

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
using ::testing::Invoke;
using ::testing::Return;
using ::testing::ReturnRef;

const auto kEventName{"Field1"};

using ProxyEventBindingFactoryFixture = lola::ProxyMockedMemoryFixture;
TEST_F(ProxyEventBindingFactoryFixture, CreateLolaProxy)
{
    RecordProperty("Verifies", "1, 2, ");
    RecordProperty("Description", "Checks whether a proxy event lola binding can be created and set at runtime");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("Priority", "1");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    const uint16_t instance_id = 0x31;
    const lola::ElementFqId element_fq_id{0x1337, 0x5, instance_id, lola::ElementType::EVENT};
    const lola::SkeletonEventProperties skeleton_event_properties{5, 3, true};

    // Now create proxy
    auto instance_specifier_result = InstanceSpecifier::Create("/my_dummy_instance_specifier");
    const auto service_id = make_ServiceIdentifierType("/a/service/somewhere/out/there", 13, 37);
    LolaServiceInstanceDeployment shm_binding_information{LolaServiceInstanceId{0x31}};
    shm_binding_information.events_[kEventName] = LolaEventInstanceDeployment{};

    ServiceInstanceDeployment deployment_information{service_id,
                                                     std::move(shm_binding_information),
                                                     QualityType::kASIL_B,
                                                     std::move(instance_specifier_result).value()};

    LolaServiceTypeDeployment lola_service_type_deployment{0x1337};
    lola_service_type_deployment.events_[kEventName] = 5U;
    const ServiceTypeDeployment type_deployment{lola_service_type_deployment};

    auto instance_identifier = make_InstanceIdentifier(deployment_information, type_deployment);

    InitialiseProxyWithCreate(instance_identifier);
    InitialiseDummySkeletonEvent(element_fq_id, skeleton_event_properties);

    const auto handle = make_HandleType(std::move(instance_identifier));
    ProxyBase proxy_base{std::move(parent_), handle};

    std::unique_ptr<ProxyEventBinding<std::uint8_t>> proxy_event =
        ProxyEventBindingFactory<std::uint8_t>::Create(proxy_base, kEventName);
    ASSERT_NE(proxy_event, nullptr);
    EXPECT_EQ(proxy_event->GetSubscriptionState(), SubscriptionState::kNotSubscribed);

    auto slot = event_control_->data_control.AllocateNextSlot().value();
    event_data_storage_->at(slot) = 42;
    event_control_->data_control.EventReady(slot, 1);

    SampleReferenceTracker tracker{2U};
    auto guard_factory{tracker.Allocate(1U)};

    proxy_event->Subscribe(2U);
    EXPECT_EQ(proxy_event->GetSubscriptionState(), SubscriptionState::kSubscribed);
    proxy_event->GetNewSamples(
        [](SamplePtr<std::uint8_t> sample, tracing::ITracingRuntime::TracePointDataId timestamp) {
            EXPECT_EQ(*sample, 42);
            EXPECT_EQ(timestamp, 1);
        },
        guard_factory);
}

}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
