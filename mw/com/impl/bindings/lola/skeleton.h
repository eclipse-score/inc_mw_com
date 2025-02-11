// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#ifndef PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_SKELETON_H
#define PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_SKELETON_H

#include "platform/aas/mw/com/impl/bindings/lola/element_fq_id.h"
#include "platform/aas/mw/com/impl/bindings/lola/event_data_control_composite.h"
#include "platform/aas/mw/com/impl/bindings/lola/event_data_storage.h"
#include "platform/aas/mw/com/impl/bindings/lola/i_partial_restart_path_builder.h"
#include "platform/aas/mw/com/impl/bindings/lola/i_shm_path_builder.h"
#include "platform/aas/mw/com/impl/bindings/lola/runtime.h"
#include "platform/aas/mw/com/impl/bindings/lola/service_data_control.h"
#include "platform/aas/mw/com/impl/bindings/lola/service_data_storage.h"
#include "platform/aas/mw/com/impl/bindings/lola/skeleton_event_properties.h"
#include "platform/aas/mw/com/impl/configuration/lola_event_id.h"
#include "platform/aas/mw/com/impl/configuration/lola_service_instance_deployment.h"
#include "platform/aas/mw/com/impl/instance_identifier.h"
#include "platform/aas/mw/com/impl/runtime.h"
#include "platform/aas/mw/com/impl/skeleton_binding.h"
#include "platform/aas/mw/com/impl/tracing/skeleton_event_tracing.h"
#include "platform/aas/mw/com/impl/tracing/skeleton_event_tracing_data.h"

#include "platform/aas/lib/filesystem/filesystem.h"
#include "platform/aas/lib/memory/shared/flock/exclusive_flock_mutex.h"
#include "platform/aas/lib/memory/shared/flock/flock_mutex_and_lock.h"
#include "platform/aas/lib/memory/shared/i_shared_memory_resource.h"
#include "platform/aas/lib/memory/shared/lock_file.h"

#include <amp_assert.hpp>
#include <amp_optional.hpp>
#include <amp_string_view.hpp>

#include <memory>
#include <string>
#include <tuple>
#include <utility>

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

/// \brief LoLa Skeleton implement all binding specific functionalities that are needed by a Skeleton.
/// This includes all actions that need to be performed on Service offerings, as also the possibility to register events
/// dynamically at this skeleton.
class Skeleton final : public SkeletonBinding
{
    friend class SkeletonAttorney;

  public:
    static std::unique_ptr<Skeleton> Create(
        const InstanceIdentifier& identifier,
        bmw::filesystem::Filesystem filesystem,
        std::unique_ptr<IShmPathBuilder> shm_path_builder,
        std::unique_ptr<IPartialRestartPathBuilder> partial_restart_path_builder) noexcept;

    /// \brief Construct a Skeleton instance for this specific instance with possibility of passing mock objects during
    /// construction. It is only for testing. For production code Skeleton::Create shall be used.
    Skeleton(const InstanceIdentifier& identifier,
             bmw::filesystem::Filesystem filesystem,
             std::unique_ptr<IShmPathBuilder> shm_path_builder,
             std::unique_ptr<IPartialRestartPathBuilder> partial_restart_path_builder,
             amp::optional<memory::shared::LockFile> service_instance_existence_marker_file,
             std::unique_ptr<bmw::memory::shared::FlockMutexAndLock<bmw::memory::shared::ExclusiveFlockMutex>>
                 service_instance_existence_flock_mutex_and_lock);
    ~Skeleton() override = default;

    ResultBlank PrepareOffer(
        SkeletonEventBindings& events,
        SkeletonFieldBindings& fields,
        amp::optional<RegisterShmObjectTraceCallback> register_shm_object_trace_callback) noexcept override final;

    ResultBlank FinalizeOffer() noexcept override final;

    void PrepareStopOffer(
        amp::optional<UnregisterShmObjectTraceCallback> unregister_shm_object_callback) noexcept override final;

    BindingType GetBindingType() const noexcept override final { return BindingType::kLoLa; };

