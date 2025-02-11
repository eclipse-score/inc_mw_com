// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#ifndef PLATFORM_AAS_MW_COM_IMPL_CONFIGURATION_SHM_SIZE_CALC_MODE_H
#define PLATFORM_AAS_MW_COM_IMPL_CONFIGURATION_SHM_SIZE_CALC_MODE_H

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

enum class ShmSizeCalculationMode : std::uint8_t
{
    kEstimation = 0x00,
    kSimulation = 0x01,
};

std::ostream& operator<<(std::ostream& ostream_out, const ShmSizeCalculationMode& mode);

}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_CONFIGURATION_SHM_SIZE_CALC_MODE_H
