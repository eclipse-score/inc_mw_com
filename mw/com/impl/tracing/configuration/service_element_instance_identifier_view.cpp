// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#include "platform/aas/mw/com/impl/tracing/configuration/service_element_instance_identifier_view.h"

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

bool operator==(const ServiceElementInstanceIdentifierView& lhs,
                const ServiceElementInstanceIdentifierView& rhs) noexcept
{
    return ((lhs.service_element_identifier_view == rhs.service_element_identifier_view) &&
            (lhs.instance_specifier == rhs.instance_specifier));
}

::bmw::mw::log::LogStream& operator<<(
    ::bmw::mw::log::LogStream& log_stream,
    const ServiceElementInstanceIdentifierView& service_element_instance_identifier_view)
{
    log_stream << "service id: " << service_element_instance_identifier_view.service_element_identifier_view
               << ", instance id: " << service_element_instance_identifier_view.instance_specifier;
    return log_stream;
}

}  // namespace tracing
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
