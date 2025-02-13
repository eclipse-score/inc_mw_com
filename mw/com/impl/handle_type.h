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


#ifndef PLATFORM_AAS_MW_COM_IMPL_HANDLETYPE_H
#define PLATFORM_AAS_MW_COM_IMPL_HANDLETYPE_H

#include "platform/aas/mw/com/impl/configuration/service_instance_id.h"
#include "platform/aas/mw/com/impl/configuration/service_type_deployment.h"
#include "platform/aas/mw/com/impl/instance_identifier.h"

#include <amp_optional.hpp>
#include <amp_string_view.hpp>

#include <cstring>

namespace bmw
{
namespace mw
{
namespace com
{
namespace impl
{

class HandleType;

/**
 * \brief A make_ function is introduced to hide the Constructor of HandleType.
 * The HandleType will be exposed to the API user and by not having a public constructor
 * we can avoid that by chance the user will construct this class. Introducing a custom make method
 * that is _not_ mentioned in the standard, will avoid this!
 *
 * \param identifier The InstanceIdentifier that this handle wraps
 * \param instance_id An optional instance ID that can be passed during a find-all search by FindService. If provided,
 * this value will be used instead of the value in the configuration, referenced from identifier.
 * \return A constructed InstanceIdentifier
 */
HandleType make_HandleType(InstanceIdentifier identifier, amp::optional<ServiceInstanceId> instance_id = {}) noexcept;

/**
 * \brief It types the handle for a specific service
 * instance and shall contain the information that is needed to create a ServiceProxy
 *
 * \requirement 
 */
class HandleType
{
  public:
    HandleType() = delete;
    ~HandleType() = default;
    HandleType(const HandleType&) = default;
    HandleType(HandleType&&) noexcept = default;
    HandleType& operator=(const HandleType&) = default;
    HandleType& operator=(HandleType&&) noexcept = default;

    /**
     * \brief Compares two instances for equality
     *
     * \param lhs The first instance to check for equality
     * \param rhs The second instance to check for equality
     * \return true if lhs and rhs equal, false otherwise
     */
    friend bool operator==(const HandleType& lhs, const HandleType& rhs) noexcept;

    /**
     * \brief LessThanComparable operator
     *
     * \param lhs The first HandleType instance to compare
     * \param rhs The second HandleType instance to compare
     * \return true if lhs is less than rhs, false otherwise
     */
    friend bool operator<(const HandleType& lhs, const HandleType& rhs) noexcept;

    /**
     * \brief Query the associated instance
     *
     * \return The InstanceIdentifier that is associated with this handle
     */
    const InstanceIdentifier& GetInstanceIdentifier() const noexcept;

    /// Extracts the deployment information from the handle.
    ///
    /// \return A reference to the included deployment information.
    const ServiceInstanceDeployment& GetDeploymentInformation() const noexcept;

    /**
     * \brief Query the associated instance id
     *
     * \return instance id associated with this handle. If a ServiceInstanceId was provided in the constructor (in the
     * case of find any semantics) it will be returned here. Otherwise, the instance id from the configuration will be
     * returned.
     */
    ServiceInstanceId GetInstanceId() const noexcept { return instance_id_; }

  private:
    InstanceIdentifier identifier_;
    ServiceInstanceId instance_id_;

    explicit HandleType(InstanceIdentifier, amp::optional<ServiceInstanceId> instance_id) noexcept;

    friend HandleType make_HandleType(InstanceIdentifier, amp::optional<ServiceInstanceId> instance_id) noexcept;
};

}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

namespace std
{
template <>
// NOLINTNEXTLINE(bmw-struct-usage-compliance): STL defines hash as struct.
struct hash<bmw::mw::com::impl::HandleType>
{
    std::size_t operator()(const bmw::mw::com::impl::HandleType& handle_type) const noexcept
    {
        // To prevent dynamic memory allocations, we copy the input hash strings into a local buffer and create a string
        // view to the local buffer. We then calculate the hash of the string_view.
        constexpr std::size_t service_type_deployment_hash_string_size{
            bmw::mw::com::impl::ServiceTypeDeployment::hashStringSize};
        constexpr std::size_t service_instance_id_hash_string_size{
            bmw::mw::com::impl::ServiceInstanceId::hashStringSize};
        constexpr std::size_t hash_string_size{service_type_deployment_hash_string_size +
                                               service_instance_id_hash_string_size};

        const auto& instance_identifier_view =
            bmw::mw::com::impl::InstanceIdentifierView{handle_type.GetInstanceIdentifier()};
        const auto service_type_deployment = instance_identifier_view.GetServiceTypeDeployment();
        const auto service_instance_id = handle_type.GetInstanceId();

        const auto instance_id_hash_string = service_instance_id.ToHashString();
        const auto service_type_deployment_hash_string = service_type_deployment.ToHashString();

        std::array<char, hash_string_size> local_buffer{};

        const std::size_t total_string_size{instance_id_hash_string.size() +
                                            service_type_deployment_hash_string.size()};

        AMP_ASSERT_PRD_MESSAGE(
            total_string_size <= hash_string_size,
            "InstansceIdentifier and/or ServiceTypeDeployment obtained from handle_type produce a hash "
            "string that is too long. These strings together do not fit in the buffer.");

        const auto second_copy_start_pos =
            std::copy(instance_id_hash_string.begin(), instance_id_hash_string.end(), local_buffer.begin());
        std::copy(service_type_deployment_hash_string.begin(),
                  service_type_deployment_hash_string.end(),
                  second_copy_start_pos);

        const amp::string_view local_string_buffer{local_buffer.data(), total_string_size};
        return hash<amp::string_view>{}(local_string_buffer);
    }
};

}  // namespace std

#endif  // PLATFORM_AAS_MW_COM_IMPL_HANDLETYPE_H
