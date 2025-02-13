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


#ifndef PLATFORM_AAS_MW_COM_IMPL_SKELETON_EVENT_BASE_H
#define PLATFORM_AAS_MW_COM_IMPL_SKELETON_EVENT_BASE_H

#include "platform/aas/mw/com/impl/flag_owner.h"
#include "platform/aas/mw/com/impl/skeleton_event_binding.h"
#include "platform/aas/mw/com/impl/tracing/skeleton_event_tracing.h"
#include "platform/aas/mw/com/impl/tracing/skeleton_event_tracing_data.h"

#include "platform/aas/lib/result/result.h"

#include <amp_string_view.hpp>

#include <functional>
#include <memory>
#include <utility>

namespace bmw
{
namespace mw
{
namespace com
{
namespace impl
{

class SkeletonEventBaseView;
// False Positive: this is a normal forward declaration.
// Which is used to avoid cyclic dependency with skeleton_base.h
// 
class SkeletonBase;

class SkeletonEventBase
{
    friend SkeletonEventBaseView;

  public:
    SkeletonEventBase(SkeletonBase& skeleton_base,
                      const amp::string_view event_name,
                      std::unique_ptr<SkeletonEventBindingBase> binding)
        : binding_{std::move(binding)},
          skeleton_base_{skeleton_base},
          event_name_{event_name},
          tracing_data_{},
          service_offered_flag_{}
    {
    }

    virtual ~SkeletonEventBase() { Cleanup(); }

    SkeletonEventBase(const SkeletonEventBase&) = delete;
    SkeletonEventBase& operator=(const SkeletonEventBase&) & = delete;

    SkeletonEventBase(SkeletonEventBase&&) noexcept = default;
    SkeletonEventBase& operator=(SkeletonEventBase&& other) & noexcept
    {
        if (this != &other)
        {
            Cleanup();

            binding_ = std::move(other.binding_);
            service_offered_flag_ = std::move(other.service_offered_flag_);
        }
        return *this;
    }

    void UpdateSkeletonReference(SkeletonBase& skeleton_base) noexcept { skeleton_base_ = skeleton_base; }

    /// \brief Used to indicate that the event shall be available to consumer
    /// Performs binding independent functionality and then dispatches to the binding
    bmw::ResultBlank PrepareOffer() noexcept
    {
        const auto result = binding_->PrepareOffer();
        if (result.has_value())
        {
            service_offered_flag_.Set();
        }
        return result;
    }

    /// \brief Used to indicate that the event shall no longer be available to consumer
    /// Performs binding independent functionality and then dispatches to the binding
    void PrepareStopOffer() noexcept
    {
        if (service_offered_flag_.IsSet())
        {
            binding_->PrepareStopOffer();
            service_offered_flag_.Clear();
        }
    }

  protected:
    std::unique_ptr<SkeletonEventBindingBase> binding_;

    // The SkeletonEventBase must contain a reference to the SkeletonBase so that a SkeletonBase can call
    // UpdateSkeletonReference whenever it is moved to a new address. A SkeletonBase only has a reference to a
    // SkeletonEventBase, not a typed SkeletonEvent, which is why UpdateSkeletonReference has to be in this class
    // despite skeleton_base_ being used in the derived class, SkeletonEvent.
    std::reference_wrapper<SkeletonBase> skeleton_base_;
    amp::string_view event_name_;
    tracing::SkeletonEventTracingData tracing_data_;
    FlagOwner service_offered_flag_;

  private:
    /// \brief Perform required clean up operations when a SkeletonBase object is destroyed or overwritten (by the
    /// move assignment operator) Currently just dispatches to PrepareStopOffer(), however we provide for symmetry
    /// with SkeletonBase and to allow easy additions to the clean up functionality in future.
    void Cleanup() noexcept { PrepareStopOffer(); }
};

class SkeletonEventBaseView
{
  public:
    explicit SkeletonEventBaseView(SkeletonEventBase& skeleton_event_base) : skeleton_event_base_{skeleton_event_base}
    {
    }

    SkeletonEventBindingBase* GetBinding() { return skeleton_event_base_.binding_.get(); }

    const tracing::SkeletonEventTracingData& GetSkeletonEventTracing() { return skeleton_event_base_.tracing_data_; }

  private:
    SkeletonEventBase& skeleton_event_base_;
};

}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_SKELETON_EVENT_BASE_H
