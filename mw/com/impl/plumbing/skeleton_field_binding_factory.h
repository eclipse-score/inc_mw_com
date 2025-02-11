// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#ifndef PLATFORM_AAS_MW_COM_IMPL_PLUMBING_SKELETON_FIELD_BINDING_FACTORY_H
#define PLATFORM_AAS_MW_COM_IMPL_PLUMBING_SKELETON_FIELD_BINDING_FACTORY_H

#include "platform/aas/mw/com/impl/bindings/lola/element_fq_id.h"
#include "platform/aas/mw/com/impl/bindings/lola/skeleton_event.h"
#include "platform/aas/mw/com/impl/configuration/service_type_deployment.h"
#include "platform/aas/mw/com/impl/instance_identifier.h"
#include "platform/aas/mw/com/impl/plumbing/i_skeleton_field_binding_factory.h"
#include "platform/aas/mw/com/impl/plumbing/skeleton_field_binding_factory_impl.h"
#include "platform/aas/mw/com/impl/skeleton_base.h"
#include "platform/aas/mw/com/impl/skeleton_event_binding.h"

#include <amp_string_view.hpp>

#include <functional>
#include <memory>
#include <utility>

namespace bmw
{
namespace mw
{
namespace com
{
namespace impl
{

/// \brief Class that dispatches to either a real SkeletonFieldBindingFactoryImpl or a mocked version
/// SkeletonFieldBindingFactoryMock, if a mock is injected.
template <typename SampleType>
class SkeletonFieldBindingFactory final
{
  public:
    /// \brief See documentation in ISkeletonFieldBindingFactory.
    static std::unique_ptr<SkeletonEventBinding<SampleType>> CreateEventBinding(const InstanceIdentifier& identifier,
                                                                                SkeletonBase& parent,
                                                                                const amp::string_view field_name)
    {
        return instance().CreateEventBinding(identifier, parent, field_name);
    }

    /// \brief Inject a mock ISkeletonFieldBindingFactory. If a mock is injected, then all calls on
    /// SkeletonFieldBindingFactory will be dispatched to the mock.
    static void InjectMockBinding(ISkeletonFieldBindingFactory<SampleType>* mock) noexcept { mock_ = mock; }

  private:
    static ISkeletonFieldBindingFactory<SampleType>& instance() noexcept;
    static ISkeletonFieldBindingFactory<SampleType>* mock_;
};

template <typename SampleType>
auto SkeletonFieldBindingFactory<SampleType>::instance() noexcept -> ISkeletonFieldBindingFactory<SampleType>&
{
    if (mock_ != nullptr)
    {
        return *mock_;
    }

    static SkeletonFieldBindingFactoryImpl<SampleType> instance{};
    return instance;
}

template <typename SampleType>
// 
ISkeletonFieldBindingFactory<SampleType>* SkeletonFieldBindingFactory<SampleType>::mock_{nullptr};

}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_PLUMBING_SKELETON_FIELD_BINDING_FACTORY_H
