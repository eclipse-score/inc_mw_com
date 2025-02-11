// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#include "platform/aas/mw/com/impl/tracing/configuration/service_element_identifier.h"

#include "platform/aas/mw/log/logging.h"

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

bool operator==(const ServiceElementIdentifier& lhs, const ServiceElementIdentifier& rhs) noexcept
{
    return ((lhs.service_type_name == rhs.service_type_name) &&
            (lhs.service_element_name == rhs.service_element_name) &&
            (lhs.service_element_type == rhs.service_element_type));
}

bool operator<(const ServiceElementIdentifier& lhs, const ServiceElementIdentifier& rhs) noexcept
{
    if (lhs.service_type_name != rhs.service_type_name)
    {
        return lhs.service_type_name < rhs.service_type_name;
    }
    else if (lhs.service_element_name != rhs.service_element_name)
    {
        return lhs.service_element_name < rhs.service_element_name;
    }
    else
    {
        return lhs.service_element_type < rhs.service_element_type;
    }
}

::bmw::mw::log::LogStream& operator<<(::bmw::mw::log::LogStream& log_stream,
                                      const ServiceElementIdentifier& service_element_identifier)
{
    log_stream << "service type: " << service_element_identifier.service_element_type
               << ", service element: " << service_element_identifier.service_element_name
               << ", service element type: " << service_element_identifier.service_element_type;
    return log_stream;
}

}  // namespace tracing
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
