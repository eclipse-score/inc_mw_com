// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#ifndef PLATFORM_AAS_MW_COM_IMPL_INSTANCE_SPECIFIER_H
#define PLATFORM_AAS_MW_COM_IMPL_INSTANCE_SPECIFIER_H

#include "platform/aas/lib/result/result.h"
#include "platform/aas/mw/log/logging.h"

#include <amp_string_view.hpp>

#include <string>

namespace bmw
{
namespace mw
{
namespace com
{
namespace impl
{

class InstanceSpecifier
{

    friend std::hash<InstanceSpecifier>;

  public:
    static bmw::Result<InstanceSpecifier> Create(const amp::string_view shortname_path) noexcept;

    amp::string_view ToString() const noexcept;

  private:
    explicit InstanceSpecifier(const amp::string_view shortname_path) noexcept;

    std::string instance_specifier_string_;
};

bool operator==(const InstanceSpecifier& lhs, const InstanceSpecifier& rhs) noexcept;
bool operator==(const InstanceSpecifier& lhs, const amp::string_view& rhs) noexcept;
bool operator==(const amp::string_view& lhs, const InstanceSpecifier& rhs) noexcept;

bool operator!=(const InstanceSpecifier& lhs, const InstanceSpecifier& rhs) noexcept;
bool operator!=(const InstanceSpecifier& lhs, const amp::string_view& rhs) noexcept;
bool operator!=(const amp::string_view& lhs, const InstanceSpecifier& rhs) noexcept;

bool operator<(const InstanceSpecifier& lhs, const InstanceSpecifier& rhs) noexcept;

::bmw::mw::log::LogStream& operator<<(::bmw::mw::log::LogStream& log_stream,
                                      const InstanceSpecifier& instance_specifier);

}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

namespace std
{

/// \brief InstanceSpecifier is used as a key for maps, so we need a hash func for it.
template <>
// NOLINTNEXTLINE(bmw-struct-usage-compliance): STL defines hash as struct.
struct hash<bmw::mw::com::impl::InstanceSpecifier>
{
    std::size_t operator()(const bmw::mw::com::impl::InstanceSpecifier& instance_specifier) const noexcept
    {
        return std::hash<std::string>{}(instance_specifier.instance_specifier_string_);
    }
};

}  // namespace std

#endif  // PLATFORM_AAS_MW_COM_IMPL_INSTANCE_SPECIFIER_H
