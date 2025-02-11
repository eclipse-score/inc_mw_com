// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#ifndef PLATFORM_AAS_MW_COM_MESSAGE_PASSING_RESMGR_TRAITS_COMMON_H
#define PLATFORM_AAS_MW_COM_MESSAGE_PASSING_RESMGR_TRAITS_COMMON_H

#include <amp_static_vector.hpp>
#include <amp_string_view.hpp>

#include <array>

namespace bmw
{
namespace mw
{
namespace com
{
namespace message_passing
{

inline constexpr amp::string_view GetQnxPrefix() noexcept
{
    using amp::literals::operator""_sv;
    // NOLINTNEXTLINE(bmw-no-user-defined-literals): _sv is not user defined but a C++17 provided by amp
    return "/mw_com/message_passing"_sv;
}

class QnxResourcePath
{
  public:
    explicit QnxResourcePath(const amp::string_view identifier) noexcept;

    const char* c_str() const noexcept { return buffer_.data(); }

    static constexpr std::size_t kMaxIdentifierLen = 256U;

  private:
    amp::static_vector<char, GetQnxPrefix().size() + kMaxIdentifierLen + 1U> buffer_;
};

}  // namespace message_passing
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_MESSAGE_PASSING_RESMGR_TRAITS_COMMON_H