    /// \brief Enables dynamic registration of Events at the Skeleton.
    /// \tparam SampleType The type of the event
    /// \param element_fq_id The full qualified of the element (event or field) that shall be registered
    /// \param element_properties properties of the element, which are currently event specific properties.
    /// \param skeleton_event_tracing_data struct which contains flags for enabling or disabling specific trace points.
    ///        If the send or send_with_allocate trace points are enabled, then a TransactionLog will be registered for
    ///        the registered event.
    /// \return The registered data structures within the Skeleton
    /// (first: where to store data, second: control data
    ///         access) If PrepareOffer created the shared memory, then will create an EventDataControl (for QM and
    ///         optionally for ASIL B) and an EventDataStorage which will be returned. If PrepareOffer opened the shared
    ///         memory, then the opened event data from the existing shared memory will be returned.
    template <typename SampleType>
    std::pair<EventDataStorage<SampleType>*, EventDataControlComposite> Register(
        const ElementFqId element_fq_id,
        SkeletonEventProperties element_properties,
        amp::optional<impl::tracing::SkeletonEventTracingData> skeleton_event_tracing_data = {}) noexcept;

    /// \brief Returns the meta-info for the given registered event.
    /// \param element_fq_id identification of the event.
    /// \return Events meta-info, if it has been registered, null else.
    amp::optional<EventMetaInfo> GetEventMetaInfo(const ElementFqId element_fq_id) const noexcept;

    QualityType GetInstanceQualityType() const noexcept;

    /// \brief Cleans up all allocated slots for this SkeletonEvent of any previous running instance
    /// \details Note: Only invoke _after_ a crash was detected!
    void CleanupSharedMemoryAfterCrash() noexcept;

    /// \brief "Disconnects" unsafe QM consumers by stop-offering the service instances QM part.
    /// \details Only supported/valid for a skeleton instance, where GetInstanceQualityType() returns
    ///          QualityType::kASIL_B. Calling it for a skeleton, which returns QualityType::kASIL_QM, will lead to
    ///          an assert/termination.
    void DisconnectQmConsumers() noexcept;

  private:
    ResultBlank OpenExistingSharedMemory(
        amp::optional<RegisterShmObjectTraceCallback> register_shm_object_trace_callback) noexcept;
    ResultBlank CreateSharedMemory(
        SkeletonEventBindings& events,
        SkeletonFieldBindings& fields,
        amp::optional<RegisterShmObjectTraceCallback> register_shm_object_trace_callback) noexcept;

    bool CreateSharedMemoryForData(
        const LolaServiceInstanceDeployment&,
        const std::size_t shm_size,
        amp::optional<RegisterShmObjectTraceCallback> register_shm_object_trace_callback) noexcept;
    bool CreateSharedMemoryForControl(const LolaServiceInstanceDeployment& instance,
                                      const QualityType asil_level,
                                      const std::size_t shm_size) noexcept;
    bool OpenSharedMemoryForData(
        const amp::optional<RegisterShmObjectTraceCallback> register_shm_object_trace_callback) noexcept;
    bool OpenSharedMemoryForControl(const QualityType asil_level) noexcept;
    void InitializeSharedMemoryForData(
        const std::shared_ptr<bmw::memory::shared::ManagedMemoryResource>& memory) noexcept;
    void InitializeSharedMemoryForControl(
        const QualityType asil_level,
        const std::shared_ptr<bmw::memory::shared::ManagedMemoryResource>& memory) noexcept;

    template <typename SampleType>
    std::pair<EventDataStorage<SampleType>*, EventDataControlComposite> OpenEventDataFromOpenedSharedMemory(
        const ElementFqId element_fq_id) noexcept;

    template <typename SampleType>
    std::pair<EventDataStorage<SampleType>*, EventDataControlComposite> CreateEventDataFromOpenedSharedMemory(
        const ElementFqId element_fq_id,
        const SkeletonEventProperties& element_properties) noexcept;

    struct ShmResourceStorageSizes
    {
        const std::size_t data_size;
        const std::size_t control_qm_size;
        const amp::optional<std::size_t> control_asil_b_size;
    };

