// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#ifndef PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_DATA_TYPE_META_INFO_H
#define PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_DATA_TYPE_META_INFO_H

#include <cstddef>
#include <cstdint>

namespace bmw
{
namespace mw
{
namespace com
{
namespace impl
{
namespace lola
{

/// \brief Meta-info of a data type exchanged via mw::com/LoLa. I.e. can be the data type of an event/filed/method arg.
struct DataTypeMetaInfo
{
    //@todo -> std::uint64_t fingerprint
    std::size_t size_of_;
    std::uint8_t align_of_;
};

}  // namespace lola
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_DATA_TYPE_META_INFO_H
