// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#ifndef PLATFORM_AAS_MW_COM_IMPL_TRACING_CONFIGURATION_SERVICE_ELEMENT_INSTANCE_IDENTIFIER_VIEW_H
#define PLATFORM_AAS_MW_COM_IMPL_TRACING_CONFIGURATION_SERVICE_ELEMENT_INSTANCE_IDENTIFIER_VIEW_H

#include "platform/aas/mw/com/impl/tracing/configuration/service_element_identifier_view.h"

#include <amp_string_view.hpp>

#include <array>
#include <cstring>
#include <exception>
#include <string>

namespace bmw
{
namespace mw
{
namespace log
{
class LogStream;
}
namespace com
{
namespace impl
{
namespace tracing
{

/// \brief Binding independent unique identifier of an instance of a service element (I.e. event, field, method) which
/// contains owning strings.
struct ServiceElementInstanceIdentifierView
{
    ServiceElementIdentifierView service_element_identifier_view;
    amp::string_view instance_specifier;
};

bool operator==(const ServiceElementInstanceIdentifierView& lhs,
                const ServiceElementInstanceIdentifierView& rhs) noexcept;

::bmw::mw::log::LogStream& operator<<(
    ::bmw::mw::log::LogStream& log_stream,
    const ServiceElementInstanceIdentifierView& service_element_instance_identifier_view);

}  // namespace tracing
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

namespace std
{

template <>
// NOLINTNEXTLINE(bmw-struct-usage-compliance): STL defines hash as struct.
struct hash<bmw::mw::com::impl::tracing::ServiceElementInstanceIdentifierView>
{
    size_t operator()(const bmw::mw::com::impl::tracing::ServiceElementInstanceIdentifierView& value) const noexcept
    {
        const auto& service_element_identifier_view = value.service_element_identifier_view;
        const auto& instance_specifier = value.instance_specifier;

        // To prevent dynamic memory allocations, we copy the input ServiceElementIdentifier elements into a local
        // buffer and create a string view to the local buffer. We then calculate the hash of the string_view.
        constexpr std::size_t max_buffer_size{1024U};
        const auto input_value_size = service_element_identifier_view.service_type_name.size() +
                                      service_element_identifier_view.service_element_name.size() +
                                      sizeof(service_element_identifier_view.service_element_type) +
                                      instance_specifier.size();
        if (input_value_size > max_buffer_size)
        {
            bmw::mw::log::LogFatal("lola")
                << "ServiceElementInstanceIdentifierView data strings (service_type_name, "
                   "service_element_name and instance_specifier) are too long: size"
                << input_value_size << "should be less than"
                << max_buffer_size - sizeof(service_element_identifier_view.service_element_type) << ". Terminating.";
            std::terminate();
        }

        std::array<char, max_buffer_size> local_buffer{};

        auto first_free_pos = local_buffer.begin();

        first_free_pos = std::copy(service_element_identifier_view.service_type_name.begin(),
                                   service_element_identifier_view.service_type_name.end(),
                                   first_free_pos);
        first_free_pos = std::copy(service_element_identifier_view.service_element_name.begin(),
                                   service_element_identifier_view.service_element_name.end(),
                                   first_free_pos);
        first_free_pos = std::copy(instance_specifier.begin(), instance_specifier.end(), first_free_pos);

        static_assert(sizeof(service_element_identifier_view.service_element_type) == sizeof(char),
                      "service_element_type is not of size char");
        *first_free_pos = static_cast<char>(service_element_identifier_view.service_element_type);

        amp::string_view local_string_buffer{local_buffer.data(), input_value_size};
        return std::hash<amp::string_view>{}(local_string_buffer);
    }
};

}  // namespace std

#endif  // PLATFORM_AAS_MW_COM_IMPL_TRACING_CONFIGURATION_SERVICE_ELEMENT_INSTANCE_IDENTIFIER_VIEW_H