    /// \brief Calculates needed sizes for shm-objects for data and ctrl either via simulation or a rough estimation
    /// depending on config.
    /// \return storage sizes for the different shm-objects
    ShmResourceStorageSizes CalculateShmResourceStorageSizes(SkeletonEventBindings& events,
                                                             SkeletonFieldBindings& fields) noexcept;
    /// \brief Calculates needed sizes for shm-objects for data and ctrl via simulation of allocations against a heap
    /// backed memory resource.
    /// \return storage sizes for the different shm-objects
    ShmResourceStorageSizes CalculateShmResourceStorageSizesBySimulation(SkeletonEventBindings& events,
                                                                         SkeletonFieldBindings& fields) noexcept;
    /// \brief Calculates needed sizes for shm-objects for data and ctrl via estimation based on sizeof info of related
    /// data types.
    /// \return storage sizes for the different shm-objects
    ShmResourceStorageSizes CalculateShmResourceStorageSizesByEstimation(SkeletonEventBindings& events,
                                                                         SkeletonFieldBindings& fields) const noexcept;

    void RemoveSharedMemory() noexcept;
    void RemoveStaleSharedMemoryArtefacts() const noexcept;

    InstanceIdentifier identifier_;

    amp::optional<std::string> data_storage_path_;
    amp::optional<std::string> data_control_qm_path_;
    amp::optional<std::string> data_control_asil_path_;
    ServiceDataStorage* storage_;
    ServiceDataControl* control_qm_;
    ServiceDataControl* control_asil_b_;
    std::shared_ptr<bmw::memory::shared::ManagedMemoryResource> storage_resource_;
    std::shared_ptr<bmw::memory::shared::ManagedMemoryResource> control_qm_resource_;
    std::shared_ptr<bmw::memory::shared::ManagedMemoryResource> control_asil_resource_;

    std::unique_ptr<IShmPathBuilder> shm_path_builder_;
    std::unique_ptr<IPartialRestartPathBuilder> partial_restart_path_builder_;
    amp::optional<memory::shared::LockFile> service_instance_existence_marker_file_;
    amp::optional<memory::shared::LockFile> service_instance_usage_marker_file_;

    std::unique_ptr<bmw::memory::shared::FlockMutexAndLock<bmw::memory::shared::ExclusiveFlockMutex>>
        service_instance_existence_flock_mutex_and_lock_;

    bool was_old_shm_region_reopened_;

    bmw::filesystem::Filesystem filesystem_;
};

namespace detail_skeleton
{

bool HasAsilBSupport(const InstanceIdentifier& identifier) noexcept;

}  // namespace detail_skeleton

template <typename SampleType>
std::pair<EventDataStorage<SampleType>*, EventDataControlComposite> Skeleton::Register(
    const ElementFqId element_fq_id,
    SkeletonEventProperties element_properties,
    amp::optional<impl::tracing::SkeletonEventTracingData> skeleton_event_tracing_data) noexcept
{
    // If the skeleton previously crashed and there are active proxies connected to the old shared memory, then we
    // re-open that shared memory in PrepareOffer(). In that case, we should retrieved the EventDataControl and
    // EventDataStorage from the shared memory and attempt to rollback the Skeleton tracing transaction log.
    if (was_old_shm_region_reopened_)
    {
        auto [typed_event_data_storage_ptr, event_data_control_composite] =
            OpenEventDataFromOpenedSharedMemory<SampleType>(element_fq_id);

        auto& event_data_control_qm = event_data_control_composite.GetQmEventDataControl();
        auto rollback_result = event_data_control_qm.GetTransactionLogSet().RollbackSkeletonTracingTransactions(
            [&event_data_control_qm](const TransactionLog::SlotIndexType slot_index) noexcept {
                event_data_control_qm.DereferenceEventWithoutTransactionLogging(slot_index);
            });
        if (!rollback_result.has_value())
        {
            ::bmw::mw::log::LogWarn("lola")
                << "SkeletonEvent: PrepareOffer failed: Could not rollback tracing consumer after "
                   "crash. Disabling tracing.";
            impl::Runtime::getInstance().GetTracingRuntime()->DisableTracing();
        }
        else
        {
            impl::tracing::RegisterTracingTransactionLog(skeleton_event_tracing_data, event_data_control_qm);
        }
        return {typed_event_data_storage_ptr, event_data_control_composite};
    }
    else
    {
        auto [typed_event_data_storage_ptr, event_data_control_composite] =
            CreateEventDataFromOpenedSharedMemory<SampleType>(element_fq_id, element_properties);

        auto& event_data_control_qm = event_data_control_composite.GetQmEventDataControl();
        impl::tracing::RegisterTracingTransactionLog(skeleton_event_tracing_data, event_data_control_qm);

        return {typed_event_data_storage_ptr, event_data_control_composite};
    }
}

