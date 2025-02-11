// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#include "platform/aas/mw/com/impl/bindings/lola/skeleton.h"

#include "platform/aas/mw/com/impl/bindings/lola/shm_path_builder.h"
#include "platform/aas/mw/com/impl/bindings/lola/tracing/tracing_runtime.h"
#include "platform/aas/mw/com/impl/configuration/lola_service_type_deployment.h"
#include "platform/aas/mw/com/impl/skeleton_event_binding.h"

#include "platform/aas/lib/memory/shared/flock/flock_mutex_and_lock.h"
#include "platform/aas/lib/memory/shared/new_delete_delegate_resource.h"
#include "platform/aas/lib/memory/shared/shared_memory_factory.h"
#include "platform/aas/lib/memory/shared/shared_memory_resource.h"
#include "platform/aas/lib/os/acl.h"
#include "platform/aas/lib/os/errno_logging.h"
#include "platform/aas/lib/os/fcntl.h"
#include "platform/aas/lib/os/stat.h"
#include "platform/aas/mw/log/logger.h"
#include "platform/aas/mw/log/logging.h"

#include <amp_assert.hpp>
#include <amp_overload.hpp>
#include <amp_variant.hpp>

#include <string>
#include <unordered_map>
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
constexpr std::size_t STL_CONTAINER_STORAGE_NEEDS = 1024U;
constexpr std::size_t STL_CONTAINER_ELEMENT_STORAGE_NEEDS = sizeof(void*);
const filesystem::Path TMP_DIR{"/tmp/mw_com_lola"};

/// \brief log with INFO level the ACL of the given SharedMemoryResource
/// \param shared_mem_res shared-memory resource
void LogAclOfShmObj(std::shared_ptr<bmw::memory::shared::ISharedMemoryResource> shared_mem_res)
{
    auto is_log_enabled =
        bmw::mw::log::CreateLogger(bmw::mw::log::GetDefaultContextId()).IsLogEnabled(bmw::mw::log::LogLevel::kInfo);

    if (is_log_enabled)
    {
        const std::string* shared_mem_path_ptr = shared_mem_res->getPath();
        if (shared_mem_path_ptr == nullptr)
        {
            bmw::mw::log::LogError("lola")
                << __func__ << __LINE__
                << "Path of SharedMemory object is not set. You are probably trying to get a path "
                   "from an anonymous SharedMemory object.";
            return;
        }
        const auto& shared_mem_fd = shared_mem_res->GetFileDescriptor();
        auto& acl = ::bmw::os::Acl::instance();
        const auto acl_result = acl.acl_get_fd(shared_mem_fd);
        if (!acl_result.has_value())
        {
            bmw::mw::log::LogInfo("lola")
                << __func__ << __LINE__ << "ACL of SharedMemory object: " << *shared_mem_path_ptr
                << " error in acl_get_fd(): " << acl_result.error();
        }
        else
        {
            ssize_t len{};
            const auto acl_text_result = acl.acl_to_text(acl_result.value(), &len);
            acl.acl_free(acl_result.value());
            if (!acl_text_result.has_value())
            {
                bmw::mw::log::LogInfo("lola")
                    << __func__ << __LINE__ << "ACL of SharedMemory object: " << *shared_mem_path_ptr
                    << " error in acl_to_text(): " << acl_text_result.error();
            }
            else
            {
                bmw::mw::log::LogInfo("lola")
                    << __func__ << __LINE__ << "ACL of SharedMemory object: " << *shared_mem_path_ptr
                    << " acl: " << std::string_view{acl_text_result.value(), static_cast<std::size_t>(len)};
                acl.acl_free(acl_text_result.value());
            }
        }
    }
}

const LolaServiceTypeDeployment& GetLolaServiceTypeDeployment(const InstanceIdentifier& identifier) noexcept
{
    const auto& service_type_depl_info = InstanceIdentifierView{identifier}.GetServiceTypeDeployment();
    AMP_ASSERT_PRD_MESSAGE(amp::holds_alternative<LolaServiceTypeDeployment>(service_type_depl_info.binding_info_),
                           "Wrong Binding! ServiceTypeDeployment doesn't contain a LoLa deployment!");
    return amp::get<LolaServiceTypeDeployment>(service_type_depl_info.binding_info_);
}

const LolaServiceInstanceDeployment& GetLolaServiceInstanceDeployment(const InstanceIdentifier& identifier) noexcept
{
    const auto& instance_depl_info = InstanceIdentifierView{identifier}.GetServiceInstanceDeployment();
    AMP_ASSERT_PRD_MESSAGE(amp::holds_alternative<LolaServiceInstanceDeployment>(instance_depl_info.bindingInfo_),
                           "Wrong Binding! ServiceInstanceDeployment doesn't contain a LoLa deployment!");
    return amp::get<LolaServiceInstanceDeployment>(instance_depl_info.bindingInfo_);
}

ServiceDataControl* GetServiceDataControl(
    const std::shared_ptr<memory::shared::ManagedMemoryResource>& control) noexcept
{
    auto* const service_data_control = static_cast<ServiceDataControl*>(control->getUsableBaseAddress());
    AMP_ASSERT_PRD_MESSAGE(service_data_control != nullptr, "Could not retrieve service data control.");
    return service_data_control;
}

