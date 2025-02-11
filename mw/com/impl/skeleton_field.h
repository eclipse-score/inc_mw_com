// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#ifndef PLATFORM_AAS_MW_COM_IMPL_SKELETON_FIELD_H
#define PLATFORM_AAS_MW_COM_IMPL_SKELETON_FIELD_H

#include "platform/aas/mw/com/impl/plumbing/sample_allocatee_ptr.h"
#include "platform/aas/mw/com/impl/plumbing/skeleton_field_binding_factory.h"
#include "platform/aas/mw/com/impl/skeleton_event.h"
#include "platform/aas/mw/com/impl/skeleton_field_base.h"

#include "platform/aas/lib/result/result.h"
#include "platform/aas/mw/log/logging.h"

#include <amp_assert.hpp>
#include <amp_string_view.hpp>

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

template <typename SampleDataType>
class SkeletonField : public SkeletonFieldBase
{
  public:
    using FieldType = SampleDataType;

    SkeletonField(SkeletonBase& parent, const amp::string_view field_name);

    ~SkeletonField() override = default;

    SkeletonField(const SkeletonField&) = delete;
    SkeletonField& operator=(const SkeletonField&) & = delete;

    SkeletonField(SkeletonField&& other) noexcept;
    SkeletonField& operator=(SkeletonField&& other) & noexcept;

    /// \brief FieldType is allocated by the user and provided to the middleware to send. Dispatches to
    /// SkeletonEvent::Send()
    ///
    /// The initial value of the field must be set before PrepareOffer() is called. However, the actual value of the
    /// field cannot be set until the Skeleton has been set up via Skeleton::OfferService(). Therefore, we create a
    /// callback that will update the field value with sample_value which will be called in the first call to
    /// SkeletonFieldBase::PrepareOffer().
    ResultBlank Update(const FieldType& sample_value) noexcept;

    /// \brief FieldType is previously allocated by middleware and provided by the user to indicate that he is finished
    /// filling the provided pointer with live data. Dispatches to SkeletonEvent::Send()
    ResultBlank Update(SampleAllocateePtr<FieldType> sample) noexcept;

    /// \brief Allocates memory for FieldType for the user to fill it. This is especially necessary for Zero-Copy
    /// implementations. Dispatches to SkeletonEvent::Allocate()
    ///
    /// This function cannot be currently called to set the initial value of a field as the shared memory must be first
    /// set up in the Skeleton::PrepareOffer() before the user can obtain / use a SampleAllocateePtr.
    Result<SampleAllocateePtr<FieldType>> Allocate() noexcept;

  private:
    bool IsInitialValueSaved() const noexcept override { return initial_field_value_ != nullptr; }
    ResultBlank DoDeferredUpdate() noexcept override;

    /// \brief FieldType is allocated by the user and provided to the middleware to send. Dispatches to
    /// SkeletonEvent::Send()
    ResultBlank UpdateImpl(const FieldType& sample_value) noexcept;

    SkeletonEvent<FieldType>* GetTypedEvent() const noexcept;

    std::unique_ptr<FieldType> initial_field_value_;
};

template <typename SampleDataType>
SkeletonField<SampleDataType>::SkeletonField(SkeletonBase& parent, const amp::string_view field_name)
    : SkeletonFieldBase{parent,
                        field_name,
                        std::make_unique<SkeletonEvent<FieldType>>(
                            parent,
                            field_name,
                            SkeletonFieldBindingFactory<SampleDataType>::CreateEventBinding(
                                SkeletonBaseView{parent}.GetAssociatedInstanceIdentifier(),
                                parent,
                                field_name),
                            typename SkeletonEvent<FieldType>::PrivateConstructorEnabler{})},
      initial_field_value_{nullptr}
{
    SkeletonBaseView skeleton_base_view{parent};
    skeleton_base_view.RegisterField(field_name, *this);
}

template <typename SampleDataType>
SkeletonField<SampleDataType>::SkeletonField(SkeletonField&& other) noexcept
    : SkeletonFieldBase(std::move(other)), initial_field_value_{std::move(other.initial_field_value_)}
{
    // Since the address of this event has changed, we need update the address stored in the parent skeleton.
    SkeletonBaseView skeleton_base_view{skeleton_base_.get()};
    skeleton_base_view.UpdateField(field_name_, *this);
}