template <typename SampleType>
std::pair<EventDataStorage<SampleType>*, EventDataControlComposite> Skeleton::OpenEventDataFromOpenedSharedMemory(
    const ElementFqId element_fq_id) noexcept
{
    auto find_element = [](auto& map, const ElementFqId& target_element_fq_id) noexcept -> auto {
        const auto it = map.find(target_element_fq_id);
        AMP_ASSERT_PRD_MESSAGE(it != map.cend(), "Could not find element fq id in map");
        return it;
    };

    amp::ignore = find_element(storage_->events_metainfo_, element_fq_id);
    const auto event_data_storage_it = find_element(storage_->events_, element_fq_id);
    const auto event_control_qm_it = find_element(control_qm_->event_controls_, element_fq_id);

    EventDataControl* event_data_control_asil_b{nullptr};
    if (detail_skeleton::HasAsilBSupport(identifier_))
    {
        const auto event_control_asil_b_it = find_element(control_asil_b_->event_controls_, element_fq_id);
        event_data_control_asil_b = &(event_control_asil_b_it->second.data_control);
    }

    auto* const typed_event_data_storage_ptr =
        static_cast<EventDataStorage<SampleType>*>(event_data_storage_it->second.get());
    AMP_ASSERT_PRD_MESSAGE(typed_event_data_storage_ptr != nullptr, "Could not cast void* to EventDataStorage*");

    return {typed_event_data_storage_ptr,
            // The lifetime of the "event_data_control_asil_b" object lasts as long as the Skeleton is alive.
            // 
            // 
            // 
            EventDataControlComposite{&event_control_qm_it->second.data_control, event_data_control_asil_b}};
}

template <typename SampleType>
std::pair<EventDataStorage<SampleType>*, EventDataControlComposite> Skeleton::CreateEventDataFromOpenedSharedMemory(
    const ElementFqId element_fq_id,
    const SkeletonEventProperties& element_properties) noexcept
{
    auto* typed_event_data_storage_ptr = storage_resource_->construct<EventDataStorage<SampleType>>(
        element_properties.number_of_slots, storage_resource_->getMemoryResourceProxy());

    auto inserted_data_slots = storage_->events_.emplace(std::piecewise_construct,
                                                         std::forward_as_tuple(element_fq_id),
                                                         std::forward_as_tuple(typed_event_data_storage_ptr));
    AMP_ASSERT_PRD_MESSAGE(inserted_data_slots.second, "Couldn't register/emplace event-storage in data-section.");

    constexpr DataTypeMetaInfo sample_meta_info{sizeof(SampleType), alignof(SampleType)};
    auto* event_data_raw_array = typed_event_data_storage_ptr->data();
    auto inserted_meta_info =
        storage_->events_metainfo_.emplace(std::piecewise_construct,
                                           std::forward_as_tuple(element_fq_id),
                                           std::forward_as_tuple(sample_meta_info, event_data_raw_array));
    AMP_ASSERT_PRD_MESSAGE(inserted_meta_info.second, "Couldn't register/emplace event-meta-info in data-section.");

    auto control_qm =
        control_qm_->event_controls_.emplace(std::piecewise_construct,
                                             std::forward_as_tuple(element_fq_id),
                                             std::forward_as_tuple(element_properties.number_of_slots,
                                                                   element_properties.max_subscribers,
                                                                   element_properties.enforce_max_samples,
                                                                   control_qm_resource_->getMemoryResourceProxy()));
    AMP_ASSERT_PRD_MESSAGE(control_qm.second, "Couldn't register/emplace event-meta-info in data-section.");

    EventDataControl* control_asil_result{nullptr};
    if (control_asil_resource_ != nullptr)
    {
        auto iterator = control_asil_b_->event_controls_.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(element_fq_id),
            std::forward_as_tuple(element_properties.number_of_slots,
                                  element_properties.max_subscribers,
                                  element_properties.enforce_max_samples,
                                  control_asil_resource_->getMemoryResourceProxy()));

        control_asil_result = &iterator.first->second.data_control;
    }
    // clang-format off
    // The lifetime of the "control_asil_result" object lasts as long as the Skeleton is alive.
    // 
    // 
    // 
    return {typed_event_data_storage_ptr, EventDataControlComposite{&control_qm.first->second.data_control, control_asil_result}};
    // clang-format on
}

}  // namespace lola
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_SKELETON_H