ServiceDataStorage* GetServiceDataStorage(const std::shared_ptr<memory::shared::ManagedMemoryResource>& data) noexcept
{
    auto* const service_data_storage = static_cast<ServiceDataStorage*>(data->getUsableBaseAddress());
    AMP_ASSERT_PRD_MESSAGE(service_data_storage != nullptr,
                           "Could not retrieve service data storage within shared-memory.");
    return service_data_storage;
}

/// \brief Get LoLa runtime needed to look up global LoLa specific configuration settings
/// \return instance of LoLa runtime
lola::IRuntime& GetLoLaRuntime()
{
    auto* const lola_runtime =
        dynamic_cast<lola::IRuntime*>(impl::Runtime::getInstance().GetBindingRuntime(BindingType::kLoLa));
    if (lola_runtime == nullptr)
    {
        ::bmw::mw::log::LogFatal("lola") << "Skeleton: No lola runtime available.";
        std::terminate();
    }
    return *lola_runtime;
}

enum class ShmObjectType : std::uint8_t
{
    kControl_QM = 0x00,
    kControl_ASIL_B = 0x01,
    kData = 0x02,
};

std::uint64_t CalculateMemoryResourceId(const LolaServiceTypeDeployment& service_type_deployment,
                                        const LolaServiceInstanceDeployment& service_instance_deployment,
                                        const ShmObjectType object_type) noexcept
{
    return ((static_cast<std::uint64_t>(service_type_deployment.service_id_) << 24U) +
            (static_cast<std::uint64_t>(service_instance_deployment.instance_id_.value().id_) << 8U) +
            static_cast<std::uint8_t>(object_type));
}

/// \brief Calculates (estimates) size needed for shm-object for control.
/// \param instance_deployment deployment info needed for "max-samples" lookup
/// \param events events the skeleton provides
/// \return estimated size in bytes
std::size_t EstimateControlShmResourceSize(const LolaServiceInstanceDeployment& instance_deployment,
                                           const SkeletonBinding::SkeletonEventBindings& events,
                                           const SkeletonBinding::SkeletonFieldBindings& fields) noexcept
{
    // Strategy to calculate the upper bound size needs of the data structures, we are going to place into ShmResource:
    // We add size needs of the "management space" the SharedMemoryResource needs itself and then the size of the
    // root data type, we place into the memory resource.
    // For every potentially allocating STL-container embedded within the root data type, we:
    // - add some placeholder STL_CONTAINER_STORAGE_NEEDS to compensate for "pre-allocation" the container impl. may do
    // - for each element within such a container, we add its size and (in case it is a map) some potential overhead in
    //   form of STL_CONTAINER_ELEMENT_STORAGE_NEEDS.
    std::size_t control_resource_size{};
    control_resource_size += sizeof(ServiceDataControl);
    control_resource_size += STL_CONTAINER_STORAGE_NEEDS;

    // ServiceDataControl contains an UidPidMapping, which again contains a DynamicArray with kMaxUidPidMappings
    // elements of MappingEntries
    control_resource_size += ServiceDataControl::kMaxUidPidMappings * sizeof(UidPidMappingEntry);

    // For the moment, fields are equivalent to events in terms of shared memory footprint. Therefore, we can use the
    // same calculation to estimate the element size of an event or field.
    constexpr auto CalculateServiceElementSize = [](const std::size_t max_samples) -> std::size_t {
        std::size_t map_element_size = sizeof(decltype(ServiceDataControl::event_controls_)::value_type);
        map_element_size += STL_CONTAINER_ELEMENT_STORAGE_NEEDS;
        // the mapped type again is a vector, so add STL_CONTAINER_STORAGE_NEEDS
        map_element_size += STL_CONTAINER_STORAGE_NEEDS;
        // and it contains max_samples_ control slots
        map_element_size += (max_samples * sizeof(EventDataControl::EventControlSlots::value_type));
        return map_element_size;
    };

    for (const auto& event : events)
    {
        const auto search = instance_deployment.events_.find(std::string(event.first.data(), event.first.size()));
        AMP_ASSERT_PRD_MESSAGE(search != instance_deployment.events_.cend(),
                               "Deployment doesn't contain event with given name!");
        const auto max_samples = static_cast<std::size_t>(search->second.GetNumberOfSampleSlots().value());
        control_resource_size += CalculateServiceElementSize(max_samples);
    }

    for (const auto& field : fields)
    {
        const auto search = instance_deployment.fields_.find(std::string(field.first.data(), field.first.size()));
        AMP_ASSERT_PRD_MESSAGE(search != instance_deployment.fields_.cend(),
                               "Deployment doesn't contain field with given name!");
        const auto max_samples = static_cast<std::size_t>(search->second.GetNumberOfSampleSlots().value());
        control_resource_size += CalculateServiceElementSize(static_cast<std::size_t>(max_samples));
    }
    return control_resource_size;
}

