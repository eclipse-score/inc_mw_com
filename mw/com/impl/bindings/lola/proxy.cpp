// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#include "platform/aas/mw/com/impl/bindings/lola/proxy.h"

#include "platform/aas/mw/com/impl/bindings/lola/element_fq_id.h"
#include "platform/aas/mw/com/impl/bindings/lola/i_runtime.h"
#include "platform/aas/mw/com/impl/bindings/lola/i_shm_path_builder.h"
#include "platform/aas/mw/com/impl/bindings/lola/partial_restart_path_builder.h"
#include "platform/aas/mw/com/impl/bindings/lola/service_data_control.h"
#include "platform/aas/mw/com/impl/bindings/lola/service_data_storage.h"
#include "platform/aas/mw/com/impl/bindings/lola/shm_path_builder.h"
#include "platform/aas/mw/com/impl/bindings/lola/transaction_log_rollback_executor.h"
#include "platform/aas/mw/com/impl/configuration/lola_event_instance_deployment.h"
#include "platform/aas/mw/com/impl/configuration/lola_service_instance_id.h"
#include "platform/aas/mw/com/impl/configuration/lola_service_type_deployment.h"
#include "platform/aas/mw/com/impl/configuration/quality_type.h"
#include "platform/aas/mw/com/impl/find_service_handle.h"
#include "platform/aas/mw/com/impl/handle_type.h"
#include "platform/aas/mw/com/impl/runtime.h"

#include "platform/aas/lib/filesystem/filesystem.h"
#include "platform/aas/lib/memory/shared/flock/flock_mutex_and_lock.h"
#include "platform/aas/lib/memory/shared/flock/shared_flock_mutex.h"
#include "platform/aas/lib/memory/shared/lock_file.h"
#include "platform/aas/lib/memory/shared/shared_memory_factory.h"
#include "platform/aas/lib/os/errno_logging.h"
#include "platform/aas/lib/os/fcntl.h"
#include "platform/aas/lib/os/glob.h"

#include "platform/aas/mw/log/logging.h"

#include <amp_assert.hpp>
#include <amp_utility.hpp>
#include <amp_variant.hpp>

#include <cstring>
#include <exception>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
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

