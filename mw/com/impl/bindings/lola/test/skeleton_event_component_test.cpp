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


#include "platform/aas/mw/com/impl/bindings/lola/skeleton_event.h"
#include "platform/aas/mw/com/impl/bindings/lola/test/skeleton_test_resources.h"

#include "platform/aas/lib/memory/shared/memory_resource_registry.h"
#include "platform/aas/lib/memory/shared/shared_memory_factory.h"
#include "platform/aas/mw/com/impl/bindings/lola/messaging/message_passing_service_mock.h"
#include "platform/aas/mw/com/impl/bindings/lola/runtime_mock.h"
#include "platform/aas/mw/com/impl/bindings/lola/shm_path_builder.h"
#include "platform/aas/mw/com/impl/runtime.h"
#include "platform/aas/mw/com/impl/runtime_mock.h"
#include "platform/aas/mw/com/impl/service_discovery_mock.h"

#include <amp_string_view.hpp>
#include <amp_variant.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <utility>
#include <vector>

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

using SkeletonEventSampleType = std::uint32_t;

template <std::size_t MaxSamples>
class SkeletonEventComponentTestTemplateFixture : public ::testing::Test
{
  public:
    void SetUp() override
    {
        impl::Runtime::InjectMock(&runtime_mock_);
        ON_CALL(runtime_mock_, GetBindingRuntime).WillByDefault(::testing::Return(&lola_runtime_mock_));
        ON_CALL(lola_runtime_mock_, GetLolaMessaging)
            .WillByDefault(::testing::ReturnRef(message_passing_service_mock_));
        ON_CALL(runtime_mock_, GetServiceDiscovery()).WillByDefault(::testing::ReturnRef(service_discovery_mock_));

        SkeletonBinding::SkeletonEventBindings events{};
        SkeletonBinding::SkeletonFieldBindings fields{};
        events.emplace(fake_event_name_, skeleton_event_);
        const auto prepare_offer_result = parent_skeleton_->PrepareOffer(events, fields, {});
        ASSERT_TRUE(prepare_offer_result.has_value());
    }

    void TearDown() override
    {
        parent_skeleton_->PrepareStopOffer({});
        parent_skeleton_.reset();
        bmw::memory::shared::MemoryResourceRegistry::getInstance().clear();

        const auto is_regular_file_data =
            bmw::filesystem::IStandardFilesystem::instance().IsRegularFile("/dev/shm/lola-data-0000000000000002-00016");
        ASSERT_TRUE(is_regular_file_data.has_value());
        EXPECT_FALSE(is_regular_file_data.value());

        const auto is_regular_file_ctl =
            bmw::filesystem::IStandardFilesystem::instance().IsRegularFile("/dev/shm/lola-ctl-0000000000000002-00016");
        ASSERT_TRUE(is_regular_file_ctl.has_value());
        EXPECT_FALSE(is_regular_file_ctl.value());

        const auto is_regular_file_ctl_b = bmw::filesystem::IStandardFilesystem::instance().IsRegularFile(
            "/dev/shm/lola-ctl-0000000000000002-00016-b");
        ASSERT_TRUE(is_regular_file_ctl_b.has_value());
        EXPECT_FALSE(is_regular_file_ctl_b.value());
        impl::Runtime::InjectMock(nullptr);
    }

    InstanceIdentifier GetValidInstanceIdentifier() noexcept
    {
        return make_InstanceIdentifier(valid_asil_instance_deployment_, valid_type_deployment_);
    }

    std::uint32_t GetLastSendEvent() const
    {
        const auto lola_service_type_deployment =
            amp::get<LolaServiceTypeDeployment>(valid_type_deployment_.binding_info_);
        ShmPathBuilder path_builder{lola_service_type_deployment.service_id_};

        const auto lola_service_instance_deployment =
            amp::get<LolaServiceInstanceDeployment>(valid_asil_instance_deployment_.bindingInfo_);
        const auto instance_id = lola_service_instance_deployment.instance_id_.value().id_;

        auto memory =
            bmw::memory::shared::SharedMemoryFactory::Open(path_builder.GetDataChannelShmName(instance_id), false);

        auto* storage = static_cast<ServiceDataStorage*>(memory->getUsableBaseAddress());
        auto* value = static_cast<EventDataStorage<std::uint32_t>*>(storage->events_.at(fake_element_fq_id_).get());

        auto path = path_builder.GetControlChannelShmName(instance_id, QualityType::kASIL_QM);
        auto memory_control = bmw::memory::shared::SharedMemoryFactory::Open(path, false);
        auto* control_storage = static_cast<ServiceDataControl*>(memory_control->getUsableBaseAddress());

        auto& event_data_control = control_storage->event_controls_.find(fake_element_fq_id_)->second.data_control;
        event_data_control.GetTransactionLogSet().RegisterSkeletonTracingElement();
        auto slot = event_data_control.ReferenceNextEvent(0, TransactionLogSet::kSkeletonIndexSentinel);
        EXPECT_TRUE(slot.has_value());
        return value->at(slot.value());
    }

