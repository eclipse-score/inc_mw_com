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


#include "platform/aas/mw/com/impl/configuration/quality_type.h"

#include <exception>

namespace bmw
{
namespace mw
{
namespace com
{
namespace impl
{

namespace
{

constexpr auto kInvalidString = "kInvalid";
constexpr auto kAsilQmString = "kASIL_QM";
constexpr auto kAsilBString = "kASIL_B";

}  // namespace

std::string ToString(QualityType quality_type) noexcept
{
    switch (quality_type)
    {
        //  Return will terminate this switch clause
        case QualityType::kInvalid:
            return kInvalidString;
        //  Return will terminate this switch clause
        case QualityType::kASIL_QM:
            return kAsilQmString;
        //  Return will terminate this switch clause
        case QualityType::kASIL_B:
            return kAsilBString;
        //  Return will terminate this switch clause
        default:
            std::terminate();
    }
}

QualityType FromString(std::string quality_type) noexcept
{
    if (quality_type == kInvalidString)
    {
        return QualityType::kInvalid;
    }
    else if (quality_type == kAsilQmString)
    {
        return QualityType::kASIL_QM;
    }
    else if (quality_type == kAsilBString)
    {
        return QualityType::kASIL_B;
    }
    else
    {
        std::terminate();
    }
}

auto areCompatible(const QualityType& lhs, const QualityType& rhs) -> bool
{
    return lhs == rhs;
}

std::ostream& operator<<(std::ostream& ostream_out, const QualityType& type)
{
    switch (type)
    {
        case QualityType::kInvalid:
            ostream_out << "Invalid";
            break;
        case QualityType::kASIL_QM:
            ostream_out << "QM";
            break;
        case QualityType::kASIL_B:
            ostream_out << 'B';
            break;
        default:
            ostream_out << "(unknown)";
            break;
    }

    return ostream_out;
}

}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
