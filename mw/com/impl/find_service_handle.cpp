// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#include "platform/aas/mw/com/impl/find_service_handle.h"

namespace bmw
{
namespace mw
{
namespace com
{
namespace impl
{

FindServiceHandle::FindServiceHandle(const std::size_t uid) : uid_{uid} {}

auto operator==(const FindServiceHandle& lhs, const FindServiceHandle& rhs) noexcept -> bool
{
    return lhs.uid_ == rhs.uid_;
}

auto operator<(const FindServiceHandle& lhs, const FindServiceHandle& rhs) noexcept -> bool
{
    return lhs.uid_ < rhs.uid_;
}

auto make_FindServiceHandle(const std::size_t uid) -> FindServiceHandle
{
    return FindServiceHandle(uid);
}

}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

namespace std
{

std::size_t hash<bmw::mw::com::impl::FindServiceHandle>::operator()(
    const bmw::mw::com::impl::FindServiceHandle& find_service_handle) const noexcept
{
    bmw::mw::com::impl::FindServiceHandleView view{find_service_handle};
    return view.getUid();
}

}  // namespace std
