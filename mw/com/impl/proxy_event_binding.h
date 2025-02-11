// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#ifndef PLATFORM_AAS_MW_COM_IMPL_PROXY_EVENT_BINDING_H
#define PLATFORM_AAS_MW_COM_IMPL_PROXY_EVENT_BINDING_H

#include "platform/aas/mw/com/impl/plumbing/sample_ptr.h"
#include "platform/aas/mw/com/impl/proxy_event_binding_base.h"
#include "platform/aas/mw/com/impl/sample_reference_tracker.h"
#include "platform/aas/mw/com/impl/tracing/i_tracing_runtime.h"

#include <amp_callback.hpp>

#include <cstddef>
#include <utility>

namespace bmw
{
namespace mw
{
namespace com
{
namespace impl
{

/// \brief Base class for all proxy event binding classes inside the binding implementation.
///
/// This class contains all type-aware definitions of the proxy side for events. All proxy event binding implementations
/// are required to derive from this class.
///
/// Due to limitations of the language it is not possible to move the type parameter to GetNewSamples ond/or Callback.
/// Therefore the whole lass needs to be parameterized to work around this.
///
/// \tparam SampleType Data type that is transmitted
template <typename SampleType>
class ProxyEventBinding : public ProxyEventBindingBase
{
  public:
    /// Type-erased callback used for the GetNewSamples method.
    ///
    /// The size of 80U is chosen to allow us to store another amp::callback within this callback. This is needed as we
    /// wrap the user provided callback in order to perform tracing functionality.
    using Callback = amp::callback<void(SamplePtr<SampleType>, tracing::ITracingRuntime::TracePointDataId), 80U>;

    /// \brief Get pending data from the event.
    ///
    /// The user needs to provide a callback which will be called for each sample
    /// that is available at the time of the call. Notice that the number of callback calls cannot
    /// exceed std::min(GetFreeSampleCount(), max_num_samples) times.
    ///
    /// \param receiver Callback that will be used to hand over data to the upper layer.
    /// \param reference_tracker Tracker that is used to produce reference counted SamplePtrs.
    /// \return Number of samples that were handed over to the callable.
    virtual Result<std::size_t> GetNewSamples(Callback&& receiver, TrackerGuardFactory& reference_tracker) noexcept = 0;

  protected:
    ProxyEventBinding() = default;

    /// Create a binding-independent SamplePtr from a binding-specific sample pointer.
    ///
    /// This serves as a placeholder to facilitate more complex construction in the future (read: when reference
    /// counting will be implemented for the proxy side).
    ///
    /// \tparam SampleType Data type that is transmitted by the sample pointer.
    /// \tparam BindingSamplePtr The sample pointer from the binding.
    /// \param binding_ptr Type of the binding-specific sample pointer.
    /// \param reference_guard Reference counting guard managing the count of SamplePtrs that are alive.
    /// \return Binding-independent SamplePtr instance.
    template <typename BindingSamplePtr>
    static SamplePtr<SampleType> MakeSamplePtr(BindingSamplePtr&& binding_ptr,
                                               SampleReferenceGuard reference_guard) noexcept;
};

template <typename SampleType>
template <typename BindingSamplePtr>
inline SamplePtr<SampleType> ProxyEventBinding<SampleType>::MakeSamplePtr(BindingSamplePtr&& binding_ptr,
                                                                          SampleReferenceGuard reference_guard) noexcept
{
    return SamplePtr<SampleType>{std::forward<BindingSamplePtr>(binding_ptr), std::move(reference_guard)};
}

}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_PROXY_EVENT_BINDING_H
