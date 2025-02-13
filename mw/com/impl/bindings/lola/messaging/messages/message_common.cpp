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


#include "platform/aas/mw/com/impl/bindings/lola/messaging/messages/message_common.h"

#include <type_traits>

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

using ShortMessagePayload = message_passing::ShortMessagePayload;

constexpr std::uint32_t kElementIdSize{8UL};
constexpr std::uint32_t kInstanceIdSize{16UL};
constexpr std::uint32_t kElementTypeSize{8UL};

static_assert(sizeof(ElementFqId::service_id_) * 8U == 16UL,
              "Expected that ElementFqId::service_id_ is 16 bits in size");
static_assert(sizeof(ElementFqId::element_id_) * 8U == 8U, "Expected that ElementFqId::element_id_ is 8 bits in size");
static_assert(sizeof(ElementFqId::instance_id_) * 8U == 16UL,
              "Expected that ElementFqId::instance_id_ is 16 bits in size");
static_assert(sizeof(ElementFqId::element_type_) * 8U == 8UL,
              "Expected that ElementFqId::element_type_ is 8 bits in size");

constexpr std::uint32_t k8BitMask{0x000000FFU};
constexpr std::uint32_t k16BitMask{0x0000FFFFU};

}  // namespace

ElementFqId ShortMsgPayloadToElementFqId(const ShortMessagePayload msg_payload) noexcept
{
    return {static_cast<std::uint16_t>((msg_payload >> (kElementTypeSize + kInstanceIdSize + kElementIdSize))),
            static_cast<std::uint8_t>((msg_payload >> (kElementTypeSize + kInstanceIdSize)) & k8BitMask),
            static_cast<std::uint16_t>((msg_payload >> kElementTypeSize) & k16BitMask),
            static_cast<std::uint8_t>(msg_payload & k8BitMask)};
}

message_passing::ShortMessagePayload ElementFqIdToShortMsgPayload(const ElementFqId& element_fq_id) noexcept
{
    return ((static_cast<ShortMessagePayload>(element_fq_id.service_id_)
             << (kElementTypeSize + kInstanceIdSize + kElementIdSize)) |
            (static_cast<ShortMessagePayload>(element_fq_id.element_id_) << (kElementTypeSize + kInstanceIdSize)) |
            (static_cast<ShortMessagePayload>(element_fq_id.instance_id_) << kElementTypeSize) |
            static_cast<ShortMessagePayload>(element_fq_id.element_type_));
}

}  // namespace lola
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
