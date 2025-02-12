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


#include "platform/aas/mw/com/impl/proxy_binding.h"

#include <amp_string_view.hpp>

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

class MyProxy final : public ProxyBinding
{
  public:
    bool IsEventProvided(const amp::string_view) const noexcept override { return true; }
    void RegisterEventBinding(amp::string_view, ProxyEventBindingBase&) noexcept override {}
    void UnregisterEventBinding(amp::string_view) noexcept override {}
};

TEST(ProxyBindingTest, ProxyBindingShouldNotBeCopyable)
{
    static_assert(!std::is_copy_constructible<MyProxy>::value, "Is wrongly copyable");
    static_assert(!std::is_copy_assignable<MyProxy>::value, "Is wrongly copyable");
}

TEST(ProxyBindingTest, ProxyBindingShouldNotBeMoveable)
{
    static_assert(!std::is_move_constructible<MyProxy>::value, "Is wrongly moveable");
    static_assert(!std::is_move_assignable<MyProxy>::value, "Is wrongly moveable");
}

}  // namespace
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
