// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#ifndef PLATFORM_AAS_MW_COM_IMPL_CONFIGURATION_QUALITY_TYPE_H
#define PLATFORM_AAS_MW_COM_IMPL_CONFIGURATION_QUALITY_TYPE_H

#include <cstdint>
#include <ostream>

namespace bmw
{
namespace mw
{
namespace com
{
namespace impl
{

enum class QualityType : std::uint16_t
{
    kInvalid = 0x00,
    kASIL_QM = 0x01,
    kASIL_B = 0x02,
};

std::string ToString(QualityType quality_type) noexcept;

QualityType FromString(std::string quality_type) noexcept;


bool areCompatible(const QualityType& lhs, const QualityType& rhs);


std::ostream& operator<<(std::ostream& ostream_out, const QualityType& type);

}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_CONFIGURATION_QUALITY_TYPE_H
