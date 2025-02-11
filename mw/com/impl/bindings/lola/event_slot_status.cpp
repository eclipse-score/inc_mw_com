// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#include "platform/aas/mw/com/impl/bindings/lola/event_slot_status.h"

namespace
{
/// \brief Indicates that the event was never written.
constexpr bmw::mw::com::impl::lola::EventSlotStatus::value_type INVALID = 0U;

/// \brief Indicates that the event data is altered and one should not increase the refcount.
constexpr bmw::mw::com::impl::lola::EventSlotStatus::value_type IN_WRITING =
    std::numeric_limits<bmw::mw::com::impl::lola::EventSlotStatus::SubscriberCount>::max();
}  // namespace

bmw::mw::com::impl::lola::EventSlotStatus::EventSlotStatus(const value_type init_val) noexcept : data_{init_val} {}

bmw::mw::com::impl::lola::EventSlotStatus::EventSlotStatus(const EventTimeStamp timestamp,
                                                           const SubscriberCount refcount) noexcept
    : data_{0}
{
    SetTimeStamp(timestamp);
    SetReferenceCount(refcount);
}

auto bmw::mw::com::impl::lola::EventSlotStatus::GetReferenceCount() const noexcept -> SubscriberCount
{
    return static_cast<SubscriberCount>(data_ & 0x00000000FFFFFFFFU);  // ignore first 4 byte
}

auto bmw::mw::com::impl::lola::EventSlotStatus::GetTimeStamp() const noexcept -> EventTimeStamp
{
    return static_cast<EventTimeStamp>(data_ >> 32U);  // ignore last 4 byte
}

auto bmw::mw::com::impl::lola::EventSlotStatus::IsInvalid() const noexcept -> bool
{
    return data_ == INVALID;
}

auto bmw::mw::com::impl::lola::EventSlotStatus::IsInWriting() const noexcept -> bool
{
    return data_ == IN_WRITING;
}

auto bmw::mw::com::impl::lola::EventSlotStatus::MarkInWriting() noexcept -> void
{
    data_ = IN_WRITING;
}

auto bmw::mw::com::impl::lola::EventSlotStatus::MarkInvalid() noexcept -> void
{
    data_ = INVALID;
}

auto bmw::mw::com::impl::lola::EventSlotStatus::SetTimeStamp(const EventTimeStamp time_stamp) noexcept -> void
{
    data_ = static_cast<value_type>(time_stamp) << 32U;
}

auto bmw::mw::com::impl::lola::EventSlotStatus::SetReferenceCount(const SubscriberCount ref_count) noexcept -> void
{
    data_ = (data_ & 0xFFFFFFFF00000000U) | static_cast<value_type>(ref_count);
}

auto bmw::mw::com::impl::lola::EventSlotStatus::IsTimeStampBetween(const EventTimeStamp min_timestamp,
                                                                   const EventTimeStamp max_timestamp) const noexcept
    -> bool
{
    return ((IsInWriting() == false) && (IsInvalid() == false) && (GetTimeStamp() > min_timestamp) &&
            (GetTimeStamp() < max_timestamp));
}

bmw::mw::com::impl::lola::EventSlotStatus::operator value_type&() noexcept
{
    // 
    return data_;
}

bmw::mw::com::impl::lola::EventSlotStatus::operator const value_type&() const noexcept
{
    return data_;
}

auto bmw::mw::com::impl::lola::EventSlotStatus::IsUsed() const noexcept -> bool
{
    return ((GetReferenceCount() != static_cast<SubscriberCount>(0)) || IsInWriting());
}