template <typename SampleDataType>
SkeletonField<SampleDataType>& SkeletonField<SampleDataType>::operator=(SkeletonField&& other) & noexcept
{
    if (this != &other)
    {
        SkeletonFieldBase::operator=(std::move(other));

        initial_field_value_ = std::move(other.initial_field_value_);

        // Since the address of this event has changed, we need update the address stored in the parent skeleton.
        SkeletonBaseView skeleton_base_view{skeleton_base_.get()};
        skeleton_base_view.UpdateField(field_name_, *this);
    }
    return *this;
}

/// \brief FieldType is allocated by the user and provided to the middleware to send. Dispatches to
/// SkeletonEvent::Send()
///
/// The initial value of the field must be set before PrepareOffer() is called. However, the actual value of the
/// field cannot be set until the Skeleton has been set up via Skeleton::OfferService(). Therefore, we create a
/// callback that will update the field value with sample_value which will be called in the first call to
/// SkeletonFieldBase::PrepareOffer().
template <typename SampleDataType>
ResultBlank SkeletonField<SampleDataType>::Update(const FieldType& sample_value) noexcept
{
    if (!was_prepare_offer_called_)
    {
        initial_field_value_ = std::make_unique<FieldType>(sample_value);
        return Blank{};
    }
    return UpdateImpl(sample_value);
}

/// \brief FieldType is previously allocated by middleware and provided by the user to indicate that he is finished
/// filling the provided pointer with live data. Dispatches to SkeletonEvent::Send()
template <typename SampleDataType>
ResultBlank SkeletonField<SampleDataType>::Update(SampleAllocateePtr<FieldType> sample) noexcept
{
    return GetTypedEvent()->Send(std::move(sample));
}

/// \brief Allocates memory for FieldType for the user to fill it. This is especially necessary for Zero-Copy
/// implementations. Dispatches to SkeletonEvent::Allocate()
///
/// This function cannot be currently called to set the initial value of a field as the shared memory must be first
/// set up in the Skeleton::PrepareOffer() before the user can obtain / use a SampleAllocateePtr.
template <typename SampleDataType>
Result<SampleAllocateePtr<SampleDataType>> SkeletonField<SampleDataType>::Allocate() noexcept
{
    // This check can be removed when  is implemented
    if (!was_prepare_offer_called_)
    {
        bmw::mw::log::LogWarn("lola") << "Lola currently doesn't support zero-copy Allocate() before OfferService() is "
                                         "called as the shared memory is not setup until OfferService() is called.";
        return MakeUnexpected(ComErrc::kBindingFailure);
    }
    return GetTypedEvent()->Allocate();
}

template <typename SampleDataType>
ResultBlank SkeletonField<SampleDataType>::DoDeferredUpdate() noexcept
{
    AMP_ASSERT_MESSAGE(initial_field_value_ != nullptr,
                       "Initial field value containing a value is a precondition for DoDeferredUpdate.");
    const auto update_result = UpdateImpl(*initial_field_value_);
    if (!update_result.has_value())
    {
        return update_result;
    }

    // If the Update call succeeded, then we can delete the initial value
    initial_field_value_.reset();
    return Blank{};
}

template <typename SampleDataType>
ResultBlank SkeletonField<SampleDataType>::UpdateImpl(const FieldType& sample_value) noexcept
{
    return GetTypedEvent()->Send(sample_value);
}

template <typename SampleDataType>
SkeletonEvent<SampleDataType>* SkeletonField<SampleDataType>::GetTypedEvent() const noexcept
{
    auto* const typed_event = dynamic_cast<SkeletonEvent<FieldType>*>(skeleton_event_dispatch_.get());
    AMP_ASSERT_PRD_MESSAGE(typed_event != nullptr, "Downcast to SkeletonEvent<FieldType> failed!");
    return typed_event;
}

}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_SKELETON_FIELD_H
