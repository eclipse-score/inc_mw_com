// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#ifndef PLATFORM_AAS_MW_COM_IMPL_SKELETON_FIELD_BASE_H
#define PLATFORM_AAS_MW_COM_IMPL_SKELETON_FIELD_BASE_H

#include "platform/aas/mw/com/impl/com_error.h"
#include "platform/aas/mw/com/impl/skeleton_event_base.h"

#include "platform/aas/lib/result/result.h"
#include "platform/aas/mw/log/logging.h"

#include <amp_string_view.hpp>

#include <functional>
#include <memory>

namespace bmw
{
namespace mw
{
namespace com
{
namespace impl
{

class SkeletonFieldBaseView;
class SkeletonBase;

class SkeletonFieldBase
{
    friend SkeletonFieldBaseView;

  public:
    SkeletonFieldBase(SkeletonBase& skeleton_base,
                      const amp::string_view field_name,
                      std::unique_ptr<SkeletonEventBase> skeleton_event_base)
        : skeleton_event_dispatch_{std::move(skeleton_event_base)},
          was_prepare_offer_called_{false},
          skeleton_base_{skeleton_base},
          field_name_{field_name}
    {
    }

    virtual ~SkeletonFieldBase() = default;

    SkeletonFieldBase(const SkeletonFieldBase&) = delete;
    SkeletonFieldBase& operator=(const SkeletonFieldBase&) & = delete;

    SkeletonFieldBase(SkeletonFieldBase&&) noexcept = default;
    SkeletonFieldBase& operator=(SkeletonFieldBase&&) & noexcept = default;

    void UpdateSkeletonReference(SkeletonBase& skeleton_base) noexcept { skeleton_base_ = skeleton_base; }

    /// \brief Used to indicate that the field shall be available to consumer (e.g. binding specific preparation)
    ResultBlank PrepareOffer() noexcept
    {
        // If PrepareOffer() has not been called yet on this field, then we have to set the initial value immediately
        // after calling PrepareOffer() on the binding
        if (!was_prepare_offer_called_)
        {
            if (!IsInitialValueSaved())
            {
                bmw::mw::log::LogWarn("lola") << "Initial value must be set before offering field: " << field_name_;
                return MakeUnexpected(ComErrc::kFieldValueIsNotValid);
            }

            const auto prepare_offer_result = skeleton_event_dispatch_->PrepareOffer();
            if (!prepare_offer_result.has_value())
            {
                return prepare_offer_result;
            }

            const auto update_field_result = DoDeferredUpdate();

            // If we succesfully called PrepareOffer() and succesfully updated the field value, then set the
            // was_prepare_offer_called_ flag.
            if (update_field_result.has_value())
            {
                was_prepare_offer_called_ = true;
            }
            return update_field_result;
        }
        else
        {
            return skeleton_event_dispatch_->PrepareOffer();
        }
    }

    void PrepareStopOffer() noexcept { skeleton_event_dispatch_->PrepareStopOffer(); }

  protected:
    std::unique_ptr<SkeletonEventBase> skeleton_event_dispatch_;
    bool was_prepare_offer_called_;

    // The SkeletonFieldBase must contain a reference to the SkeletonBase so that a SkeletonBase can call
    // UpdateSkeletonReference whenever it is moved to a new address. A SkeletonBase only has a reference to a
    // SkeletonFieldBase, not a typed SkeletonField, which is why UpdateSkeletonReference has to be in this class
    // despite skeleton_base_ being used in the derived class, SkeletonField.
    std::reference_wrapper<SkeletonBase> skeleton_base_;
    amp::string_view field_name_;

  private:
    /// \brief Returns whether the initial value has been saved by the user to be used by DoDeferredUpdate
    virtual bool IsInitialValueSaved() const noexcept = 0;

    /// \brief Sets the initial value of the field.
    ///
    /// The existence of the value is a precondition of this function, so IsInitialValueSaved() should be checked before
    /// calling DoDeferredUpdate()
    virtual ResultBlank DoDeferredUpdate() noexcept = 0;
};

class SkeletonFieldBaseView
{
  public:
    explicit SkeletonFieldBaseView(SkeletonFieldBase& base) : base_{base} {}

    // A SkeletonField does not contain a SkeletonFieldBinding, as it dispatches to a SkeletonEvent at the binding
    // independent level. Instead, it consists of an event binding and (in the future when method support is
    // implemented) two method bindings.
    SkeletonEventBindingBase* GetEventBinding()
    {
        SkeletonEventBase& skeleton_event_base = *base_.skeleton_event_dispatch_;
        return SkeletonEventBaseView{skeleton_event_base}.GetBinding();
    }

    SkeletonEventBase& GetEventBase() { return *base_.skeleton_event_dispatch_; }

    const tracing::SkeletonEventTracingData& GetSkeletonEventTracing()
    {
        return SkeletonEventBaseView{*base_.skeleton_event_dispatch_}.GetSkeletonEventTracing();
    }

  private:
    SkeletonFieldBase& base_;
};

}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_SKELETON_FIELD_BASE_H
