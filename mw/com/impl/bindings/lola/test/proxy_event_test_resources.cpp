// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#include "platform/aas/mw/com/impl/bindings/lola/test/proxy_event_test_resources.h"

#include "platform/aas/mw/com/impl/bindings/lola/rollback_data.h"

#include "platform/aas/lib/memory/shared/memory_resource_registry.h"
#include "platform/aas/mw/com/impl/bindings/lola/messaging/message_passing_service_mock.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
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

namespace
{

const auto kControlChannelPrefix{"/lola-ctl-"};
const auto kDataChannelPrefix{"/lola-data-"};

using namespace ::bmw::memory::shared;

using ::testing::_;
using ::testing::InvokeWithoutArgs;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::StartsWith;

}  // namespace

GenericProxyEventAttorney::GenericProxyEventAttorney(GenericProxyEvent& generic_proxy_event) noexcept
    : generic_proxy_event_{generic_proxy_event}
{
}

Result<std::size_t> GenericProxyEventAttorney::GetNumNewSamplesAvailableImpl() const noexcept
{
    return generic_proxy_event_.GetNumNewSamplesAvailableImpl();
}

Result<std::size_t> GenericProxyEventAttorney::GetNewSamples(Callback&& receiver, TrackerGuardFactory& tracker) noexcept
{
    return generic_proxy_event_.GetNewSamples(std::move(receiver), tracker);
}

Result<std::size_t> GenericProxyEventAttorney::GetNewSamplesImpl(Callback&& receiver,
                                                                 TrackerGuardFactory& tracker) noexcept
{
    return generic_proxy_event_.GetNewSamplesImpl(std::move(receiver), tracker);
}

ProxyEventCommon& GenericProxyEventAttorney::GetProxyEventCommon() noexcept
{
    return generic_proxy_event_.proxy_event_common_;
}

ProxyEventCommonAttorney::ProxyEventCommonAttorney(ProxyEventCommon& proxy_event_common) noexcept
    : proxy_event_common_{proxy_event_common}
{
}

void ProxyEventCommonAttorney::InjectSlotCollector(SlotCollector&& slot_collector)
{
    proxy_event_common_.InjectSlotCollector(std::move(slot_collector));
}

ProxyMockedMemoryFixture::ProxyMockedMemoryFixture() noexcept
{
    ON_CALL(runtime_mock_.mock_, GetBindingRuntime(BindingType::kLoLa))
        .WillByDefault(::testing::Return(&binding_runtime_));
    ON_CALL(binding_runtime_, GetUid()).WillByDefault(::testing::Return(kDummyUid));
    ON_CALL(binding_runtime_, GetPid()).WillByDefault(::testing::Return(kDummyPid));
    ON_CALL(runtime_mock_.mock_, GetServiceDiscovery()).WillByDefault(ReturnRef(service_discovery_mock_));

    ExpectControlSegmentOpened();
    ExpectDataSegmentOpened();

    ON_CALL(*(fake_data_.control_memory), getUsableBaseAddress())
        .WillByDefault(Return(static_cast<void*>(fake_data_.data_control)));
    ON_CALL(*(fake_data_.data_memory), getUsableBaseAddress())
        .WillByDefault(Return(static_cast<void*>(fake_data_.data_storage)));

    ON_CALL(binding_runtime_, GetRollbackData()).WillByDefault(ReturnRef(rollback_data_));
}

void ProxyMockedMemoryFixture::ExpectControlSegmentOpened()
{
    ON_CALL(shared_memory_factory_mock_guard_.mock_, Open(StartsWith(kControlChannelPrefix), true, _))
        .WillByDefault(InvokeWithoutArgs(
            [this]() -> std::shared_ptr<memory::shared::ISharedMemoryResource> { return fake_data_.control_memory; }));
}

void ProxyMockedMemoryFixture::ExpectDataSegmentOpened()
{
    ON_CALL(shared_memory_factory_mock_guard_.mock_, Open(StartsWith(kDataChannelPrefix), false, _))
        .WillByDefault(InvokeWithoutArgs(
            [this]() -> std::shared_ptr<memory::shared::ISharedMemoryResource> { return fake_data_.data_memory; }));
}

