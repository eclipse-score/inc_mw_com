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


#ifndef PLATFORM_AAS_MW_COM_IMPL_FLAG_OWNER_H
#define PLATFORM_AAS_MW_COM_IMPL_FLAG_OWNER_H

namespace bmw
{
namespace mw
{
namespace com
{
namespace impl
{

/// \brief Helper class which maintains a flag that has a single owner.
/// When an object of this class is move constructed or move assigned, the flag of the moved-from object will be
/// cleared.
class FlagOwner
{
  public:
    FlagOwner() : flag_{false} {}
    ~FlagOwner() = default;

    FlagOwner(const FlagOwner&) = delete;
    FlagOwner& operator=(const FlagOwner&) & = delete;

    FlagOwner(FlagOwner&& other) noexcept : flag_{other.flag_} { other.Clear(); }

    FlagOwner& operator=(FlagOwner&& other) & noexcept
    {
        if (this != &other)
        {
            flag_ = other.flag_;
            other.Clear();
        }
        return *this;
    }

    void Set() noexcept { flag_ = true; }
    void Clear() noexcept { flag_ = false; }

    bool IsSet() const noexcept { return flag_; }

  private:
    bool flag_;
};

}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_FLAG_OWNER_H
