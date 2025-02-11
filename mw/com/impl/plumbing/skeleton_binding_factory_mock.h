// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#ifndef PLATFORM_AAS_MW_COM_IMPL_PLUMBING_SKELETON_BINDING_FACTORY_MOCK_H
#define PLATFORM_AAS_MW_COM_IMPL_PLUMBING_SKELETON_BINDING_FACTORY_MOCK_H

#include "platform/aas/mw/com/impl/plumbing/i_skeleton_binding_factory.h"

#include <gmock/gmock.h>

namespace bmw
{
namespace mw
{
namespace com
{
namespace impl
{

class SkeletonBindingFactoryMock : public ISkeletonBindingFactory
{
  public:
    MOCK_METHOD(std::unique_ptr<SkeletonBinding>, Create, (const InstanceIdentifier&), (noexcept, override));
};

}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_PLUMBING_SKELETON_BINDING_FACTORY_MOCK_H
