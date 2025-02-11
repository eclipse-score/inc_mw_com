// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#ifndef PLATFORM_AAS_MW_COM_IMPL_SKELETON_BINDING_H
#define PLATFORM_AAS_MW_COM_IMPL_SKELETON_BINDING_H

#include "platform/aas/lib/memory/shared/i_shared_memory_resource.h"
#include "platform/aas/lib/result/result.h"
#include "platform/aas/mw/com/impl/binding_type.h"
#include "platform/aas/mw/com/impl/tracing/configuration/service_element_type.h"

#include <amp_callback.hpp>
#include <amp_optional.hpp>
#include <amp_string_view.hpp>

#include <functional>
#include <map>

namespace bmw
{
namespace mw
{
namespace com
{
namespace impl
{

class SkeletonEventBindingBase;

/**
 * \brief The SkeletonBinding abstracts the interface that _every_ binding needs to provide. It will be used by a
 * concrete skeleton to perform _any_ operation in a then binding specific manner.
 */
class SkeletonBinding
{
  public:
    using SkeletonEventBindings = std::map<amp::string_view, std::reference_wrapper<SkeletonEventBindingBase>>;

    /// \brief callback type for registering shared-memory objects with tracing
    /// \details Needs only get used/called by bindings, which use shared-memory as their underlying communication/
    ///          data-exchange mechanism.
    using RegisterShmObjectTraceCallback =
        amp::callback<void(amp::string_view element_name,
                           tracing::ServiceElementType element_type,
                           memory::shared::ISharedMemoryResource::FileDescriptor shm_object_fd,
                           void* shm_memory_start_address),
                      64U>;

    /// \brief callback type for unregistering shared-memory objects with tracing
    /// \details Needs only get used/called by bindings, which use shared-memory as their underlying communication/
    ///          data-exchange mechanism.
    using UnregisterShmObjectTraceCallback =
        amp::callback<void(amp::string_view element_name, tracing::ServiceElementType element_type), 64U>;

    // For the moment, SkeletonFields use only SkeletonEventBindings. However, in the future when Get / Set are
    // supported in fields, then SkeletonFieldBindings will be a map in which the values are:
    // std::tuple<SkeletonEventBindingBase, SkeletonMethodBindingBase, SkeletonMethodBindingBase>
    using SkeletonFieldBindings = SkeletonEventBindings;

    SkeletonBinding() = default;

    // A SkeletonBinding is always held via a pointer in the binding independent impl::SkeletonBase.
    // Therefore, the binding itself doesn't have to be moveable or copyable, as the pointer can simply be copied when
    // moving the impl::SkeletonBase.
    SkeletonBinding(const SkeletonBinding&) = delete;
    SkeletonBinding(SkeletonBinding&&) noexcept = delete;

    
    SkeletonBinding& operator=(const SkeletonBinding&) & = delete;
    SkeletonBinding& operator=(SkeletonBinding&&) & noexcept = delete;
    
    virtual ~SkeletonBinding();

    /// \brief In case there are any binding specifics with regards to service offerings, this can be implemented within
    ///  this function. It shall be called before actually offering the service over the service discovery mechanism.
    ///  A SkeletonBinding doesn't know its events so they should be passed "on-demand" into PrepareOffer() in case it
    ///  needs the events in order to complete the offering.
    ///  The optional RegisterShmObjectTraceCallback is handed over, in case tracing is enabled for elements within this
    ///  skeleton instance. If it is handed over AND the binding is using shared-memory as its underlying data-exchange
    ///  mechanism, it must call this callback for each shm-object it will use.
    ///
    /// \return expects void
    virtual ResultBlank PrepareOffer(SkeletonEventBindings&,
                                     SkeletonFieldBindings&,
                                     amp::optional<RegisterShmObjectTraceCallback>) noexcept = 0;

    /**
     * \brief Indicates that the offering of the service is now completed. The binding should not allow a Proxy to find
     * this service until this function is called.
     *
     *  \return expects void
     */
    virtual ResultBlank FinalizeOffer() noexcept = 0;

    /**
     * \brief In case there are any binding specifics with regards to service withdrawals, this can be implemented
     * within this function. It shall be called before StopOffering() the service.
     *
     * \return void
     */
    virtual void PrepareStopOffer(amp::optional<UnregisterShmObjectTraceCallback>) noexcept = 0;

    /// \brief Gets the binding type of the binding
    virtual BindingType GetBindingType() const noexcept = 0;
};

}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_SKELETON_BINDING_H