/// \brief Calculates (estimates) size needed for shm-object for data.
/// \param instance_deployment deployment info needed for "max-samples" lookup
/// \param events events the skeleton provides
/// \return estimated size in bytes
std::size_t EstimateDataShmResourceSize(const LolaServiceInstanceDeployment& instance_deployment,
                                        const SkeletonBinding::SkeletonEventBindings& events,
                                        const SkeletonBinding::SkeletonFieldBindings& fields) noexcept
{
    // Explanation of estimation algo/approach -> see comment in EstimateControlShmResourceSize()

    std::size_t data_resource_size{};
    data_resource_size += sizeof(bmw::mw::com::impl::lola::ServiceDataStorage);
    // since ServiceDataStorage contains two std::maps ->
    data_resource_size += (2U * STL_CONTAINER_STORAGE_NEEDS);

    // For the moment, fields are equivalent to events in terms of shared memory footprint. Therefore, we can use the
    // same calculation to estimate the element size of an event or field.
    constexpr auto CalculateServiceElementSize = [](const std::size_t max_samples,
                                                    const std::size_t max_size) -> std::size_t {
        // 1st the storage size per event_map_element
        std::size_t event_map_element_size = sizeof(decltype(ServiceDataStorage::events_)::value_type);
        event_map_element_size += STL_CONTAINER_ELEMENT_STORAGE_NEEDS;
        // the mapped type again is a vector, so add STL_CONTAINER_STORAGE_NEEDS
        event_map_element_size += STL_CONTAINER_STORAGE_NEEDS;
        // and it contains max_samples_ data slots
        event_map_element_size += (max_samples * max_size);
        // 2nd the storage size per meta_info_map_element
        std::size_t meta_info_map_element_size = sizeof(decltype(ServiceDataStorage::events_metainfo_)::value_type);
        meta_info_map_element_size += STL_CONTAINER_ELEMENT_STORAGE_NEEDS;
        return event_map_element_size + meta_info_map_element_size;
    };
    for (const auto& event : events)
    {
        const auto search = instance_deployment.events_.find(std::string(event.first.data(), event.first.size()));
        AMP_ASSERT_PRD_MESSAGE(search != instance_deployment.events_.cend(),
                               "Deployment doesn't contain event with given name!");
        const auto max_samples = static_cast<std::size_t>(search->second.GetNumberOfSampleSlots().value());
        const auto max_size = event.second.get().GetMaxSize();
        data_resource_size += CalculateServiceElementSize(max_samples, max_size);
    }

    for (const auto& field : fields)
    {
        const auto search = instance_deployment.fields_.find(std::string(field.first.data(), field.first.size()));
        AMP_ASSERT_PRD_MESSAGE(search != instance_deployment.fields_.cend(),
                               "Deployment doesn't contain field with given name!");
        const auto max_samples = static_cast<std::size_t>(search->second.GetNumberOfSampleSlots().value());
        const auto max_size = field.second.get().GetMaxSize();
        data_resource_size += CalculateServiceElementSize(max_samples, max_size);
    }
    return data_resource_size;
}

bool CreatePartialRestartDirectory(
    const bmw::filesystem::Filesystem& filesystem,
    const std::unique_ptr<IPartialRestartPathBuilder>& partial_restart_path_builder) noexcept
{
    const auto partial_restart_dir_path{partial_restart_path_builder->GetLolaPartialRestartDirectoryPath()};

    constexpr auto permissions{os::Stat::Mode::kReadWriteExecUser | os::Stat::Mode::kReadWriteExecGroup |
                               os::Stat::Mode::kReadWriteExecOthers};
    const auto create_dir_result = filesystem.utils->CreateDirectories(partial_restart_dir_path, permissions);
    if (!create_dir_result.has_value())
    {
        bmw::mw::log::LogError("lola") << create_dir_result.error().Message()
                                       << ":CreateDirectories failed:" << create_dir_result.error().UserMessage();
        return false;
    }
    return true;
}

amp::optional<memory::shared::LockFile> CreateOrOpenServiceInstanceExistenceMarkerFile(
    const InstanceIdentifier& identifier,
    const std::unique_ptr<IPartialRestartPathBuilder>& partial_restart_path_builder) noexcept
{
    const auto service_instance_deployment = GetLolaServiceInstanceDeployment(identifier);
    const auto instance_id = service_instance_deployment.instance_id_.value().id_;
    auto service_instance_existence_marker_file_path{
        partial_restart_path_builder->GetServiceInstanceExistenceMarkerFilePath(instance_id)};

    // The instance existence marker file can be opened in the case that another skeleton of the same service currently
    // exists or that a skeleton of the same service previously crashed. We cannot determine which is true until we try
    // to flock the file. Therefore, we do not take ownership on construction and take ownership later if we can
    // exclusively flock the file.
    bool take_ownership{false};
    return memory::shared::LockFile::CreateOrOpen(std::move(service_instance_existence_marker_file_path),
                                                  take_ownership);
}

amp::optional<memory::shared::LockFile> CreateOrOpenServiceInstanceUsageMarkerFile(
    const InstanceIdentifier& identifier,
    const std::unique_ptr<IPartialRestartPathBuilder>& partial_restart_path_builder) noexcept
{
    const auto service_instance_deployment = GetLolaServiceInstanceDeployment(identifier);
    const auto instance_id = service_instance_deployment.instance_id_.value().id_;
    auto service_instance_usage_marker_file_path{
        partial_restart_path_builder->GetServiceInstanceUsageMarkerFilePath(instance_id)};

    // The instance usage marker file should be created if the skeleton is starting up for the very first time and
    // opened in all other cases.
    // We should never take ownership of the file so that it remains in the filesystem indefinitely. This is because
    // proxies might still have a shared lock on the file while destructing the skeleton. It is imperative to retain
    // this knowledge between skeleton restarts.
    constexpr bool take_ownership{false};
    return memory::shared::LockFile::CreateOrOpen(service_instance_usage_marker_file_path, take_ownership);
}

