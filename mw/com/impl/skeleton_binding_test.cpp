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


#include "platform/aas/mw/com/impl/skeleton_binding.h"

#include <gtest/gtest.h>
#include <type_traits>

namespace bmw
{
namespace mw
{
namespace com
{
namespace impl
{
namespace
{

class MySkeleton final : public SkeletonBinding
{
  public:
    ResultBlank PrepareOffer(SkeletonEventBindings&,
                             SkeletonFieldBindings&,
                             amp::optional<RegisterShmObjectTraceCallback>) noexcept override
    {
        return {};
    }
    ResultBlank FinalizeOffer() noexcept override { return {}; }
    void PrepareStopOffer(amp::optional<UnregisterShmObjectTraceCallback>) noexcept override {}
    BindingType GetBindingType() const noexcept override { return BindingType::kFake; };
};

TEST(SkeletonBindingTest, SkeletonBindingShouldNotBeCopyable)
{
    static_assert(!std::is_copy_constructible<MySkeleton>::value, "Is wrongly copyable");
    static_assert(!std::is_copy_assignable<MySkeleton>::value, "Is wrongly copyable");
}

TEST(SkeletonBindingTest, SkeletonBindingShouldNotBeMoveable)
{
    static_assert(!std::is_move_constructible<MySkeleton>::value, "Is wrongly moveable");
    static_assert(!std::is_move_assignable<MySkeleton>::value, "Is wrongly moveable");
}

}  // namespace
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
