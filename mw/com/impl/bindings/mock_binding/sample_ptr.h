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


#ifndef PLATFORM_AAS_MW_COM_IMPL_BINDINGS_MOCK_BINDING_SAMPLE_PTR_H
#define PLATFORM_AAS_MW_COM_IMPL_BINDINGS_MOCK_BINDING_SAMPLE_PTR_H

#include <amp_callback.hpp>

#include <memory>

namespace bmw
{
namespace mw
{
namespace com
{
namespace impl
{
namespace mock_binding
{

template <typename SampleType>
using CustomDeleter = amp::callback<void(SampleType*)>;

/// \brief SamplePtr used for the mock binding.
///
/// The SamplePtr is an alias for a unique_ptr with a custom deleter. If no deleter is provided, a default deleter
/// will be used. A custom deleter must be supplied when SampleType == void, as calling delete on a void pointer is
/// undefined behaviour (as it's unclear what destructor to call on a void pointer).
///
/// @tparam SampleType The data that is transmitted via the mock proxy.
template <typename SampleType>
using SamplePtr = std::unique_ptr<SampleType, CustomDeleter<SampleType>>;

}  // namespace mock_binding
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_BINDINGS_MOCK_BINDING_SAMPLE_PTR_H
