// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#include "platform/aas/mw/com/impl/bindings/lola/slot_collector.h"

#include "platform/aas/mw/com/impl/bindings/lola/test_doubles/fake_memory_resource.h"

#include <gtest/gtest.h>

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

constexpr std::size_t kMaxSubscribers{5U};
const TransactionLogId kDummyTransactionLogId{10U};

using namespace ::bmw::memory::shared;
class SlotCollectorWithFakeMem : public ::testing::Test
{
  protected:
    static EventDataControl::SlotIndexType AllocateSlot(EventDataControl& event_data_control,
                                                        const EventSlotStatus::EventTimeStamp timestamp = 1)
    {
        const auto slot = event_data_control.AllocateNextSlot();
        EXPECT_TRUE(slot.has_value());
        event_data_control.EventReady(*slot, timestamp);
        return *slot;
    }

    std::size_t CalculateNumberOfCollectedSlots(const SlotCollector::SlotIndices& indices)
    {
        return static_cast<std::size_t>(std::distance(indices.begin, indices.end));
    }
    FakeMemoryResource fake_memory_resource_;
};

TEST_F(SlotCollectorWithFakeMem, TestProperEventAcquisition)
{
    EventDataControl event_data_control{5, fake_memory_resource_.getMemoryResourceProxy(), kMaxSubscribers};
    const auto transaction_log_index =
        event_data_control.GetTransactionLogSet().RegisterProxyElement(kDummyTransactionLogId).value();

    AllocateSlot(event_data_control);
    SlotCollector slot_collector{event_data_control, 1U, transaction_log_index};
    EXPECT_EQ(slot_collector.GetNumNewSamplesAvailable(), 1);

    const std::size_t max_count{1};
    const auto slot_indices = slot_collector.GetNewSamplesSlotIndices(max_count);

    EXPECT_EQ(CalculateNumberOfCollectedSlots(slot_indices), 1);
    EXPECT_EQ(*slot_indices.begin, 0);
}

TEST_F(SlotCollectorWithFakeMem, ReceiveEventsInOrder)
{
    EventDataControl event_data_control{4, fake_memory_resource_.getMemoryResourceProxy(), kMaxSubscribers};
    const auto transaction_log_index =
        event_data_control.GetTransactionLogSet().RegisterProxyElement(kDummyTransactionLogId).value();

    EventSlotStatus::EventTimeStamp send_time{1};
    const std::size_t num_values_to_send{3};
    for (std::size_t i = 0; i < num_values_to_send; ++i)
    {
        AllocateSlot(event_data_control, send_time);
        send_time++;
    }

    SlotCollector slot_collector{event_data_control, 3U, transaction_log_index};
    EXPECT_EQ(slot_collector.GetNumNewSamplesAvailable(), 3);

    const std::size_t max_count{3};
    const auto slot_indices = slot_collector.GetNewSamplesSlotIndices(max_count);

    EXPECT_EQ(CalculateNumberOfCollectedSlots(slot_indices), 3);

    std::size_t count = 0;
    for (auto it = slot_indices.begin; it != slot_indices.end; ++it)
    {
        EXPECT_EQ(*it, count);
        count++;
    }

    EXPECT_EQ(slot_collector.GetNumNewSamplesAvailable(), 0);
    const std::size_t new_max_count{15};
    const auto no_new_sample = slot_collector.GetNewSamplesSlotIndices(new_max_count);
    EXPECT_EQ(CalculateNumberOfCollectedSlots(no_new_sample), 0);
}

TEST_F(SlotCollectorWithFakeMem, DoNotReceiveEventsFromThePast)
{
    EventDataControl event_data_control{3, fake_memory_resource_.getMemoryResourceProxy(), kMaxSubscribers};
    const auto transaction_log_index =
        event_data_control.GetTransactionLogSet().RegisterProxyElement(kDummyTransactionLogId).value();
    SlotCollector slot_collector{event_data_control, 2U, transaction_log_index};

    AllocateSlot(event_data_control, 17);
    EXPECT_EQ(slot_collector.GetNumNewSamplesAvailable(), 1);

    const std::size_t max_count{37};
    const auto slot_indices = slot_collector.GetNewSamplesSlotIndices(max_count);

    EXPECT_EQ(CalculateNumberOfCollectedSlots(slot_indices), 1);

    AllocateSlot(event_data_control, 1);

    EXPECT_EQ(slot_collector.GetNumNewSamplesAvailable(), 0);
    const std::size_t new_max_count{38};
    const auto no_new_sample = slot_collector.GetNewSamplesSlotIndices(new_max_count);
    EXPECT_EQ(CalculateNumberOfCollectedSlots(no_new_sample), 0);
}

}  // namespace
}  // namespace lola
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
