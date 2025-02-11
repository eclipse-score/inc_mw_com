// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#include "platform/aas/mw/com/impl/instance_specifier.h"

#include "platform/aas/mw/com/impl/com_error.h"

#include <regex>

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

bool IsShortNameValid(const amp::string_view shortname) noexcept
{
    static std::regex check_characters_regex("^[A-Za-z_/][A-Za-z_/0-9]*", std::regex_constants::basic);
    static std::regex check_duplicate_slashes_regex("/$", std::regex_constants::basic);
    static std::regex check_trailing_slash_regex("/{2,}", std::regex_constants::extended);

    const bool all_characters_valid = std::regex_match(shortname.data(), check_characters_regex);
    const bool duplicate_slashes_found = std::regex_search(shortname.data(), check_duplicate_slashes_regex);
    const bool trailing_slash_found = std::regex_search(shortname.data(), check_trailing_slash_regex);

    return all_characters_valid && !duplicate_slashes_found && !trailing_slash_found;
}

}  // namespace

bmw::Result<InstanceSpecifier> InstanceSpecifier::Create(const amp::string_view shortname_path) noexcept
{
    if (!IsShortNameValid(shortname_path))
    {
        bmw::mw::log::LogWarn("lola") << "Shortname" << shortname_path
                                      << "does not adhere to shortname naming requirements.";
        return MakeUnexpected(ComErrc::kInvalidMetaModelShortname);
    }

    const InstanceSpecifier instance_specifier{shortname_path};
    return instance_specifier;
}

amp::string_view InstanceSpecifier::ToString() const noexcept
{
    return instance_specifier_string_;
}

InstanceSpecifier::InstanceSpecifier(const amp::string_view shortname_path) noexcept
    : instance_specifier_string_{shortname_path.data(), shortname_path.size()}
{
}

auto operator==(const InstanceSpecifier& lhs, const InstanceSpecifier& rhs) noexcept -> bool
{
    return lhs.ToString() == rhs.ToString();
}

bool operator==(const InstanceSpecifier& lhs, const amp::string_view& rhs) noexcept
{
    return lhs.ToString() == rhs;
}

bool operator==(const amp::string_view& lhs, const InstanceSpecifier& rhs) noexcept
{
    return lhs == rhs.ToString();
}

auto operator!=(const InstanceSpecifier& lhs, const InstanceSpecifier& rhs) noexcept -> bool
{
    return !(lhs == rhs);
}

bool operator!=(const InstanceSpecifier& lhs, const amp::string_view& rhs) noexcept
{
    return !(lhs == rhs);
}

bool operator!=(const amp::string_view& lhs, const InstanceSpecifier& rhs) noexcept
{
    return !(lhs == rhs);
}

auto operator<(const InstanceSpecifier& lhs, const InstanceSpecifier& rhs) noexcept -> bool
{
    return lhs.ToString() < rhs.ToString();
}

auto operator<<(::bmw::mw::log::LogStream& log_stream, const InstanceSpecifier& instance_specifier)
    -> ::bmw::mw::log::LogStream&
{
    log_stream << instance_specifier.ToString();
    return log_stream;
}

}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