namespace
{

const LolaServiceInstanceDeployment* GetLoLaInstanceDeployment(const bmw::mw::com::impl::HandleType& handle) noexcept
{
    const auto* const instance_deployment =
        amp::get_if<LolaServiceInstanceDeployment>(&handle.GetDeploymentInformation().bindingInfo_);
    AMP_PRECONDITION_PRD_MESSAGE(instance_deployment != nullptr,
                                 "Could not create Proxy: lola service instance deployment does not exist.");
    return instance_deployment;
}

const LolaServiceTypeDeployment* GetLoLaServiceTypeDeployment(const bmw::mw::com::impl::HandleType& handle) noexcept
{
    const auto* lola_service_deployment = amp::get_if<LolaServiceTypeDeployment>(
        &InstanceIdentifierView{handle.GetInstanceIdentifier()}.GetServiceTypeDeployment().binding_info_);
    return lola_service_deployment;
}

std::pair<std::shared_ptr<memory::shared::ManagedMemoryResource>,
          std::shared_ptr<memory::shared::ManagedMemoryResource>>
CreateSharedMemory(const LolaServiceInstanceDeployment* instance_deployment,
                   QualityType quality_type,
                   const LolaServiceTypeDeployment* lola_service_deployment,
                   const LolaServiceInstanceId* lola_service_instance_id) noexcept
{
    std::optional<amp::span<const uid_t>> providers{std::nullopt};
    const auto found = instance_deployment->allowed_provider_.find(quality_type);
    if (found != instance_deployment->allowed_provider_.end())
    {
        providers = std::make_optional(amp::span<const uid_t>{found->second});
    }

    ShmPathBuilder shm_path_builder{lola_service_deployment->service_id_};
    const auto control_shm = shm_path_builder.GetControlChannelShmName(lola_service_instance_id->id_, quality_type);
    const auto data_shm = shm_path_builder.GetDataChannelShmName(lola_service_instance_id->id_);

    const std::shared_ptr<memory::shared::ManagedMemoryResource> control =
        bmw::memory::shared::SharedMemoryFactory::Open(control_shm, true, providers);
    const std::shared_ptr<memory::shared::ManagedMemoryResource> data =
        bmw::memory::shared::SharedMemoryFactory::Open(data_shm, false, providers);
    if ((control == nullptr) || (data == nullptr))
    {
        bmw::mw::log::LogError("lola") << "Could not create Proxy: Opening shared memory failed.";
        return std::make_pair(nullptr, nullptr);
    }

    return std::make_pair(control, data);
}

ServiceDataControl& GetServiceDataControl(
    const std::shared_ptr<memory::shared::ManagedMemoryResource>& control) noexcept
{
    auto* const service_data_control = static_cast<ServiceDataControl*>(control->getUsableBaseAddress());
    AMP_ASSERT_PRD_MESSAGE(service_data_control != nullptr, "Could not retrieve service data control.");
    return *service_data_control;
}

ServiceDataStorage& GetServiceDataStorage(const std::shared_ptr<memory::shared::ManagedMemoryResource>& data) noexcept
{
    auto* const service_data_storage = static_cast<ServiceDataStorage*>(data->getUsableBaseAddress());
    AMP_ASSERT_PRD_MESSAGE(service_data_storage != nullptr,
                           "Could not retrieve service data storage within shared-memory.");
    return *service_data_storage;
}

bmw::ResultBlank ExecutePartialRestartLogic(QualityType quality_type,
                                            const std::shared_ptr<memory::shared::ManagedMemoryResource>& control,
                                            const std::shared_ptr<memory::shared::ManagedMemoryResource>& data) noexcept
{
    auto& service_data_storage = GetServiceDataStorage(data);

    auto* const lola_runtime =
        dynamic_cast<lola::IRuntime*>(mw::com::impl::Runtime::getInstance().GetBindingRuntime(BindingType::kLoLa));
    AMP_ASSERT_PRD_MESSAGE(lola_runtime, "No LoLa Runtime available although we are creating a LoLa proxy!");

    const TransactionLogId transaction_log_id{lola_runtime->GetUid()};
    auto& service_data_control = GetServiceDataControl(control);
    TransactionLogRollbackExecutor transaction_log_rollback_executor{
        &service_data_control, quality_type, service_data_storage.skeleton_pid_, transaction_log_id};
    const auto rollback_result = transaction_log_rollback_executor.RollbackTransactionLogs();
    if (!rollback_result.has_value())
    {
        bmw::mw::log::LogError("lola") << "Could not create Proxy: Rolling back transaction log failed.";
        return MakeUnexpected(ComErrc::kBindingFailure, "Could not create Proxy: Rolling back transaction log failed.");
    }

    const auto previous_pid =
        service_data_control.uid_pid_mapping_.RegisterPid(lola_runtime->GetUid(), lola_runtime->GetPid());
    AMP_ASSERT_PRD_MESSAGE(previous_pid.has_value(), "Couldn't register current pid/uid into service_data_control");
    const bool our_pid_has_changed = previous_pid.value() != lola_runtime->GetPid();
    if (our_pid_has_changed)
    {
        lola_runtime->GetLolaMessaging().NotifyOutdatedNodeId(
            quality_type, previous_pid.value(), service_data_storage.skeleton_pid_);
    }
    return {};
}

EventControl& GetEventControlImpl(const std::shared_ptr<memory::shared::ManagedMemoryResource>& control,
                                  const ElementFqId element_fq_id) noexcept
{
    auto& service_data_control = GetServiceDataControl(control);
    const auto event_entry = service_data_control.event_controls_.find(element_fq_id);
    if (event_entry != service_data_control.event_controls_.end())
    {
        return event_entry->second;
    }
    else
    {
        bmw::mw::log::LogFatal("lola") << __func__ << __LINE__
                                       << "Unable to find control channel for given event instance. Terminating.";
        std::terminate();
    }
}

const void* GetRawDataStorageImpl(const std::shared_ptr<memory::shared::ManagedMemoryResource>& data,
                                  const ElementFqId element_fq_id) noexcept
{
    auto& service_data_storage = GetServiceDataStorage(data);
    const auto event_entry = service_data_storage.events_.find(element_fq_id);
    if (event_entry != service_data_storage.events_.end())
    {
        return event_entry->second.get();
    }
    else
    {
        bmw::mw::log::LogFatal("lola") << __func__ << __LINE__
                                       << "Unable to find data storage for given event instance. Terminating.";
        std::terminate();
    }
}

}  // namespace

class FindServiceGuard final
{
  public:
    FindServiceGuard(FindServiceHandler<HandleType> find_service_handler,
                     EnrichedInstanceIdentifier enriched_instance_identifier) noexcept
        : service_availability_change_handle_{nullptr}
    {
        auto& service_discovery = impl::Runtime::getInstance().GetServiceDiscovery();
        const auto find_service_handle_result = service_discovery.StartFindService(
            std::move(find_service_handler), std::move(enriched_instance_identifier));
        if (!find_service_handle_result.has_value())
        {
            bmw::mw::log::LogFatal("lola")
                << "StartFindService failed with error" << find_service_handle_result.error() << ". Terminating.";
            std::terminate();
        }
        service_availability_change_handle_ = std::make_unique<FindServiceHandle>(find_service_handle_result.value());
    }