void ProxyMockedMemoryFixture::InitialiseProxyWithConstructor(const InstanceIdentifier& instance_identifier)
{
    EnrichedInstanceIdentifier enriched_instance_identifier{instance_identifier};
    ON_CALL(service_discovery_mock_, StartFindService(_, enriched_instance_identifier))
        .WillByDefault(Return(make_FindServiceHandle(10U)));

    Proxy::EventNameToElementFqIdConverter event_name_to_element_fq_id_converter{lola_service_deployment_,
                                                                                 lola_service_instance_id_.id_};
    parent_ = std::make_unique<Proxy>(fake_data_.control_memory,
                                      fake_data_.data_memory,
                                      service_quality_type_,
                                      std::move(event_name_to_element_fq_id_converter),
                                      make_HandleType(instance_identifier),
                                      amp::optional<memory::shared::LockFile>{},
                                      nullptr);
}

void ProxyMockedMemoryFixture::InitialiseProxyWithCreate(const InstanceIdentifier& instance_identifier)
{
    EnrichedInstanceIdentifier enriched_instance_identifier{instance_identifier};
    ON_CALL(service_discovery_mock_, StartFindService(_, enriched_instance_identifier))
        .WillByDefault(Return(make_FindServiceHandle(10U)));

    parent_ = Proxy::Create(make_HandleType(instance_identifier));
}

void ProxyMockedMemoryFixture::InitialiseDummySkeletonEvent(const ElementFqId element_fq_id,
                                                            const SkeletonEventProperties& skeleton_event_properties)
{
    std::tie(event_control_, event_data_storage_) =
        fake_data_.AddEvent<SampleType>(element_fq_id, skeleton_event_properties);
}

LolaProxyEventResources::LolaProxyEventResources() : ProxyMockedMemoryFixture{}
{
    InitialiseProxyWithConstructor(identifier_);
    InitialiseDummySkeletonEvent(element_fq_id_, SkeletonEventProperties{max_num_slots_, max_subscribers_, true});
}

LolaProxyEventResources::~LolaProxyEventResources()
{
    bmw::memory::shared::MemoryResourceRegistry::getInstance().clear();
}

std::future<BindingEventReceiveHandler> LolaProxyEventResources::ExpectRegisterEventNotification(
    amp::optional<pid_t> pid)
{
    const auto pid_to_use = pid.has_value() ? pid.value() : kDummyPid;

    // Since gtest currently seems to require that the provided lambdas are copyable, we can't move the
    // local_handler_promise into the state of the lambda using a generalized lambda captures. Instead we pass it in
    // using a shared_ptr
    auto local_handler_promise = std::make_shared<std::promise<BindingEventReceiveHandler>>();
    auto local_handler_future = local_handler_promise->get_future();

    EXPECT_CALL(*mock_service_,
                RegisterEventNotification(QualityType::kASIL_QM, element_fq_id_, ::testing::_, pid_to_use))
        .WillOnce(::testing::Invoke(::testing::WithArg<2>([local_handler_promise](BindingEventReceiveHandler handler)
                                                              -> IMessagePassingService::HandlerRegistrationNoType {
            local_handler_promise->set_value(std::move(handler));
            IMessagePassingService::HandlerRegistrationNoType registration_no{0};
            return registration_no;
        })))
        .RetiresOnSaturation();
    return local_handler_future;
}

void LolaProxyEventResources::ExpectReregisterEventNotification(amp::optional<pid_t> pid)
{
    const auto pid_to_use = pid.has_value() ? pid.value() : kDummyPid;
    EXPECT_CALL(*mock_service_, ReregisterEventNotification(QualityType::kASIL_QM, element_fq_id_, pid_to_use))
        .RetiresOnSaturation();
}

void LolaProxyEventResources::ExpectUnregisterEventNotification(amp::optional<pid_t> pid)
{
    const auto pid_to_use = pid.has_value() ? pid.value() : kDummyPid;
    EXPECT_CALL(*mock_service_,
                UnregisterEventNotification(QualityType::kASIL_QM, element_fq_id_, ::testing::_, pid_to_use));
}

EventDataControl::SlotIndexType LolaProxyEventResources::PutData(const std::uint32_t value,
                                                                 const EventSlotStatus::EventTimeStamp timestamp)
{
    auto slot_result = event_control_->data_control.AllocateNextSlot();
    EXPECT_TRUE(slot_result.has_value());
    auto slot = *slot_result;
    event_data_storage_->at(slot) = value;
    event_control_->data_control.EventReady(slot, timestamp);
    return slot;
}

}  // namespace lola
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