std::string GetControlChannelShmPath(const InstanceIdentifier& identifier,
                                     const QualityType quality_type,
                                     const std::unique_ptr<IShmPathBuilder>& shm_path_builder) noexcept
{
    const auto service_instance_deployment = GetLolaServiceInstanceDeployment(identifier);
    const auto instance_id = service_instance_deployment.instance_id_.value().id_;
    return shm_path_builder->GetControlChannelShmName(instance_id, quality_type);
}

std::string GetDataChannelShmPath(const InstanceIdentifier& identifier,
                                  const std::unique_ptr<IShmPathBuilder>& shm_path_builder) noexcept
{
    const auto service_instance_deployment = GetLolaServiceInstanceDeployment(identifier);
    const auto instance_id = service_instance_deployment.instance_id_.value().id_;
    return shm_path_builder->GetDataChannelShmName(instance_id);
}

}  // namespace

namespace detail_skeleton
{

bool HasAsilBSupport(const InstanceIdentifier& identifier) noexcept
{
    return (InstanceIdentifierView{identifier}.GetServiceInstanceDeployment().asilLevel_ == QualityType::kASIL_B);
}

}  // namespace detail_skeleton

std::unique_ptr<Skeleton> Skeleton::Create(
    const InstanceIdentifier& identifier,
    bmw::filesystem::Filesystem filesystem,
    std::unique_ptr<IShmPathBuilder> shm_path_builder,
    std::unique_ptr<IPartialRestartPathBuilder> partial_restart_path_builder) noexcept
{
    const auto partial_restart_dir_creation_result =
        CreatePartialRestartDirectory(filesystem, partial_restart_path_builder);
    if (!partial_restart_dir_creation_result)
    {
        bmw::mw::log::LogError("lola") << "Could not create partial restart directory.";
        return nullptr;
    }

    auto service_instance_existence_marker_file =
        CreateOrOpenServiceInstanceExistenceMarkerFile(identifier, partial_restart_path_builder);
    if (!service_instance_existence_marker_file.has_value())
    {
        bmw::mw::log::LogError("lola") << "Could not create or open service instance existence marker file.";
        return nullptr;
    }

    auto service_instance_existence_mutex_and_lock =
        std::make_unique<memory::shared::FlockMutexAndLock<memory::shared::ExclusiveFlockMutex>>(
            *service_instance_existence_marker_file);
    if (!service_instance_existence_mutex_and_lock->TryLock())
    {
        bmw::mw::log::LogError("lola")
            << "Flock try_lock failed: Another Skeleton could have already flocked the marker file and is "
               "actively offering the same service instance.";
        return nullptr;
    }

    // Since we were able to flock the existence marker file, it means that either we created it or the skeleton that
    // created it previously crashed. Either way, we take ownership of the LockFile so that it's destroyed when this
    // Skeleton is destroyed.
    service_instance_existence_marker_file.value().TakeOwnership();
    return std::make_unique<lola::Skeleton>(identifier,
                                            std::move(filesystem),
                                            std::move(shm_path_builder),
                                            std::move(partial_restart_path_builder),
                                            std::move(service_instance_existence_marker_file),
                                            std::move(service_instance_existence_mutex_and_lock));
}

Skeleton::Skeleton(const InstanceIdentifier& identifier,
                   bmw::filesystem::Filesystem filesystem,
                   std::unique_ptr<IShmPathBuilder> shm_path_builder,
                   std::unique_ptr<IPartialRestartPathBuilder> partial_restart_path_builder,
                   amp::optional<memory::shared::LockFile> service_instance_existence_marker_file,
                   std::unique_ptr<memory::shared::FlockMutexAndLock<memory::shared::ExclusiveFlockMutex>>
                       service_instance_existence_flock_mutex_and_lock)
    : SkeletonBinding{},
      identifier_{identifier},
      data_storage_path_{},
      data_control_qm_path_{},
      data_control_asil_path_{},
      storage_{nullptr},
      control_qm_{nullptr},
      control_asil_b_{nullptr},
      storage_resource_{},
      control_qm_resource_{},
      control_asil_resource_{},
      shm_path_builder_{std::move(shm_path_builder)},
      partial_restart_path_builder_{std::move(partial_restart_path_builder)},
      service_instance_existence_marker_file_{std::move(service_instance_existence_marker_file)},
      service_instance_usage_marker_file_{},
      service_instance_existence_flock_mutex_and_lock_{std::move(service_instance_existence_flock_mutex_and_lock)},
      was_old_shm_region_reopened_{false},
      filesystem_{std::move(filesystem)}
{
}

