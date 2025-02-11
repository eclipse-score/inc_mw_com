// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#ifndef PLATFORM_AAS_MW_COM_IMPL_PLUMBING_SKELETON_BINDING_FACTORY_IMPL_H
#define PLATFORM_AAS_MW_COM_IMPL_PLUMBING_SKELETON_BINDING_FACTORY_IMPL_H

#include "platform/aas/mw/com/impl/instance_identifier.h"
#include "platform/aas/mw/com/impl/plumbing/i_skeleton_binding_factory.h"
#include "platform/aas/mw/com/impl/skeleton_binding.h"

#include <amp_span.hpp>

#include <memory>

namespace bmw
{
namespace mw
{
namespace com
{
namespace impl
{

/// \brief Factory class that dispatches calls to the appropriate binding based on binding information in the
/// deployment configuration.
class SkeletonBindingFactoryImpl : public ISkeletonBindingFactory
{
  public:
    /// \brief Creates the necessary binding based on the deployment information associated in the InstanceIdentifier
    ///
    /// \details Currently supports only Shared Memory
    /// \return A SkeletonBinding instance on valid deployment information, nullptr otherwise
    std::unique_ptr<SkeletonBinding> Create(const InstanceIdentifier&) noexcept override;
};

}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_PLUMBING_SKELETON_BINDING_FACTORY_IMPL_H
