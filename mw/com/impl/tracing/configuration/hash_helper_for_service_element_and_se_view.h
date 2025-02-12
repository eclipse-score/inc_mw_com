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


#ifndef PLATFORM_AAS_MW_COM_IMPL_TRACING_STD_HASH_OVERLOAD_FOR_SERVICE_ELEMENT_AND_SE_VIEW_H
#define PLATFORM_AAS_MW_COM_IMPL_TRACING_STD_HASH_OVERLOAD_FOR_SERVICE_ELEMENT_AND_SE_VIEW_H

#include "platform/aas/mw/log/logging.h"

#include <array>
#include <type_traits>

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
struct ServiceElementIdentifierView;
struct ServiceElementIdentifier;

// bmw::mw::com::impl::tracing::ServiceElementIdentifierView
template <typename T,
          typename = std::enable_if<std::is_same_v<T, bmw::mw::com::impl::tracing::ServiceElementIdentifier> ||
                                    std::is_same_v<T, bmw::mw::com::impl::tracing::ServiceElementIdentifierView>>>
std::size_t hash_helper(const T& value) noexcept
{
    // To prevent dynamic memory allocations, we copy the input ServiceElementIdentifierView elements into a local
    // buffer and create a string view to the local buffer. We then calculate the hash of the string_view.
    constexpr std::size_t max_buffer_size{1024U};

    const auto size_to_copy_1 = value.service_type_name.size();
    const auto size_to_copy_2 = value.service_element_name.size();

    const auto input_value_size = size_to_copy_1 + size_to_copy_2 + sizeof(value.service_element_type);
    if (input_value_size > max_buffer_size)
    {
        bmw::mw::log::LogFatal() << "ServiceElementIdentifier data strings (service_type_name and "
                                    "service_element_name) are too long: size"
                                 << input_value_size << "should be less than"
                                 << max_buffer_size - sizeof(value.service_element_type) << ". Terminating.";
        std::terminate();
    }

    std::array<char, max_buffer_size> local_buffer{};

    std::copy(value.service_type_name.begin(), value.service_type_name.end(), local_buffer.begin());
    std::copy(value.service_element_name.begin(), value.service_element_name.end(), &local_buffer.at(size_to_copy_1));

    static_assert(sizeof(value.service_element_type) == sizeof(char), "service_element_type is not of size char");
    local_buffer.at(size_to_copy_1 + size_to_copy_2) = static_cast<char>(value.service_element_type);

    amp::string_view local_string_buffer{local_buffer.data(), input_value_size};
    return std::hash<amp::string_view>{}(local_string_buffer);
}

}  // namespace tracing
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_TRACING_STD_HASH_OVERLOAD_FOR_SERVICE_ELEMENT_AND_SE_VIEW_H