auto Skeleton::PrepareOffer(SkeletonEventBindings& events,
                            SkeletonFieldBindings& fields,
                            amp::optional<RegisterShmObjectTraceCallback> register_shm_object_trace_callback) noexcept
    -> ResultBlank
{
    EnrichedInstanceIdentifier enriched_instance_identifier{identifier_};
    const auto service_id =
        enriched_instance_identifier.GetBindingSpecificServiceId<LolaServiceTypeDeployment>().value();
    const auto instance_id = enriched_instance_identifier.GetBindingSpecificInstanceId<LolaServiceInstanceId>().value();

    service_instance_usage_marker_file_ =
        CreateOrOpenServiceInstanceUsageMarkerFile(identifier_, partial_restart_path_builder_);
    if (!service_instance_usage_marker_file_.has_value())
    {
        bmw::mw::log::LogError("lola") << "Could not create or open service instance usage marker file.";
        /// \todo Use logical error code
        return MakeUnexpected(ComErrc::kBindingFailure);
    }

    memory::shared::ExclusiveFlockMutex service_instance_usage_mutex{*service_instance_usage_marker_file_};
    std::unique_lock<memory::shared::ExclusiveFlockMutex> service_instance_usage_lock{service_instance_usage_mutex,
                                                                                      std::defer_lock};
    const bool previous_shm_region_unused_by_proxies = service_instance_usage_lock.try_lock();
    was_old_shm_region_reopened_ = !previous_shm_region_unused_by_proxies;
    if (previous_shm_region_unused_by_proxies)
    {

        bmw::mw::log::LogDebug("lola") << "Recreating SHM of Skeleton (S:" << service_id << "I:" << instance_id << ")";
        // Since the previous shared memory region is not being currently used by proxies, this can mean 2 things: (1)
        // The previous shared memory was properly created and OfferService finished (the SkeletonBinding and all
        // Skeleton service elements finished their PrepareOffer calls) and either no Proxies subscribed or they have
        // all since unsubscribed. Or, (2), the previous Skeleton crashed while setting up the shared memory. Since we
        // don't differentiate between the 2 cases and because it's unused anyway, we simply remove the old memory
        // region and re-create it.
        RemoveStaleSharedMemoryArtefacts();

        const auto create_result = CreateSharedMemory(events, fields, std::move(register_shm_object_trace_callback));
        return create_result;
    }
    else
    {
        bmw::mw::log::LogDebug("lola") << "Reusing SHM of Skeleton (S:" << service_id << "I:" << instance_id << ")";
        // Since the previous shared memory region is being currently used by proxies, it must have been properly
        // created and OfferService finished. Therefore, we can simply re-open it and cleanup any previous in-writing
        // transactions by the previous skeleton.
        const auto open_result = OpenExistingSharedMemory(std::move(register_shm_object_trace_callback));
        if (!open_result.has_value())
        {
            return open_result;
        }
        CleanupSharedMemoryAfterCrash();
        return {};
    }
}

ResultBlank Skeleton::FinalizeOffer() noexcept
{
    return {};
}

auto Skeleton::PrepareStopOffer(amp::optional<UnregisterShmObjectTraceCallback> unregister_shm_object_callback) noexcept
    -> void
{
    if (unregister_shm_object_callback.has_value())
    {
        unregister_shm_object_callback.value()(
            amp::string_view{tracing::TracingRuntime::kDummyElementNameForShmRegisterCallback},
            tracing::TracingRuntime::kDummyElementTypeForShmRegisterCallback);
    }

    memory::shared::ExclusiveFlockMutex service_instance_usage_mutex{*service_instance_usage_marker_file_};
    std::unique_lock<memory::shared::ExclusiveFlockMutex> service_instance_usage_lock{service_instance_usage_mutex,
                                                                                      std::defer_lock};
    if (!service_instance_usage_lock.try_lock())
    {
        bmw::mw::log::LogInfo("lola")
            << "Skeleton::RemoveSharedMemory(): Could not exclusively lock service instance usage "
               "marker file indicating that some proxies are still subscribed. Will not remove shared memory.";
        return;
    }
    else
    {
        RemoveSharedMemory();
        service_instance_usage_lock.unlock();
        service_instance_usage_marker_file_.reset();
    }

    storage_ = nullptr;
    control_qm_ = nullptr;
    control_asil_b_ = nullptr;
}

auto Skeleton::CreateSharedMemory(
    SkeletonEventBindings& events,
    SkeletonFieldBindings& fields,
    amp::optional<RegisterShmObjectTraceCallback> register_shm_object_trace_callback) noexcept -> ResultBlank
{
    const auto storage_size_calc_result = CalculateShmResourceStorageSizes(events, fields);
    const auto& service_instance_deployment = GetLolaServiceInstanceDeployment(identifier_);

    if (!CreateSharedMemoryForControl(
            service_instance_deployment, QualityType::kASIL_QM, storage_size_calc_result.control_qm_size))
    {
        return MakeUnexpected(ComErrc::kErroneousFileHandle, "Could not create shared memory object for control QM");
    }

    if (detail_skeleton::HasAsilBSupport(identifier_) &&
        (!CreateSharedMemoryForControl(
            service_instance_deployment, QualityType::kASIL_B, storage_size_calc_result.control_asil_b_size.value())))
    {
        return MakeUnexpected(ComErrc::kErroneousFileHandle,
                              "Could not create shared memory object for control ASIL-B");
    }

    if (!CreateSharedMemoryForData(service_instance_deployment,
                                   storage_size_calc_result.data_size,
                                   std::move(register_shm_object_trace_callback)))
    {
        return MakeUnexpected(ComErrc::kErroneousFileHandle, "Could not create shared memory object for data");
    }
    return {};
}

auto Skeleton::OpenExistingSharedMemory(
    amp::optional<RegisterShmObjectTraceCallback> register_shm_object_trace_callback) noexcept -> ResultBlank
{
    if (!OpenSharedMemoryForControl(QualityType::kASIL_QM))
    {
        return MakeUnexpected(ComErrc::kErroneousFileHandle, "Could not open shared memory object for control QM");
    }

    if (detail_skeleton::HasAsilBSupport(identifier_) && (!OpenSharedMemoryForControl(QualityType::kASIL_B)))
    {
        return MakeUnexpected(ComErrc::kErroneousFileHandle, "Could not open shared memory object for control ASIL-B");
    }

    if (!OpenSharedMemoryForData(std::move(register_shm_object_trace_callback)))
    {
        return MakeUnexpected(ComErrc::kErroneousFileHandle, "Could not open shared memory object for data");
    }
    return {};
}

