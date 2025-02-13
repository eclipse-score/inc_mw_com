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


#ifndef PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_PROXY_H
#define PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_PROXY_H

#include "platform/aas/mw/com/impl/bindings/lola/element_fq_id.h"
#include "platform/aas/mw/com/impl/bindings/lola/event_control.h"
#include "platform/aas/mw/com/impl/bindings/lola/event_meta_info.h"
#include "platform/aas/mw/com/impl/handle_type.h"
#include "platform/aas/mw/com/impl/instance_identifier.h"
#include "platform/aas/mw/com/impl/proxy_binding.h"
#include "platform/aas/mw/com/impl/proxy_event_binding_base.h"

#include "platform/aas/lib/filesystem/filesystem.h"
#include "platform/aas/lib/memory/shared/flock/flock_mutex_and_lock.h"
#include "platform/aas/lib/memory/shared/flock/shared_flock_mutex.h"
#include "platform/aas/lib/memory/shared/lock_file.h"
#include "platform/aas/lib/memory/shared/managed_memory_resource.h"
#include "platform/aas/lib/os/glob.h"
#include "platform/aas/lib/result/result.h"

#include <amp_string_view.hpp>

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace bmw
{
namespace mw
{
namespace com
{
namespace impl
{
namespace lola
{

class IShmPathBuilder;
class FindServiceGuard;

/// \brief Proxy binding implementation for all Lola proxies.
class Proxy : public ProxyBinding
{
    friend class ProxyTestAttorney;

  public:
    /// \brief Class to convert an event name to an event fq id given the information already known to a Proxy
    ///
    /// We create a separate class to encapsulate the data that is only required for the conversion within this class.
    class EventNameToElementFqIdConverter
    {
      public:
        EventNameToElementFqIdConverter(const LolaServiceTypeDeployment& lola_service_type_deployment,
                                        LolaServiceInstanceId::InstanceId instance_id) noexcept
            : service_id_{lola_service_type_deployment.service_id_},
              events_{lola_service_type_deployment.events_},
              instance_id_{instance_id}
        {
        }

        ElementFqId Convert(const amp::string_view event_name) const noexcept;

      private:
        const std::uint16_t service_id_;
        const std::reference_wrapper<const LolaServiceTypeDeployment::EventIdMapping> events_;
        LolaServiceInstanceId::InstanceId instance_id_;
    };

    Proxy(const Proxy&) noexcept = delete;
    Proxy& operator=(const Proxy&) noexcept = delete;
    Proxy(Proxy&&) noexcept = delete;
    Proxy& operator=(Proxy&&) noexcept = delete;

    ~Proxy() override;

    static std::unique_ptr<Proxy> Create(const HandleType handle) noexcept;

    Proxy(std::shared_ptr<memory::shared::ManagedMemoryResource> control,
          std::shared_ptr<memory::shared::ManagedMemoryResource> data,
          const QualityType quality_type,
          EventNameToElementFqIdConverter event_name_to_element_fq_id_converter,
          HandleType handle,
          amp::optional<memory::shared::LockFile> service_instance_usage_marker_file,
          std::unique_ptr<bmw::memory::shared::FlockMutexAndLock<bmw::memory::shared::SharedFlockMutex>>
              service_instance_usage_flock_mutex_and_lock) noexcept;

    /// Returns the address of the control structure, for the given event ID.
    ///
    /// Terminates if the event control structure cannot be found.
    ///
    /// \param element_fq_id The Event ID.
    /// \return A reference to the event control structure.
    EventControl& GetEventControl(const ElementFqId element_fq_id) noexcept;

    /// Retrieves a raw pointer to the event data storage area.
    ///
    /// The pointer that is returned points to an EventDataStorage of a certain type. The type is identified later when
    /// samples are retrieved, see GetNewSamples for an explanation.
    ///
    /// \param element_fq_id The Event ID.
    /// \return A pointer to the data or nullptr if the event couldn't be found.
    const void* GetRawDataStorage(const ElementFqId element_fq_id) const noexcept;

    /// Retrieves an event data meta info.
    ///
    /// The event meta info can be used to iterate over events in the event data storage when the type is not known e.g.
    /// when dealing with a GenericProxyEvent. Terminates if the event meta info cannot be found.
    ///
    /// \param element_fq_id The Event ID.
    /// \return An event data meta info.
    EventMetaInfo GetEventMetaInfo(const ElementFqId element_fq_id) const noexcept;

    /// Checks whether the event corresponding to event_name is provided
    ///
    /// It does this by checking whether the event corresponding to event_name exists in shared memory.
    /// \param event_name The event name to check.
    /// \return True if the event name exists, otherwise, false
    bool IsEventProvided(const amp::string_view event_name) const noexcept override;

    /// \brief Adds a reference to a Proxy service element binding to an internal map
    ///
    /// Will insert the provided ProxyEventBindingBase& into a map stored within the class which will be used to call
    /// NotifyServiceInstanceChangedAvailability on all saved Proxy service elements by the FindServiceHandler of
    /// find_service_guard_. It will then call NotifyServiceInstanceChangedAvailability on the provided
    /// ProxyEventBindingBase. Since this function first locks proxy_event_registration_mutex_, it is ensured that the
    /// provided Proxy service element will be notified synchronously about the availability of the provider and will
    /// then be notified of any future changes via the callback, without missing any notifications.
    void RegisterEventBinding(const amp::string_view service_element_name,
                              ProxyEventBindingBase& proxy_event_binding) noexcept override;

    /// \brief Removes the reference to a Proxy service element binding from an internal map
    ///
    /// This must be called by a Proxy service element before destructing to ensure that the FindService handler in
    /// find_service_guard_ does not call NotifyServiceInstanceChangedAvailability on a Proxy service element after it's
    /// been destructed.
    void UnregisterEventBinding(const amp::string_view service_element_name) noexcept override;

    QualityType GetQualityType() const noexcept;

    /// \brief Returns pid of provider/skeleton side, this proxy is "connected" with.
    /// \return
    pid_t GetSourcePid() const noexcept;

    const InstanceSpecifier& GetInstanceSpecifier() const noexcept;

  private:
    void ServiceAvailabilityChangeHandler(const bool is_service_available) noexcept;

    std::shared_ptr<memory::shared::ManagedMemoryResource> control_;
    std::shared_ptr<memory::shared::ManagedMemoryResource> data_;

    QualityType quality_type_;
    EventNameToElementFqIdConverter event_name_to_element_fq_id_converter_;
    HandleType handle_;
    std::unordered_map<amp::string_view, std::reference_wrapper<ProxyEventBindingBase>> event_bindings_;

    /// Mutex which synchronises registration of Proxy service elements via Proxy::RegisterEventBinding with the
    /// FindServiceHandler in find_service_guard_ which will call NotifyServiceInstanceChangedAvailability on all
    /// currently registered Proxy service elements.
    std::mutex proxy_event_registration_mutex_;
    bool is_service_instance_available_;
    std::unique_ptr<FindServiceGuard> find_service_guard_;
    amp::optional<memory::shared::LockFile> service_instance_usage_marker_file_;
    std::unique_ptr<bmw::memory::shared::FlockMutexAndLock<bmw::memory::shared::SharedFlockMutex>>
        service_instance_usage_flock_mutex_and_lock_;
};

}  // namespace lola
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_PROXY_H
