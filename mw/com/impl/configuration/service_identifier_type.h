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


#ifndef PLATFORM_AAS_MW_COM_IMPL_CONFIGURATION_SERVICE_IDENTIFIER_TYPE_H
#define PLATFORM_AAS_MW_COM_IMPL_CONFIGURATION_SERVICE_IDENTIFIER_TYPE_H

#include "platform/aas/mw/com/impl/configuration/service_version_type.h"

#include "platform/aas/lib/json/json_parser.h"
#include "platform/aas/lib/memory/string_literal.h"

#include <amp_string_view.hpp>

#include <cstdint>
#include <string>

namespace bmw
{
namespace mw
{
namespace com
{
namespace impl
{

class ServiceIdentifierType;
class ServiceIdentifierTypeView;
ServiceIdentifierType make_ServiceIdentifierType(std::string, const std::uint32_t, const std::uint32_t);

/**
 * \brief Represents a unique identifier for a specific service
 *
 * This class is mentioned in the ara::com specification, but its implementation
 * specific. Meaning, content of the class shall not be made public or anyhow accessible,
 * besides the `ToString()` method and operators below.
 * That's also why no public constructor is given an this class needs to be constructed
 * by the given make_ServiceIdentifierType() method below - which is not for usage by an
 * ara::com API user.
 *
 * \requirement 
 */
class ServiceIdentifierType final
{
  public:
    explicit ServiceIdentifierType(const bmw::json::Object& json_object) noexcept;

    ServiceIdentifierType() = delete;
    ~ServiceIdentifierType() noexcept = default;

    /**
     * \brief CopyAssignment for ServiceIdentifierType
     *
     * \post *this == other
     * \param other The ServiceIdentifierType *this shall be constructed from
     * \return The ServiceIdentifierType that was constructed
     */
    ServiceIdentifierType& operator=(const ServiceIdentifierType& other) = default;
    ServiceIdentifierType(const ServiceIdentifierType&) = default;
    ServiceIdentifierType(ServiceIdentifierType&&) noexcept = default;
    ServiceIdentifierType& operator=(ServiceIdentifierType&& other) = default;

    
    /* Block suppress all comparison operators below: */

    /**
     * \brief Compares two instances for equality
     *
     * \param lhs The first instance to check for equality
     * \param rhs The second instance to check for equality
     * \return true if lhs and rhs equal, false otherwise
     */
    friend bool operator==(const ServiceIdentifierType& lhs, const ServiceIdentifierType& rhs) noexcept;

    /**
     * \brief LessThanComparable operator
     *
     * \param lhs The first ServiceIdentifierType instance to compare
     * \param rhs The second ServiceIdentifierType instance to compare
     * \return true if lhs is less then rhs, false otherwise
     */
    friend bool operator<(const ServiceIdentifierType& lhs, const ServiceIdentifierType& rhs) noexcept;

    

    /**
     * \brief Returns a non-owning string representation of the service type name
     */
    amp::string_view ToString() const noexcept;

    /**
     * \brief Returns a non-owning string representation of the serialized internals of this class to be used for
     * hashing.
     */
    amp::string_view ToHashString() const noexcept { return serialized_string_; }

    bmw::json::Object Serialize() const noexcept;

  private:
    /**
     * @brief this is the FQN of the AUTOSAR service interface (AUTOSAR short-name-path)
     */
    std::string serviceTypeName_;

    ServiceVersionType version_;

    std::string serialized_string_;

    constexpr static std::uint32_t serializationVersion = 1U;

    

    // User should not be able to create ServiceIdentifierType it is meant only for internal use
    // -> declared as private access with friend class access.
    // Additionally, there is an AoU stating that Impl Namespace should not be used [No APIs from Implementation
    // Namespace]
    // 
    friend ServiceIdentifierType make_ServiceIdentifierType(std::string serviceTypeName,
                                                            const std::uint32_t major_version_number,
                                                            const std::uint32_t minor_version_number);
    
    ServiceIdentifierType(std::string serviceTypeName,
                          const std::uint32_t major_version_number,
                          const std::uint32_t minor_version_number);

    // User should not be able to create ServiceIdentifierType it is meant only for internal use
    // -> declared as private access with friend class access
    // Additionally, there is an AoU stating that Impl Namespace should not be used [No APIs from Implementation
    // Namespace]
    // 
    friend ServiceIdentifierTypeView;

    ServiceIdentifierType(std::string serviceTypeName, ServiceVersionType version);
};

/**
 * \brief A make_ function is introduced to hide the Constructor of ServiceIdentifierType.
 * The ServiceIdentifierType will be exposed to the API user and by not having a public constructor
 * we can avoid that by chance the user will construct this class. Introducing a custom make method
 * that is _not_ mentioned in the standard, will avoid this!
 *
 * \param id The service identification number
 * \return A constructed ServiceIdentifierType from the provided numbers
 */
inline ServiceIdentifierType make_ServiceIdentifierType(std::string serviceTypeName,
                                                        const std::uint32_t major_version_number = std::size_t{1U},
                                                        const std::uint32_t minor_version_number = std::size_t{0U})
{
    return ServiceIdentifierType{std::move(serviceTypeName), major_version_number, minor_version_number};
}

/**
 * \brief The ServiceIdentifierType API is described by the ara::com standard.
 * But we also need to use it for internal purposes, where we need access to internal impl. details,
 * that is not exposed by the public API described in the adaptive AUTOSAR Standard.
 * In order to not leak implementation details, we come up with a `View` onto the ServiceIdentifierType.
 * Since our view is anyhow _only_ located in the `impl` namespace, there is zero probability that
 * any well minded user would depend on it.
 */
class ServiceIdentifierTypeView
{
  public:
    constexpr explicit ServiceIdentifierTypeView(const ServiceIdentifierType& type) : srvIdentifierType_(type){};
    inline amp::string_view getInternalTypeName() const { return srvIdentifierType_.serviceTypeName_; };

    inline ServiceVersionType GetVersion() const { return srvIdentifierType_.version_; }

    constexpr static inline std::uint32_t GetSerializationVersion()
    {
        return ServiceIdentifierType::serializationVersion;
    }

  private:
    const ServiceIdentifierType& srvIdentifierType_;
};

}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

namespace std
{
template <>
// NOLINTBEGIN(bmw-struct-usage-compliance): STL defines a struct, not a class
// coverity [autosar_cpp14_a11_0_2_violation]
struct hash<bmw::mw::com::impl::ServiceIdentifierType>
// NOLINTEND(bmw-struct-usage-compliance): see above
{
    size_t operator()(const bmw::mw::com::impl::ServiceIdentifierType& value) const noexcept
    {
        return std::hash<amp::string_view>{}(value.ToHashString());
    }
};
}  // namespace std

#endif  // PLATFORM_AAS_MW_COM_IMPL_CONFIGURATION_SERVICE_IDENTIFIER_TYPE_H