bool Skeleton::CreateSharedMemoryForData(
    const LolaServiceInstanceDeployment& instance,
    const std::size_t shm_size,
    amp::optional<RegisterShmObjectTraceCallback> register_shm_object_trace_callback) noexcept
{
    memory::shared::SharedMemoryFactory::UserPermissionsMap permissions{};
    for (const auto& allowed_consumer : instance.allowed_consumer_)
    {
        for (const auto& user_identifier : allowed_consumer.second)
        {
            permissions[bmw::os::Acl::Permission::kRead].push_back(user_identifier);
        }
    }

    const auto& service_instance_deployment = GetLolaServiceInstanceDeployment(identifier_);
    const auto instance_id = service_instance_deployment.instance_id_.value().id_;
    const auto path = shm_path_builder_->GetDataChannelShmName(instance_id);
    const bool use_typed_memory = register_shm_object_trace_callback.has_value();
    const auto memory_resource = bmw::memory::shared::SharedMemoryFactory::Create(
        path,
        [this](std::shared_ptr<bmw::memory::shared::ManagedMemoryResource> memory) {
            this->InitializeSharedMemoryForData(memory);
        },
        shm_size,
        permissions.empty() && instance.strict_permissions_ == false
            ? memory::shared::SharedMemoryFactory::WorldReadable{}
            : memory::shared::SharedMemoryFactory::UserPermissions{permissions},
        use_typed_memory);
    if (memory_resource == nullptr)
    {
        return false;
    }
    data_storage_path_ = path;
    LogAclOfShmObj(memory_resource);
    if (memory_resource->IsShmInTypedMemory())
    {
        // only if the memory_resource could be successfully allocated in typed-memory, we call back the
        // register_shm_object_trace_callback, because only then the shm-object can be accessed by tracing
        // subsystem.
        // Since LoLa creates shm-objects on the granularity of whole service-instances (including ALL its service
        // elements), we call register_shm_object_trace_callback once and hand over a dummy element name/type!
        // Other bindings, which might create shm-objects per service-element would call
        // register_shm_object_trace_callback for each service-element and then use their "real" name and type ...
        register_shm_object_trace_callback.value()(tracing::TracingRuntime::kDummyElementNameForShmRegisterCallback,
                                                   tracing::TracingRuntime::kDummyElementTypeForShmRegisterCallback,
                                                   memory_resource->GetFileDescriptor(),
                                                   memory_resource->getBaseAddress());
    }

    bmw::mw::log::LogDebug("lola") << "Creating SHM of Skeleton (I:" << instance_id << ")";
    return true;
}

bool Skeleton::CreateSharedMemoryForControl(const LolaServiceInstanceDeployment& instance,
                                            const QualityType asil_level,
                                            const std::size_t shm_size) noexcept
{
    const auto& service_instance_deployment = GetLolaServiceInstanceDeployment(identifier_);
    const auto instance_id = service_instance_deployment.instance_id_.value().id_;
    const auto path = shm_path_builder_->GetControlChannelShmName(instance_id, asil_level);

    const auto consumer = instance.allowed_consumer_.find(asil_level);
    auto& control_resource = (asil_level == QualityType::kASIL_QM) ? control_qm_resource_ : control_asil_resource_;
    auto& data_control_path = (asil_level == QualityType::kASIL_QM) ? data_control_qm_path_ : data_control_asil_path_;

    memory::shared::SharedMemoryFactory::UserPermissionsMap permissions{};
    if (consumer != instance.allowed_consumer_.cend())
    {
        for (const auto& user_identifier : consumer->second)
        {
            permissions[bmw::os::Acl::Permission::kRead].push_back(user_identifier);
            permissions[bmw::os::Acl::Permission::kWrite].push_back(user_identifier);
        }
    }

    control_resource = bmw::memory::shared::SharedMemoryFactory::Create(
        path,
        [this, asil_level](std::shared_ptr<bmw::memory::shared::ManagedMemoryResource> memory) {
            this->InitializeSharedMemoryForControl(asil_level, memory);
        },
        shm_size,
        permissions.empty() && instance.strict_permissions_ == false
            ? memory::shared::SharedMemoryFactory::WorldWritable{}
            : memory::shared::SharedMemoryFactory::UserPermissions{permissions});
    if (control_resource == nullptr)
    {
        return false;
    }
    data_control_path = path;
    // static cast is safe here as at this stage members control_qm_resource_/control_asil_resource_ are
    // SharedMemoryResources!
    LogAclOfShmObj(std::static_pointer_cast<bmw::memory::shared::ISharedMemoryResource>(control_resource));
    return true;
}

