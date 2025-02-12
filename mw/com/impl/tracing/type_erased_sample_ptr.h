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


#ifndef PLATFORM_AAS_MW_COM_IMPL_TRACING_TYPE_ERASED_SAMPLE_PTR_H
#define PLATFORM_AAS_MW_COM_IMPL_TRACING_TYPE_ERASED_SAMPLE_PTR_H

#include <amp_callback.hpp>

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

class TypeErasedSamplePtr
{
  public:
    template <typename SamplePtrType>
    explicit TypeErasedSamplePtr(SamplePtrType sample_ptr)
        : type_erased_sample_ptr_{[sample_ptr = std::move(sample_ptr)] { static_cast<void>(sample_ptr); }}
    {
    }

  private:
    amp::callback<void(), 128U> type_erased_sample_ptr_;
};

}  // namespace tracing
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_TRACING_TYPE_ERASED_SAMPLE_PTR_H
