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


#include "platform/aas/mw/com/impl/bindings/lola/element_fq_id.h"
#include "platform/aas/mw/log/logging.h"

#include <cstdint>
#include <exception>
#include <limits>
#include <string>

namespace bmw::mw::com::impl::lola
{

ElementFqId::ElementFqId() noexcept
    : service_id_{std::numeric_limits<std::uint16_t>::max()},
      element_id_{std::numeric_limits<std::uint8_t>::max()},
      instance_id_{std::numeric_limits<std::uint16_t>::max()},
      element_type_{ElementType::INVALID}
{
}

ElementFqId::ElementFqId(const std::uint16_t service_id,
                         const std::uint8_t element_id,
                         const std::uint16_t instance_id,
                         const std::uint8_t element_type) noexcept
    : ElementFqId(service_id, element_id, instance_id, static_cast<ElementType>(element_type))
{
    if (element_type > static_cast<std::uint8_t>(ElementType::FIELD))
    {
        bmw::mw::log::LogFatal("lola") << "ElementFqId::ElementFqId failed: Invalid ElementType:" << element_type;
        std::terminate();
    }
}

ElementFqId::ElementFqId(const std::uint16_t service_id,
                         const std::uint8_t element_id,
                         const std::uint16_t instance_id,
                         const ElementType element_type) noexcept
    : service_id_{service_id}, element_id_{element_id}, instance_id_{instance_id}, element_type_{element_type}
{
}

std::string ElementFqId::ToString() const noexcept
{
    const std::string result = std::string("ElementFqId{S:") + std::to_string(static_cast<std::uint32_t>(service_id_)) +
                               std::string(", E:") + std::to_string(static_cast<std::uint32_t>(element_id_)) +
                               std::string(", I:") + std::to_string(static_cast<std::uint32_t>(instance_id_)) +
                               std::string(", T:") + std::to_string(static_cast<std::uint32_t>(element_type_)) +
                               std::string("}");
    return result;
}

bool IsElementEvent(const ElementFqId& element_fq_id) noexcept
{
    return element_fq_id.element_type_ == ElementType::EVENT;
}

bool IsElementField(const ElementFqId& element_fq_id) noexcept
{
    return element_fq_id.element_type_ == ElementType::FIELD;
}

bool operator==(const ElementFqId& lhs, const ElementFqId& rhs) noexcept
{
    return ((lhs.service_id_ == rhs.service_id_) && (lhs.element_id_ == rhs.element_id_) &&
            (lhs.instance_id_ == rhs.instance_id_));
}

bool operator<(const ElementFqId& lhs, const ElementFqId& rhs) noexcept
{
    if (lhs.service_id_ == rhs.service_id_)
    {
        if (lhs.instance_id_ == rhs.instance_id_)
        {
            return lhs.element_id_ < rhs.element_id_;
        }
        return lhs.instance_id_ < rhs.instance_id_;
    }
    return lhs.service_id_ < rhs.service_id_;
}

::bmw::mw::log::LogStream& operator<<(::bmw::mw::log::LogStream& log_stream, const ElementFqId& element_fq_id)
{
    log_stream << element_fq_id.ToString();
    return log_stream;
}

}  // namespace bmw::mw::com::impl::lola