bool Skeleton::OpenSharedMemoryForData(
    const amp::optional<RegisterShmObjectTraceCallback> register_shm_object_trace_callback) noexcept
{
    const auto path = GetDataChannelShmPath(identifier_, shm_path_builder_);

    const auto memory_resource = bmw::memory::shared::SharedMemoryFactory::Open(path, true);
    if (memory_resource == nullptr)
    {
        return false;
    }
    data_storage_path_ = path;
    storage_resource_ = memory_resource;

    storage_ = GetServiceDataStorage(memory_resource);

    // Our pid will have changed after re-start and we now have to update it in the re-opened DATA section.
    const auto pid = GetLoLaRuntime().GetPid();
    const auto service_instance_deployment = GetLolaServiceInstanceDeployment(identifier_);
    const auto instance_id = service_instance_deployment.instance_id_.value().id_;
    bmw::mw::log::LogDebug("lola") << "Updating PID of Skeleton (I:" << instance_id << ") with:" << pid;
    storage_->skeleton_pid_ = pid;

    if (memory_resource->IsShmInTypedMemory())
    {
        // only if the memory_resource could be successfully allocated in typed-memory, we call back the
        // register_shm_object_trace_callback, because only then the shm-object can be accessed by tracing
        // subsystem.
        register_shm_object_trace_callback.value()(tracing::TracingRuntime::kDummyElementNameForShmRegisterCallback,
                                                   tracing::TracingRuntime::kDummyElementTypeForShmRegisterCallback,
                                                   memory_resource->GetFileDescriptor(),
                                                   memory_resource->getBaseAddress());
    }
    return true;
}

bool Skeleton::OpenSharedMemoryForControl(const QualityType asil_level) noexcept
{
    const auto path = GetControlChannelShmPath(identifier_, asil_level, shm_path_builder_);

    auto& control_resource = (asil_level == QualityType::kASIL_QM) ? control_qm_resource_ : control_asil_resource_;
    auto& data_control_path = (asil_level == QualityType::kASIL_QM) ? data_control_qm_path_ : data_control_asil_path_;

    control_resource = bmw::memory::shared::SharedMemoryFactory::Open(path, true);
    if (control_resource == nullptr)
    {
        return false;
    }
    data_control_path = path;

    auto& control = (asil_level == QualityType::kASIL_QM) ? control_qm_ : control_asil_b_;

    control = GetServiceDataControl(control_resource);

    return true;
}

void Skeleton::RemoveSharedMemory() noexcept
{
    constexpr auto RemoveMemoryIfExists = [](const amp::optional<std::string>& path) -> void {
        if (path.has_value())
        {
            bmw::memory::shared::SharedMemoryFactory::Remove(path.value());
        }
    };
    RemoveMemoryIfExists(data_control_qm_path_);
    RemoveMemoryIfExists(data_control_asil_path_);
    RemoveMemoryIfExists(data_storage_path_);

    storage_resource_.reset();
    control_qm_resource_.reset();
    control_asil_resource_.reset();
}

void Skeleton::RemoveStaleSharedMemoryArtefacts() const noexcept
{
    const auto control_qm_path = GetControlChannelShmPath(identifier_, QualityType::kASIL_QM, shm_path_builder_);
    const auto control_asil_b_path = GetControlChannelShmPath(identifier_, QualityType::kASIL_B, shm_path_builder_);
    const auto data_path = GetDataChannelShmPath(identifier_, shm_path_builder_);

    memory::shared::SharedMemoryFactory::RemoveStaleArtefacts(control_qm_path);
    memory::shared::SharedMemoryFactory::RemoveStaleArtefacts(control_asil_b_path);
    memory::shared::SharedMemoryFactory::RemoveStaleArtefacts(data_path);
}

Skeleton::ShmResourceStorageSizes Skeleton::CalculateShmResourceStorageSizesBySimulation(
    SkeletonEventBindings& events,
    SkeletonFieldBindings& fields) noexcept
{
    using NewDeleteDelegateMemoryResource = memory::shared::NewDeleteDelegateMemoryResource;

    // we create up to 3 DryRun Memory Resources and then do the "normal" initialization of control and data
    // shm-objects on it
    const auto& service_type_deployment = GetLolaServiceTypeDeployment(identifier_);
    const auto& service_instance_deployment = GetLolaServiceInstanceDeployment(identifier_);
    control_qm_resource_ = std::make_shared<NewDeleteDelegateMemoryResource>(
        CalculateMemoryResourceId(service_type_deployment, service_instance_deployment, ShmObjectType::kControl_QM));

    storage_resource_ = std::make_shared<NewDeleteDelegateMemoryResource>(
        CalculateMemoryResourceId(service_type_deployment, service_instance_deployment, ShmObjectType::kData));

    // Note, that it is important to have all DryRun Memory Resources "active" in parallel as the upcoming calls to
    // PrepareOffer() for the events triggers all SkeletonEvents to register themselves at their parent Skeleton
    // (lola::Skeleton::Register()), which leads to updates/allocation within ctrl AND data resources!
    InitializeSharedMemoryForControl(QualityType::kASIL_QM, control_qm_resource_);

    if (detail_skeleton::HasAsilBSupport(identifier_))
    {
        control_asil_resource_ = std::make_shared<NewDeleteDelegateMemoryResource>(CalculateMemoryResourceId(
            service_type_deployment, service_instance_deployment, ShmObjectType::kControl_ASIL_B));
        InitializeSharedMemoryForControl(QualityType::kASIL_B, control_asil_resource_);
    }
    InitializeSharedMemoryForData(storage_resource_);

    // Offer events to calculate the shared memory allocated for the control and data segments for each event
    for (auto& event : events)
    {
        amp::ignore = event.second.get().PrepareOffer();
    }
    for (auto& field : fields)
    {
        amp::ignore = field.second.get().PrepareOffer();
    }

    const auto control_qm_size = control_qm_resource_->GetUserAllocatedBytes();
    const auto control_data_size = storage_resource_->GetUserAllocatedBytes();

    const auto control_asil_b_size = detail_skeleton::HasAsilBSupport(identifier_)
                                         ? amp::optional<std::size_t>{control_asil_resource_->GetUserAllocatedBytes()}
                                         : amp::optional<std::size_t>{};

    return ShmResourceStorageSizes{control_data_size, control_qm_size, control_asil_b_size};
}

