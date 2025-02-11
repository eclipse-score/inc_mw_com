// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#ifndef PLATFORM_AAS_MW_COM_IMPL_CONFIGURATION_SERVICE_VERSION_TYPE_H
#define PLATFORM_AAS_MW_COM_IMPL_CONFIGURATION_SERVICE_VERSION_TYPE_H

#include "platform/aas/lib/json/json_parser.h"

#include <amp_optional.hpp>
#include <amp_string_view.hpp>

#include <cstdint>
#include <string>
#include <utility>

namespace bmw
{
namespace mw
{
namespace com
{
namespace impl
{

class ServiceVersionType;
class ServiceVersionTypeView;
ServiceVersionType make_ServiceVersionType(const std::uint32_t major_version_number,
                                           const std::uint32_t minor_version_number);

/**
 * \brief Represents the Version of an ServiceInterface.
 *
 * This class is mentioned in the ara::com specification, but its implementation
 * specific. Meaning, content of the class shall not be made public or anyhow accessible,
 * besides the `ToString()` method and operators below.
 * That's also why no public constructor is given an this class needs to be constructed
 * by the given make_ServiceVersionType() method below - which is not for usage by an
 * ara::com API user.
 *
 * \requirement 
 */
class ServiceVersionType final
{
  public:
    explicit ServiceVersionType(const bmw::json::Object& json_object) noexcept;

    ServiceVersionType() = delete;
    ~ServiceVersionType() noexcept = default;

    /**
     * \brief CopyAssignment for ServiceVersionType
     *
     * \post *this == other
     * \param other The ServiceVersionType *this shall be constructed from
     * \return The ServiceVersionType that was constructed
     */
    ServiceVersionType& operator=(const ServiceVersionType& other) = default;
    ServiceVersionType(const ServiceVersionType&) = default;
    ServiceVersionType(ServiceVersionType&&) noexcept = default;
    ServiceVersionType& operator=(ServiceVersionType&& other) = default;

    /**
     * \brief Compares two instances for equality
     *
     * \param lhs The first instance to check for equality
     * \param rhs The second instance to check for equality
     * \return true if lhs and rhs equal, false otherwise
     */
    friend bool operator==(const ServiceVersionType& lhs, const ServiceVersionType& rhs) noexcept;

    /**
     * \brief extension for comparison with major/minor pair.
     *
     * \note It is perfectly valid to extend the SWS class impl. specific.
     * We do this since during configuration parsing we need efficient access to internal representation.
     *
     * @param sv
     * @param pair
     * @return true in case the pairs major/minor elements are equal to ServiceVersionType internal major/minor.
     */

    // (1) We require comparing ServiceVersionType with a std::pair, therefore, we tolerate an asymmetrical
    // comparison operator.
    // coverity [autosar_cpp14_a13_5_5_violation]
    friend bool operator==(const ServiceVersionType& sv, const std::pair<uint32_t, uint32_t>& pair) noexcept
    {
        return (((sv.major_ == pair.first) && (sv.minor_ == pair.second)));
    }

    /**
     * \brief LessThanComparable operator
     *
     * \param lhs The first ServiceVersionType instance to compare
     * \param rhs The second ServiceVersionType instance to compare
     * \return true if lhs is less then rhs, false otherwise
     */
    friend bool operator<(const ServiceVersionType& lhs, const ServiceVersionType& rhs) noexcept;

    /**
     * \brief Serializes the unknown internals of this class, to a meaningful string
     *
     * \return A non-owning string representation of the internals of this class
     */
    amp::string_view ToString() const noexcept;

    bmw::json::Object Serialize() const noexcept;

  private:
    std::uint32_t major_;
    std::uint32_t minor_;

    std::string serialized_string_;

    constexpr static std::uint32_t serializationVersion{1U};

    // User should not be able to create ServiceVersionTypeView it is meant only for internal use
    // -> declared as private access with friend class access.
    // Additionally, there is an AoU stating that Impl Namespace should not be used [No APIs from Implementation
    // Namespace]
    // 
    friend ServiceVersionTypeView;

    // User should not be able to create ServiceVersionType it is meant only for internal use
    // -> declared as private access with friend class access.
    // Additionally, there is an AoU stating that Impl Namespace should not be used [No APIs from Implementation
    // Namespace]
    // 
    friend ServiceVersionType make_ServiceVersionType(const std::uint32_t major_version_number,
                                                      const std::uint32_t minor_version_number);

    /**
     * @brief constructor for ServiceVersionType
     *
     *
     * @param major major version integer
     * @param minor minor version integer
     */
    explicit ServiceVersionType(const std::uint32_t major_version_number, const std::uint32_t minor_version_number);
};

/**
 * \brief A make_ function is introduced to hide the Constructor of ServiceVersionType.
 * The ServiceVersionType will be exposed to the API user and by not having a public constructor
 * we can avoid that by chance the user will construct this class. Introducing a custom make method
 * that is _not_ mentioned in the standard, will avoid this!
 *
 * \param major The major version number
 * \param minor The minor version number
 * \return A constructed ServiceVersionType from the provided numbers
 */
inline ServiceVersionType make_ServiceVersionType(const std::uint32_t major_version_number,
                                                  const std::uint32_t minor_version_number)
{
    return ServiceVersionType{major_version_number, minor_version_number};
}

/**
 * \brief The ServiceVersionType API is described by the ara::com standard.
 * But we also need to use it for internal purposes, where we need access to internal impl. details,
 * that is not exposed by the public API described in the adaptive AUTOSAR Standard.
 * In order to not leak implementation details, we come up with a `View` onto the ServiceVersionType.
 * Since our view is anyhow _only_ located in the `impl` namespace, there is zero probability that
 * any well minded user would depend on it.
 */
class ServiceVersionTypeView
{
  public:
    constexpr explicit ServiceVersionTypeView(const ServiceVersionType& type) : service_version_type_(type){};

    constexpr inline std::uint32_t getMajor() const { return service_version_type_.major_; };

    constexpr inline std::uint32_t getMinor() const { return service_version_type_.minor_; };
    // FP: only one statement in this line
    // 
    constexpr static std::uint32_t GetSerializationVersion() { return ServiceVersionType::serializationVersion; };

  private:
    const ServiceVersionType& service_version_type_;
};

}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_CONFIGURATION_SERVICE_VERSION_TYPE_H
