// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#ifndef PLATFORM_AAS_MW_COM_IMPL_PLUMBING_SKELETON_EVENT_BINDING_FACTORY_H
#define PLATFORM_AAS_MW_COM_IMPL_PLUMBING_SKELETON_EVENT_BINDING_FACTORY_H

#include "platform/aas/mw/com/impl/bindings/lola/element_fq_id.h"
#include "platform/aas/mw/com/impl/bindings/lola/skeleton_event.h"
#include "platform/aas/mw/com/impl/configuration/service_type_deployment.h"
#include "platform/aas/mw/com/impl/instance_identifier.h"
#include "platform/aas/mw/com/impl/plumbing/i_skeleton_event_binding_factory.h"
#include "platform/aas/mw/com/impl/plumbing/skeleton_event_binding_factory_impl.h"
#include "platform/aas/mw/com/impl/skeleton_base.h"
#include "platform/aas/mw/com/impl/skeleton_event_binding.h"

#include <amp_assert.hpp>
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

/// \brief Class that dispatches to either a real SkeletonEventBindingFactoryImpl or a mocked version
/// SkeletonEventBindingFactoryMock, if a mock is injected.
template <typename SampleType>
class SkeletonEventBindingFactory final
{
  public:
    /// \brief See documentation in ISkeletonEventBindingFactory.
    static std::unique_ptr<SkeletonEventBinding<SampleType>> Create(const InstanceIdentifier& identifier,
                                                                    SkeletonBase& parent,
                                                                    const amp::string_view event_name) noexcept
    {
        return instance().Create(identifier, parent, event_name);
    }

    /// \brief Inject a mock ISkeletonEventBindingFactory. If a mock is injected, then all calls on
    /// SkeletonEventBindingFactory will be dispatched to the mock.
    static void InjectMockBinding(ISkeletonEventBindingFactory<SampleType>* mock) noexcept { mock_ = mock; }

  private:
    static ISkeletonEventBindingFactory<SampleType>& instance() noexcept;
    static ISkeletonEventBindingFactory<SampleType>* mock_;
};

template <typename SampleType>
auto SkeletonEventBindingFactory<SampleType>::instance() noexcept -> ISkeletonEventBindingFactory<SampleType>&
{
    if (mock_ != nullptr)
    {
        return *mock_;
    }

    static SkeletonEventBindingFactoryImpl<SampleType> instance{};
    return instance;
}

// Suppress "AUTOSAR_C++14_A3-1-1" rule finding. This rule states:" It shall be possible to include any header file
// in multiple translation units without violating the One Definition Rule".
template <typename SampleType>
//  This is false-positive, "mock_" is defined once
ISkeletonEventBindingFactory<SampleType>* SkeletonEventBindingFactory<SampleType>::mock_ = nullptr;

}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_PLUMBING_SKELETON_EVENT_BINDING_FACTORY_H