    std::size_t GetFreeSampleSlots() const
    {
        std::size_t result{0};
        const auto lola_service_type_deployment =
            amp::get<LolaServiceTypeDeployment>(valid_type_deployment_.binding_info_);
        ShmPathBuilder path_builder{lola_service_type_deployment.service_id_};

        const auto lola_service_instance_deployment =
            amp::get<LolaServiceInstanceDeployment>(valid_asil_instance_deployment_.bindingInfo_);
        const auto instance_id = lola_service_instance_deployment.instance_id_.value().id_;

        auto path = path_builder.GetControlChannelShmName(instance_id, QualityType::kASIL_QM);
        auto memory_control = bmw::memory::shared::SharedMemoryFactory::Open(path, false);
        auto* control_storage = static_cast<ServiceDataControl*>(memory_control->getUsableBaseAddress());
        const auto search = control_storage->event_controls_.find(fake_element_fq_id_);
        EXPECT_TRUE(search != control_storage->event_controls_.cend());
        const auto* event_control = &search->second;

        for (EventDataControl::SlotIndexType i = 0; i < MaxSamples; i++)
        {
            if ((*event_control).data_control[i].IsInvalid())
            {
                result++;
            }
        }
        return result;
    }

    const std::uint8_t max_subscribers_{3U};
    const bool enforce_max_samples_{true};
    const ElementFqId fake_element_fq_id_{1, 1, 1, ElementType::EVENT};
    const std::string fake_event_name_{"dummy"};
    const InstanceSpecifier instance_specifier_{InstanceSpecifier::Create("/my_dummy_instance_specifier").value()};

    LolaServiceInstanceDeployment binding_info_{CreateLolaServiceInstanceDeployment(
        test::kDefaultLolaInstanceId,
        {{fake_event_name_, LolaEventInstanceDeployment{MaxSamples, 10U, 1U, true}}},
        {},
        {},
        {},
        test::kConfiguredDeploymentShmSize)};

    /// \brief A very basic (Lola) ServiceTypeDeployment, which just contains a service-id and NO events at all!
    /// \details For some of the basic tests, this is sufficient and since services without events are a valid use
    /// case
    ///          (at least later, when we also support fields/service-methods).
    LolaServiceId service_id_{2U};
    const ServiceTypeDeployment valid_type_deployment_{CreateTypeDeployment(service_id_, {{fake_event_name_, 42U}})};

    /// \brief A very basic (Lola) ASIL-B ServiceInstanceDeployment, which relates to the
    ///        valid_type_deployment_ and has a shm-size configuration of 500.
    /// \note same setup as valid_qm_instance_deployment, but ASIL-QM and ASIL-B.
    ServiceInstanceDeployment valid_asil_instance_deployment_{make_ServiceIdentifierType("foo"),
                                                              binding_info_,
                                                              QualityType::kASIL_B,
                                                              instance_specifier_};

    std::unique_ptr<Skeleton> parent_skeleton_{
        Skeleton::Create(GetValidInstanceIdentifier(),
                         filesystem::FilesystemFactory{}.CreateInstance(),
                         std::make_unique<ShmPathBuilder>(service_id_),
                         std::make_unique<PartialRestartPathBuilder>(service_id_))};
    SkeletonEvent<SkeletonEventSampleType> skeleton_event_{
        *parent_skeleton_,
        fake_element_fq_id_,
        fake_event_name_,
        SkeletonEventProperties{MaxSamples, max_subscribers_, enforce_max_samples_}};

    /// mocks used by test
    impl::RuntimeMock runtime_mock_;
    lola::RuntimeMock lola_runtime_mock_;
    MessagePassingServiceMock message_passing_service_mock_;
    ServiceDiscoveryMock service_discovery_mock_{};
};

