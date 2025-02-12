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


#ifndef PLATFORM_AAS_MW_COM_IMPL_TRACING_CONFIGURATION_SERVICE_ELEMENT_IDENTIFIER_H
#define PLATFORM_AAS_MW_COM_IMPL_TRACING_CONFIGURATION_SERVICE_ELEMENT_IDENTIFIER_H

#include "platform/aas/mw/com/impl/tracing/configuration/hash_helper_for_service_element_and_se_view.h"
#include "platform/aas/mw/com/impl/tracing/configuration/service_element_type.h"

#include "platform/aas/mw/log/logging.h"

#include <amp_string_view.hpp>

#include <array>
#include <cstring>
#include <exception>
#include <string>

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

/// \brief Binding independent unique identifier of a service element (I.e. event, field, method) which contains owning
/// strings.
///
/// A ServiceElementIdentifier cannot differentiate between the same service elements of different instances. For
/// that, ServiceElementInstanceIdentifierView should be used.
struct ServiceElementIdentifier
{
    std::string service_type_name;
    std::string service_element_name;
    ServiceElementType service_element_type;
};

bool operator==(const ServiceElementIdentifier& lhs, const ServiceElementIdentifier& rhs) noexcept;
bool operator<(const ServiceElementIdentifier& lhs, const ServiceElementIdentifier& rhs) noexcept;

::bmw::mw::log::LogStream& operator<<(::bmw::mw::log::LogStream& log_stream,
                                      const ServiceElementIdentifier& service_element_identifier);

}  // namespace tracing
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

namespace std
{

template <>
// NOLINTNEXTLINE(bmw-struct-usage-compliance): STL defines hash as struct.
struct hash<bmw::mw::com::impl::tracing::ServiceElementIdentifier>
{
    size_t operator()(const bmw::mw::com::impl::tracing::ServiceElementIdentifier& value) const noexcept
    {
        return bmw::mw::com::impl::tracing::hash_helper(value);
    }
};

}  // namespace std

#endif  // PLATFORM_AAS_MW_COM_IMPL_TRACING_CONFIGURATION_SERVICE_ELEMENT_IDENTIFIER_H
