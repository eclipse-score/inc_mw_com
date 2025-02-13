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


#ifndef PLATFORM_AAS_MW_COM_IMPL_FINDSERVICEHANDLE_H
#define PLATFORM_AAS_MW_COM_IMPL_FINDSERVICEHANDLE_H

#include <cstddef>
#include <optional>

namespace bmw
{
namespace mw
{
namespace com
{
namespace impl
{

class FindServiceHandle;
class FindServiceHandleView;

/**
 * \brief A FindServiceHandle is returned by any StartFindService() method and is
 * used to identify different searches. It needs to be passed to StopFindService()
 * in order to cancel a respective search.
 *
 * \requirement 
 */
class FindServiceHandle final
{
  public:
    FindServiceHandle() = delete;
    ~FindServiceHandle() noexcept = default;

    /**
     * \brief CopyAssignment for FindServiceHandle
     *
     * \post *this == other
     * \param other The FindServiceHandle *this shall be constructed from
     * \return The FindServiceHandle that was constructed
     */
    FindServiceHandle& operator=(const FindServiceHandle& other) = default;
    FindServiceHandle(const FindServiceHandle& other) = default;
    FindServiceHandle(FindServiceHandle&& other) noexcept = default;
    FindServiceHandle& operator=(FindServiceHandle&& other) noexcept = default;

    /**
     * \brief Compares two instances for equality
     *
     * \param lhs The first instance to check for equality
     * \param rhs The second instance to check for equality
     * \return true if lhs and rhs equal, false otherwise
     */
    friend bool operator==(const FindServiceHandle& lhs, const FindServiceHandle& rhs) noexcept;

    /**
     * \brief LessThanComparable operator
     *
     * \param lhs The first FindServiceHandle instance to compare
     * \param rhs The second FindServiceHandle instance to compare
     * \return true if lhs is less then rhs, false otherwise
     */
    friend bool operator<(const FindServiceHandle& lhs, const FindServiceHandle& rhs) noexcept;

  private:
    std::size_t uid_;

    explicit FindServiceHandle(const std::size_t uid);

    friend FindServiceHandle make_FindServiceHandle(const std::size_t uid);

    friend FindServiceHandleView;
};

/**
 * \brief A make_ function is introduced to hide the Constructor of FindServiceHandle.
 * The FindServiceHandle will be exposed to the API user and by not having a public constructor
 * we can avoid that by chance the user will construct this class. Introducing a custom make method
 * that is _not_ mentioned in the standard, will avoid this!
 *
 * \param uid The unique identifier for a FindServiceHandle
 * \return A constructed FindServiceHandle
 */
FindServiceHandle make_FindServiceHandle(const std::size_t uid);

/**
 * \brief The bmw::mw::com::FindServiceHandle API is described by the ara::com standard.
 * But we also need to use it for internal purposes, because we need to access some state information
 * that is not exposed by the public API described in the adaptive AUTOSAR Standard.
 * In order to not leak implementation details, we come up with a `View` onto the FindServiceHandle.
 * Since our view is anyhow _only_ located in the `impl` namespace, there is zero probability that
 * any well minded user would depend on it.
 */
class FindServiceHandleView
{
  public:
    
    constexpr explicit FindServiceHandleView(const FindServiceHandle& handle) : handle_{handle} {}
    

    constexpr std::size_t getUid() const { return handle_.uid_; }

  private:
    const FindServiceHandle& handle_;
};

}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

namespace std
{
template <>
// NOLINTNEXTLINE(bmw-struct-usage-compliance): STL defines hash as struct.
struct hash<bmw::mw::com::impl::FindServiceHandle>
{
    std::size_t operator()(const bmw::mw::com::impl::FindServiceHandle& find_service_handle) const noexcept;
};
}  // namespace std

#endif  // PLATFORM_AAS_MW_COM_IMPL_FINDSERVICEHANDLE_H
