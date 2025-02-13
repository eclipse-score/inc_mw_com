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


#ifndef PLATFORM_AAS_MW_COM_IMPL_SKELETON_BASE_H
#define PLATFORM_AAS_MW_COM_IMPL_SKELETON_BASE_H

#include "platform/aas/mw/com/impl/flag_owner.h"
#include "platform/aas/mw/com/impl/instance_identifier.h"
#include "platform/aas/mw/com/impl/instance_specifier.h"
#include "platform/aas/mw/com/impl/skeleton_binding.h"
#include "platform/aas/mw/com/impl/skeleton_event_binding.h"

#include "platform/aas/lib/memory/string_literal.h"
#include "platform/aas/lib/result/result.h"

#include <amp_assert.hpp>
#include <amp_optional.hpp>
#include <amp_span.hpp>
#include <amp_string_view.hpp>

#include <cstdint>
#include <functional>
#include <map>
#include <memory>

namespace bmw::mw::com::impl
{

class SkeletonEventBase;
class SkeletonFieldBase;

/// \brief Defines the processing modes for the service implementation side.
///
/// \requirement 
///
enum class MethodCallProcessingMode : std::uint8_t
{
    kPoll,
    kEvent,
    kEventSingleThread
};

/// \brief Parent class for all generated skeletons. Only the generated skeletons will be user facing. But in order to
/// reduce code duplication, we encapsulate the common logic in here.
class SkeletonBase
{
  public:
    using EventNameList = amp::span<const bmw::StringLiteral>;
    // An std::map/ordered map is NEEDED here as we require deterministic order of elements in the map, when iterating
    // over it repeatedly! A hint, that our shared-memory-size-calculation relies on it!
    using SkeletonEvents = std::map<amp::string_view, std::reference_wrapper<SkeletonEventBase>>;
    using SkeletonFields = std::map<amp::string_view, std::reference_wrapper<SkeletonFieldBase>>;

    /// \brief Creation of service skeleton with provided Skeleton binding
    ///
    /// \requirement 
    ///
    /// \param skeleton_binding The SkeletonBinding which is created using SkeletonBindingFactory.
    /// \param instance_id The instance identifier which uniquely identifies this Skeleton instance.
    /// \param mode As a default argument, this is the mode of the service implementation for processing service method
    /// invocations with kEvent as default value. See [] for the type definition and [] for more
    /// details on the behavior
    explicit SkeletonBase(std::unique_ptr<SkeletonBinding> skeleton_binding,
                          const InstanceIdentifier instance_id,
                          MethodCallProcessingMode mode = MethodCallProcessingMode::kEvent);

    virtual ~SkeletonBase();

    /// \brief A Skeleton shall not be copyable
    /// \requirement 
    SkeletonBase(const SkeletonBase&) = delete;
    SkeletonBase& operator=(const SkeletonBase&) = delete;

    /// \brief A Skeleton shall be movable
    /// \requirement 
    SkeletonBase(SkeletonBase&& other) noexcept;
    SkeletonBase& operator=(SkeletonBase&& other) noexcept;

    /// \brief Offer the respective service to other applications
    ///
    /// \return On failure, returns an error code according to the SW Component Requirements 8 and
    /// .
    [[nodiscard]] ResultBlank OfferService() noexcept;

    /// \brief Stops offering the respective service to other applications
    ///
    /// \requirement 
    void StopOfferService() noexcept;

  protected:
    bool AreBindingsValid() const noexcept;

    friend class SkeletonBaseView;
    std::unique_ptr<SkeletonBinding> binding_;
    SkeletonEvents events_;
    SkeletonFields fields_;
    InstanceIdentifier instance_id_;

  private:
    /// \brief Perform required clean up operations when a SkeletonBase object is destroyed or overwritten (by the move
    /// assignment operator)
    void Cleanup();

    [[nodiscard]] bmw::ResultBlank OfferServiceEvents() const noexcept;
    [[nodiscard]] bmw::ResultBlank OfferServiceFields() const noexcept;

    FlagOwner service_offered_flag_;
};

class SkeletonBaseView
{
  public:
    explicit SkeletonBaseView(SkeletonBase& skeleton_base) : skeleton_base_{skeleton_base} {}

    InstanceIdentifier GetAssociatedInstanceIdentifier() const { return skeleton_base_.instance_id_; }

    SkeletonBinding* GetBinding() const { return skeleton_base_.binding_.get(); }

    void RegisterEvent(const amp::string_view event_name, SkeletonEventBase& event)
    {
        const auto result = skeleton_base_.events_.emplace(event_name, event);
        const bool was_event_inserted = result.second;
        AMP_ASSERT_MESSAGE(was_event_inserted, "Event cannot be registered as it already exists.");
    }

    void RegisterField(const amp::string_view field_name, SkeletonFieldBase& field)
    {
        const auto result = skeleton_base_.fields_.emplace(field_name, field);
        const bool was_field_inserted = result.second;
        AMP_ASSERT_MESSAGE(was_field_inserted, "Field cannot be registered as it already exists.");
    }

    void UpdateEvent(const amp::string_view event_name, SkeletonEventBase& event)
    {
        skeleton_base_.events_.at(event_name) = event;
    }

    void UpdateField(const amp::string_view field_name, SkeletonFieldBase& field)
    {
        skeleton_base_.fields_.at(field_name) = field;
    }

    const SkeletonBase::SkeletonEvents& GetEvents() const noexcept { return skeleton_base_.events_; }

    const SkeletonBase::SkeletonFields& GetFields() const noexcept { return skeleton_base_.fields_; }

  private:
    SkeletonBase& skeleton_base_;
};

amp::optional<InstanceIdentifier> GetInstanceIdentifier(const InstanceSpecifier& specifier);

}  // namespace bmw::mw::com::impl

#endif  // PLATFORM_AAS_MW_COM_IMPL_SKELETON_BASE_H