    ~FindServiceGuard() noexcept
    {
        auto& service_discovery = impl::Runtime::getInstance().GetServiceDiscovery();
        const auto stop_find_service_result = service_discovery.StopFindService(*service_availability_change_handle_);
        if (!stop_find_service_result.has_value())
        {
            bmw::mw::log::LogError("lola")
                << "StopFindService failed with error" << stop_find_service_result.error() << ". Ignoring error.";
        }
    }

    FindServiceGuard(const FindServiceGuard&) = delete;
    FindServiceGuard& operator=(const FindServiceGuard&) = delete;
    FindServiceGuard(FindServiceGuard&& other) = delete;
    FindServiceGuard& operator=(FindServiceGuard&& other) = delete;

  private:
    std::unique_ptr<FindServiceHandle> service_availability_change_handle_;
};

ElementFqId Proxy::EventNameToElementFqIdConverter::Convert(const amp::string_view event_name) const noexcept
{
    const auto& events = events_.get();
    const auto event_it = events.find(std::string{event_name.data()});

    std::stringstream sstream{};
    sstream << "Event name " << event_name.data() << " does not exists in event map.";
    AMP_ASSERT_PRD_MESSAGE(event_it != events.end(), sstream.str().c_str());
    return {service_id_, event_it->second, instance_id_, ElementType::EVENT};
}

std::unique_ptr<Proxy> Proxy::Create(const HandleType handle) noexcept
{
    const auto* instance_deployment = GetLoLaInstanceDeployment(handle);

    const auto* const lola_service_deployment = GetLoLaServiceTypeDeployment(handle);

    if (lola_service_deployment == nullptr)
    {
        bmw::mw::log::LogError("lola") << "Could not create Proxy: lola service type deployment does not exist.";
        return nullptr;
    }

    const auto service_instance_id{handle.GetInstanceId()};
    const auto* const lola_service_instance_id = amp::get_if<LolaServiceInstanceId>(&service_instance_id.binding_info_);
    AMP_PRECONDITION_PRD_MESSAGE(lola_service_instance_id != nullptr,
                                 "Could not create Proxy: lola service instance id does not exist.");

    PartialRestartPathBuilder partial_restart_builder{lola_service_deployment->service_id_};
    const auto service_instance_usage_marker_file_path{
        partial_restart_builder.GetServiceInstanceUsageMarkerFilePath(lola_service_instance_id->id_)};

    auto service_instance_usage_marker_file = memory::shared::LockFile::Open(service_instance_usage_marker_file_path);
    if (!service_instance_usage_marker_file.has_value())
    {
        bmw::mw::log::LogError("lola") << "Could not open marker file: " << service_instance_usage_marker_file_path;
        return nullptr;
    }

    auto service_instance_usage_mutex_and_lock =
        std::make_unique<bmw::memory::shared::FlockMutexAndLock<bmw::memory::shared::SharedFlockMutex>>(
            service_instance_usage_marker_file.value());
    if (!service_instance_usage_mutex_and_lock->TryLock())
    {
        bmw::mw::log::LogError("lola")
            << "Flock try_lock failed: Skeleton could have already exclusively flocked the usage marker file: "
            << service_instance_usage_marker_file_path;
        return nullptr;
    }

    QualityType quality_type{handle.GetDeploymentInformation().asilLevel_};

    const auto shared_memory =
        CreateSharedMemory(instance_deployment, quality_type, lola_service_deployment, lola_service_instance_id);

    if ((shared_memory.first == nullptr) || (shared_memory.second == nullptr))
    {
        return nullptr;
    }
    const auto control = shared_memory.first;
    const auto data = shared_memory.second;

    const auto partial_restart_result = ExecutePartialRestartLogic(quality_type, control, data);

    if (!partial_restart_result.has_value())
    {
        return nullptr;
    }

    EventNameToElementFqIdConverter event_name_to_element_fq_id_converter{*lola_service_deployment,
                                                                          lola_service_instance_id->id_};
    return std::make_unique<Proxy>(control,
                                   data,
                                   quality_type,
                                   event_name_to_element_fq_id_converter,
                                   handle,
                                   std::move(service_instance_usage_marker_file),
                                   std::move(service_instance_usage_mutex_and_lock));
}

