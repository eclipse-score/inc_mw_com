// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#ifndef PLATFORM_AAS_MW_COM_IMPL_BINDING_TYPE_H
#define PLATFORM_AAS_MW_COM_IMPL_BINDING_TYPE_H

#include <cstdint>

namespace bmw
{
namespace mw
{
namespace com
{
namespace impl
{

enum class BindingType : std::uint8_t
{
    kLoLa = 0x00,
    kFake = 0x01,
    kSomeIp = 0x02,
};

}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_BINDING_TYPE_H