Skeleton::ShmResourceStorageSizes Skeleton::CalculateShmResourceStorageSizesByEstimation(
    SkeletonEventBindings& events,
    SkeletonFieldBindings& fields) const noexcept
{
    const auto control_qm_size =
        EstimateControlShmResourceSize(GetLolaServiceInstanceDeployment(identifier_), events, fields);
    const auto control_asil_b_size = detail_skeleton::HasAsilBSupport(identifier_)
                                         ? amp::optional<std::size_t>{control_qm_size}
                                         : amp::optional<std::size_t>{};

    const auto data_size = EstimateDataShmResourceSize(GetLolaServiceInstanceDeployment(identifier_), events, fields);

    return ShmResourceStorageSizes{data_size, control_qm_size, control_asil_b_size};
}

Skeleton::ShmResourceStorageSizes Skeleton::CalculateShmResourceStorageSizes(SkeletonEventBindings& events,
                                                                             SkeletonFieldBindings& fields) noexcept
{
    const auto result = GetLoLaRuntime().GetShmSizeCalculationMode() == ShmSizeCalculationMode::kSimulation
                            ? CalculateShmResourceStorageSizesBySimulation(events, fields)
                            : CalculateShmResourceStorageSizesByEstimation(events, fields);

    bmw::mw::log::LogInfo("lola") << "Calculated sizes of shm-objects for " << identifier_.ToString()
                                  << " is as follows:\nQM-Ctrl: " << result.control_qm_size << ", ASIL_B-Ctrl: "
                                  << (result.control_asil_b_size.has_value() ? result.control_asil_b_size.value() : 0)
                                  << ", Data: " << result.data_size;

    const auto& service_instance_deployment = GetLolaServiceInstanceDeployment(identifier_);

    if (service_instance_deployment.shared_memory_size_.has_value())
    {
        if (service_instance_deployment.shared_memory_size_.value() < result.data_size)
        {
            bmw::mw::log::LogWarn("lola")
                << "Skeleton::CalculateShmResourceStorageSizes() calculates a needed shm-size for DATA of: "
                << result.data_size << " bytes, but user configured value in deployment is smaller: "
                << service_instance_deployment.shared_memory_size_.value();
        }
        return {service_instance_deployment.shared_memory_size_.value(),
                result.control_qm_size,
                result.control_asil_b_size};
    }

    return result;
}

amp::optional<EventMetaInfo> Skeleton::GetEventMetaInfo(const ElementFqId element_fq_id) const noexcept
{
    auto search = storage_->events_metainfo_.find(element_fq_id);
    if (search == storage_->events_metainfo_.cend())
    {
        return amp::nullopt;
    }
    else
    {
        return search->second;
    }
}

QualityType Skeleton::GetInstanceQualityType() const noexcept
{
    return InstanceIdentifierView{identifier_}.GetServiceInstanceDeployment().asilLevel_;
}

void Skeleton::CleanupSharedMemoryAfterCrash() noexcept
{
    for (auto& event : control_qm_->event_controls_)
    {
        event.second.data_control.RemoveAllocationsForWriting();
    }

    if (control_asil_b_ != nullptr)
    {
        for (auto& event : control_asil_b_->event_controls_)
        {
            event.second.data_control.RemoveAllocationsForWriting();
        }
    }
}

void Skeleton::DisconnectQmConsumers() noexcept
{
    AMP_ASSERT_PRD_MESSAGE(GetInstanceQualityType() == QualityType::kASIL_B,
                           "DisconnectQmConsumers() called on a QualityType::kASIL_QM instance!");

    auto result = impl::Runtime::getInstance().GetServiceDiscovery().StopOfferService(
        identifier_, IServiceDiscovery::QualityTypeSelector::kAsilQm);
    if (!result.has_value())
    {
        bmw::mw::log::LogWarn("lola")
            << __func__ << __LINE__
            << "Disconnecting unsafe QM consumers via StopOffer of ASIL-QM part of service instance failed.";
    }
}

void Skeleton::InitializeSharedMemoryForData(
    const std::shared_ptr<bmw::memory::shared::ManagedMemoryResource>& memory) noexcept
{
    storage_ = memory->construct<ServiceDataStorage>(memory->getMemoryResourceProxy());
    storage_resource_ = memory;
    AMP_ASSERT_PRD_MESSAGE(storage_resource_ != nullptr,
                           "storage_resource_ must be no nullptr, otherwise the callback would not be invoked.");
}

void Skeleton::InitializeSharedMemoryForControl(
    const QualityType asil_level,
    const std::shared_ptr<bmw::memory::shared::ManagedMemoryResource>& memory) noexcept
{
    auto& control = (asil_level == QualityType::kASIL_QM) ? control_qm_ : control_asil_b_;
    control = memory->construct<ServiceDataControl>(memory->getMemoryResourceProxy());
}

}  // namespace lola
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
