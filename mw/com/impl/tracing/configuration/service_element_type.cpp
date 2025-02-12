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


#include "platform/aas/mw/com/impl/tracing/configuration/service_element_type.h"

#include "platform/aas/mw/log/logging.h"

::bmw::mw::log::LogStream& bmw::mw::com::impl::tracing::operator<<(
    ::bmw::mw::log::LogStream& log_stream,
    const bmw::mw::com::impl::tracing::ServiceElementType& service_element_type)
{
    switch (service_element_type)
    {
        case bmw::mw::com::impl::tracing::ServiceElementType::INVALID:
            log_stream << "INVALID";
            break;
        case bmw::mw::com::impl::tracing::ServiceElementType::EVENT:
            log_stream << "EVENT";
            break;
        case bmw::mw::com::impl::tracing::ServiceElementType::FIELD:
            log_stream << "FIELD";
            break;
        case bmw::mw::com::impl::tracing::ServiceElementType::METHOD:
            log_stream << "METHOD";
            break;
        default:
            log_stream << "UNKNOWN";
    }
    return log_stream;
}