// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#ifndef PLATFORM_AAS_MW_COM_IMPL_INSTANCEIDENTIFIER_H
#define PLATFORM_AAS_MW_COM_IMPL_INSTANCEIDENTIFIER_H

#include "platform/aas/mw/com/impl/com_error.h"
#include "platform/aas/mw/com/impl/configuration/configuration.h"
#include "platform/aas/mw/com/impl/configuration/service_identifier_type.h"
#include "platform/aas/mw/com/impl/configuration/service_instance_deployment.h"
#include "platform/aas/mw/com/impl/configuration/service_instance_id.h"
#include "platform/aas/mw/com/impl/configuration/service_type_deployment.h"
#include "platform/aas/mw/com/impl/configuration/service_version_type.h"

#include <amp_string_view.hpp>

#include <functional>
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

/**
 * \brief Represents a specific instance of a given service
 *
 * \requirement 
 */
class InstanceIdentifier final
{
  public:
    /**
     * \brief Exception-less constructor to create InstanceIdentifier from a serialized InstanceIdentifier created with
     * InstanceIdentifier::ToString()
     */
    static bmw::Result<InstanceIdentifier> Create(amp::string_view serialized_format) noexcept;

    InstanceIdentifier() = delete;
    ~InstanceIdentifier() noexcept = default;

    /**
     * ctor from serialized representation.
     *
     * Constructor is required by adaptive AUTOSAR Standard. But it uses exceptions, thus we will not implement it.
     *
     * explicit InstanceIdentifier(amp::string_view value);
     */

    /**
     * \brief CopyAssignment for InstanceIdentifier
     *
     * \post *this == other
     * \param other The InstanceIdentifier *this shall be constructed from
     * \return The InstanceIdentifier that was constructed
     */
    InstanceIdentifier& operator=(const InstanceIdentifier& other) = default;
    InstanceIdentifier(const InstanceIdentifier&) = default;
    InstanceIdentifier(InstanceIdentifier&&) noexcept = default;
    InstanceIdentifier& operator=(InstanceIdentifier&& other) = default;

    /**
     * \brief Returns the serialized form of the unknown internals of this class as a meaningful string
     *
     * \return A non-owning string representation of the internals of this class
     */
    amp::string_view ToString() const noexcept;

    /**
     * \brief Compares two instances for equality
     *
     * \param lhs The first instance to check for equality
     * \param rhs The second instance to check for equality
     * \return true if other and *this equal, false otherwise
     */
    friend bool operator==(const InstanceIdentifier& lhs, const InstanceIdentifier& rhs) noexcept;

    /**
     * \brief LessThanComparable operator
     *
     * \param lhs The first InstanceIdentifier instance to compare
     * \param rhs The second InstanceIdentifier instance to compare
     * \return true if *this is less then other, false otherwise
     */
    friend bool operator<(const InstanceIdentifier& lhs, const InstanceIdentifier& rhs) noexcept;

  private:
    const ServiceInstanceDeployment* instance_deployment_;
    const ServiceTypeDeployment* type_deployment_;

    /**
     * @brief Internal constructor to construct an InstanceIdentifier from a json-serialized InstanceIdentifier
     *
     * @param json_object Used to construct the InstanceIdentifier (no copies of json_object are made internally).
     * @param serialized_string Serialized string which the json_object is derived from. Used to set serialized_string_.
     */
    explicit InstanceIdentifier(const json::Object& json_object, std::string serialized_string) noexcept;

    /**
     * @brief internal impl. specific ctor.
     *
     * @param service identification of service
     * @param version version info
     * @param deployment deployment info
     */
    explicit InstanceIdentifier(const ServiceInstanceDeployment&, const ServiceTypeDeployment&) noexcept;

    static void SetConfiguration(Configuration* const configuration) noexcept
    {
        InstanceIdentifier::configuration_ = configuration;
    }

    json::Object Serialize() const noexcept;

    /**
     * @brief serialized format of this InstanceIdentifier instance
     */
    std::string serialized_string_;

    /**
     * @brief serialization format version.
     *
     * Whenever the state/content of this class changes in a way, which has effect on serialization, this version
     * has to be incremented! We potentially transfer instances of this class in a serialized form between processes
     * and need to know in the receiver process, if this serialized instance can be understood.
     */
    constexpr static std::uint32_t serializationVersion{1U};

    /**
     * \brief Global configuration object which is parsed from a json file and loaded by the runtime
     *
     * Whenever an InstanceIdentifier is created from another serialized InstanceIdentifier, the ServiceTypeDeployment /
     * ServiceInstanceDeployment held by the serialized InstanceIdentifier needs to be added to the maps within the
     * global configuration object. The newly created InstanceIdentifier will then store pointers to these structs.
     */
    static Configuration* configuration_;

    friend InstanceIdentifier make_InstanceIdentifier(const ServiceInstanceDeployment& instance_deployment,
                                                      const ServiceTypeDeployment& type_deployment) noexcept;

    friend class InstanceIdentifierView;

    friend class InstanceIdentifierAttorney;

    friend class Runtime;
};

/**
 * \brief A make_ function is introduced to hide the Constructor of InstanceIdentifier.
 * The InstanceIdentifier will be exposed to the API user and by not having a public constructor
 * we can avoid that by chance the user will construct this class. Introducing a custom make method
 * that is _not_ mentioned in the standard, will avoid this!
 *
 * \param service The service this instance is revering to
 * \param version The version of the service this instances revers to
 * \param deployment The deployment specific information for this instance
 * \return A constructed InstanceIdentifier
 */
inline InstanceIdentifier make_InstanceIdentifier(const ServiceInstanceDeployment& instance_deployment,
                                                  const ServiceTypeDeployment& type_deployment) noexcept
{
    return InstanceIdentifier{instance_deployment, type_deployment};
}

/**
 * \brief The bmw::mw::com::InstanceIdentifiers API is described by the ara::com standard.
 * But we also need to use it for internal purposes, why we need to access some state information
 * that is not exposed by the public API described in the adaptive AUTOSAR Standard.
 * In order to not leak implementation details, we come up with a `View` onto the InstanceIdentifier.
 * Since our view is anyhow _only_ located in the `impl` namespace, there is zero probability that
 * any well minded user would depend on it.
 */
class InstanceIdentifierView final
{
  public:
    explicit InstanceIdentifierView(const InstanceIdentifier&);
    ~InstanceIdentifierView() noexcept = default;

    json::Object Serialize() const noexcept { return identifier_.Serialize(); };

    amp::optional<ServiceInstanceId> GetServiceInstanceId() const noexcept;
    const ServiceInstanceDeployment& GetServiceInstanceDeployment() const;
    const ServiceTypeDeployment& GetServiceTypeDeployment() const;
    bool isCompatibleWith(const InstanceIdentifier&) const;
    bool isCompatibleWith(const InstanceIdentifierView&) const;
    constexpr static std::uint32_t GetSerializationVersion() { return InstanceIdentifier::serializationVersion; };

  private:
    const InstanceIdentifier& identifier_;
};

}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

template <>
class std::hash<bmw::mw::com::impl::InstanceIdentifier>
{
  public:
    std::size_t operator()(const bmw::mw::com::impl::InstanceIdentifier&) const noexcept;
};

#endif  // PLATFORM_AAS_MW_COM_IMPL_INSTANCEIDENTIFIER_H
