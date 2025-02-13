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


#ifndef PLATFORM_AAS_MW_COM_IMPL_TRACING_CONFIGURATION_TRACE_POINT_KEY_H
#define PLATFORM_AAS_MW_COM_IMPL_TRACING_CONFIGURATION_TRACE_POINT_KEY_H

#include "platform/aas/mw/com/impl/tracing/configuration/service_element_identifier_view.h"

#include "platform/aas/mw/log/logging.h"

#include <amp_string_view.hpp>

#include <array>
#include <cstdint>
#include <cstring>
#include <exception>

namespace bmw
{
namespace mw
{
namespace com
{
namespace impl
{
namespace tracing
{

struct TracePointKey
{
    ServiceElementIdentifierView service_element{};
    std::uint8_t trace_point_type{};
};

bool operator==(const TracePointKey& lhs, const TracePointKey& rhs) noexcept;

}  // namespace tracing
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

namespace std
{

template <>
// NOLINTNEXTLINE(bmw-struct-usage-compliance): STL defines hash as struct.
struct hash<bmw::mw::com::impl::tracing::TracePointKey>
{
    size_t operator()(const bmw::mw::com::impl::tracing::TracePointKey& value) const noexcept
    {
        // To prevent dynamic memory allocations, we copy the input TracePointKey elements into a local buffer and
        // create a string view to the local buffer. We then calculate the hash of the string_view.
        constexpr std::size_t max_buffer_size{1024U};
        const auto input_value_size =
            value.service_element.service_type_name.size() + value.service_element.service_element_name.size() +
            sizeof(value.service_element.service_element_type) + sizeof(value.trace_point_type);
        if (input_value_size > max_buffer_size)
        {
            bmw::mw::log::LogFatal("lola")
                << "TracePointKey data strings (service_type_name and service_element_name) are too long: size"
                << input_value_size << "should be less than"
                << max_buffer_size -
                       (sizeof(value.service_element.service_element_type) + sizeof(value.trace_point_type))
                << ". Terminating.";
            std::terminate();
        }

        std::array<char, max_buffer_size> local_buffer{};
        const std::size_t size_of_service_type_name = value.service_element.service_type_name.size();
        const std::size_t size_of_service_element_name = value.service_element.service_element_name.size();

        static_cast<void>(std::copy(value.service_element.service_type_name.begin(),
                                    value.service_element.service_type_name.end(),
                                    local_buffer.begin()));
        static_cast<void>(std::copy(value.service_element.service_element_name.begin(),
                                    value.service_element.service_element_name.end(),
                                    &local_buffer.at(size_of_service_type_name)));

        static_assert(sizeof(value.service_element.service_element_type) == sizeof(char),
                      "trace_point_type is not of size char");
        local_buffer.at(size_of_service_type_name + size_of_service_element_name) =
            static_cast<char>(value.service_element.service_element_type);

        static_assert(sizeof(value.trace_point_type) == sizeof(char), "trace_point_type is not of size char");
        local_buffer.at(size_of_service_type_name + size_of_service_element_name + 1) =
            static_cast<char>(value.trace_point_type);

        amp::string_view local_string_buffer{local_buffer.data(), input_value_size};
        return std::hash<amp::string_view>{}(local_string_buffer);
    }
};

}  // namespace std

#endif  // PLATFORM_AAS_MW_COM_IMPL_TRACING_CONFIGURATION_TRACE_POINT_KEY_H