using SkeletonEventComponentTestFixture = SkeletonEventComponentTestTemplateFixture<5>;
TEST_F(SkeletonEventComponentTestFixture, CanAllocateAndSendEvent)
{
    RecordProperty("Verifies", ", SSR-6225206, 0, 3");
    RecordProperty("Description",
                   "Checks whether a skeleton can send data into shared memory (req. , 3) and "
                   "slot allocation works (req. SSR-6225206, 0).");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("Priority", "1");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    // Given an offered event in an offered service
    const auto prepare_offer_result = skeleton_event_.PrepareOffer();
    ASSERT_TRUE(prepare_offer_result.has_value());

    // When allocating and sending the allocated event
    auto slot_result = skeleton_event_.Allocate();
    ASSERT_TRUE(slot_result.has_value());
    auto slot = std::move(slot_result).value();

    *slot = 5;

    // expect, that an event update notification is sent for QM and ASIL-B
    EXPECT_CALL(message_passing_service_mock_, NotifyEvent(QualityType::kASIL_QM, fake_element_fq_id_));
    EXPECT_CALL(message_passing_service_mock_, NotifyEvent(QualityType::kASIL_B, fake_element_fq_id_));
    skeleton_event_.Send(std::move(slot), {});

    // Then the send event in shared memory can be found by a proxy
    EXPECT_EQ(GetLastSendEvent(), 5);
}

TEST_F(SkeletonEventComponentTestFixture, CanSendByValue)
{
    RecordProperty("Verifies", ", 5");
    RecordProperty("Description",
                   "Sends an event sample by Copy on send via shared-memory (req. , 5).");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("Priority", "1");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    // When offering the event
    const auto prepare_offer_result = skeleton_event_.PrepareOffer();
    ASSERT_TRUE(prepare_offer_result.has_value());

    // store the number of free slots before sending ...
    auto free_slots_before = GetFreeSampleSlots();

    // When  sending by value
    skeleton_event_.Send(5, {});

    // Then the send event in shared memory can be found by a proxy
    EXPECT_EQ(GetLastSendEvent(), 5);
    // and the number of free slots has decreased by one
    EXPECT_EQ(GetFreeSampleSlots(), free_slots_before - 1);
}

TEST_F(SkeletonEventComponentTestFixture, SkeletonWillCalculateEventMetaInfoFromSkeletonEventType)
{
    RecordProperty("Verifies", "8");
    RecordProperty("Description",
                   "Checks that the type meta information is calculated based on the provided event / field type.");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    // Given a Skeleton containing a SkeletonEvent which has been offered
    const auto prepare_offer_result = skeleton_event_.PrepareOffer();
    ASSERT_TRUE(prepare_offer_result.has_value());

    // When getting the EventMetaInfo for the skeleton event
    const auto event_meta_info = parent_skeleton_->GetEventMetaInfo(fake_element_fq_id_);

    // Then the event meta info should correspond to the type of the skeleton event
    ASSERT_TRUE(event_meta_info.has_value());
    EXPECT_EQ(event_meta_info.value().data_type_info_.align_of_, alignof(SkeletonEventSampleType));
    EXPECT_EQ(event_meta_info.value().data_type_info_.size_of_, sizeof(SkeletonEventSampleType));
}

using SkeletonEventSingleSlotComponentTestFixture = SkeletonEventComponentTestTemplateFixture<1>;
TEST_F(SkeletonEventSingleSlotComponentTestFixture, SendByValueReturnsErrorIfSlotCannotBeAllocated)
{
    // When offering the event
    const auto prepare_offer_result = skeleton_event_.PrepareOffer();
    ASSERT_TRUE(prepare_offer_result.has_value());

    // Allocate a slot so that there are no free slots remaining
    const auto slot_result = skeleton_event_.Allocate();
    ASSERT_TRUE(slot_result.has_value());

    // When sending by value
    const auto send_result = skeleton_event_.Send(5, {});

    // Then the result should contain an error indicating that the allocation failes
    ASSERT_FALSE(send_result.has_value());
    EXPECT_EQ(send_result.error(), ComErrc::kSampleAllocationFailure);
}

TEST_F(SkeletonEventSingleSlotComponentTestFixture, SendByValueFreesSampleAllocateePtrAfterReturning)
{
    RecordProperty("Verifies", "");
    RecordProperty(
        "Description",
        "Sends an event sample by Copy and verifies that the Sample Allocatee Ptr that gets allocated is destroyed.");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    // When offering the event
    const auto prepare_offer_result = skeleton_event_.PrepareOffer();
    ASSERT_TRUE(prepare_offer_result.has_value());

    // Expect that there is only one slot available
    EXPECT_EQ(GetFreeSampleSlots(), 1);

    // and when calling Send twice
    const auto send_result_1 = skeleton_event_.Send(5, {});
    const auto send_result_2 = skeleton_event_.Send(5, {});

    // Then both sends return no errors indicating that each call allocated a slot and freed it before returning
    ASSERT_TRUE(send_result_1.has_value());
    ASSERT_TRUE(send_result_2.has_value());
}

}  // namespace
}  // namespace lola
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