Proxy::Proxy(std::shared_ptr<memory::shared::ManagedMemoryResource> control,
             std::shared_ptr<memory::shared::ManagedMemoryResource> data,
             const QualityType quality_type,
             EventNameToElementFqIdConverter event_name_to_element_fq_id_converter,
             HandleType handle,
             amp::optional<memory::shared::LockFile> service_instance_usage_marker_file,
             std::unique_ptr<bmw::memory::shared::FlockMutexAndLock<bmw::memory::shared::SharedFlockMutex>>
                 service_instance_usage_flock_mutex_and_lock) noexcept
    : ProxyBinding{},
      control_{std::move(control)},
      data_{std::move(data)},
      quality_type_{quality_type},
      event_name_to_element_fq_id_converter_{std::move(event_name_to_element_fq_id_converter)},
      handle_{std::move(handle)},
      event_bindings_{},
      proxy_event_registration_mutex_{},
      is_service_instance_available_{false},
      find_service_guard_{std::make_unique<FindServiceGuard>(
          [this](ServiceHandleContainer<HandleType> service_handle_container, FindServiceHandle) noexcept {
              std::lock_guard lock{proxy_event_registration_mutex_};
              is_service_instance_available_ = !service_handle_container.empty();
              ServiceAvailabilityChangeHandler(is_service_instance_available_);
          },
          EnrichedInstanceIdentifier{handle_})},
      service_instance_usage_marker_file_{std::move(service_instance_usage_marker_file)},
      service_instance_usage_flock_mutex_and_lock_{std::move(service_instance_usage_flock_mutex_and_lock)}
{
}

Proxy::~Proxy() = default;

void Proxy::ServiceAvailabilityChangeHandler(const bool is_service_available) noexcept
{
    for (auto& event_binding : event_bindings_)
    {
        event_binding.second.get().NotifyServiceInstanceChangedAvailability(is_service_available, GetSourcePid());
    }
}

EventControl& Proxy::GetEventControl(const ElementFqId element_fq_id) noexcept
{
    return GetEventControlImpl(control_, element_fq_id);
}

const void* Proxy::GetRawDataStorage(const ElementFqId element_fq_id) const noexcept
{
    return GetRawDataStorageImpl(data_, element_fq_id);
}

EventMetaInfo Proxy::GetEventMetaInfo(const ElementFqId element_fq_id) const noexcept
{
    auto& service_data_storage = GetServiceDataStorage(data_);
    const auto event_meta_info_entry = service_data_storage.events_metainfo_.find(element_fq_id);
    if (event_meta_info_entry != service_data_storage.events_metainfo_.end())
    {
        return event_meta_info_entry->second;
    }
    else
    {
        bmw::mw::log::LogFatal("lola") << __func__ << __LINE__
                                       << "Unable to find meta info for given event instance. Terminating.";
        std::terminate();
    }
}

bool Proxy::IsEventProvided(const amp::string_view event_name) const noexcept
{
    auto& service_data_control = GetServiceDataControl(control_);
    const auto element_fq_id = event_name_to_element_fq_id_converter_.Convert(event_name);
    const auto event_entry = service_data_control.event_controls_.find(element_fq_id);
    const bool event_exists = (event_entry != service_data_control.event_controls_.end());
    return event_exists;
}

void Proxy::RegisterEventBinding(const amp::string_view service_element_name,
                                 ProxyEventBindingBase& proxy_event_binding) noexcept
{
    std::lock_guard lock{proxy_event_registration_mutex_};
    const auto insert_result = event_bindings_.emplace(service_element_name, proxy_event_binding);
    AMP_ASSERT_PRD_MESSAGE(insert_result.second, "Failed to insert proxy event binding into event binding map.");
    proxy_event_binding.NotifyServiceInstanceChangedAvailability(is_service_instance_available_, GetSourcePid());
}

void Proxy::UnregisterEventBinding(const amp::string_view service_element_name) noexcept
{
    std::lock_guard lock{proxy_event_registration_mutex_};
    const auto number_of_elements_removed = event_bindings_.erase(service_element_name);
    if (number_of_elements_removed == 0)
    {
        bmw::mw::log::LogWarn("lola") << "UnregisterEventBinding that was never registered. Ignoring.";
    }
}

QualityType Proxy::GetQualityType() const noexcept
{
    return quality_type_;
}

pid_t Proxy::GetSourcePid() const noexcept
{
    auto& service_data_storage = GetServiceDataStorage(data_);
    return service_data_storage.skeleton_pid_;
}

const InstanceSpecifier& Proxy::GetInstanceSpecifier() const noexcept
{
    return handle_.GetDeploymentInformation().instance_specifier_;
}

}  // namespace lola
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
