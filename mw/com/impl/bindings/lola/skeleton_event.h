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


#ifndef PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_SKELETON_EVENT_H
#define PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_SKELETON_EVENT_H

#include "platform/aas/mw/com/impl/bindings/lola/element_fq_id.h"
#include "platform/aas/mw/com/impl/bindings/lola/event_data_control_composite.h"
#include "platform/aas/mw/com/impl/bindings/lola/event_data_storage.h"
#include "platform/aas/mw/com/impl/bindings/lola/i_runtime.h"
#include "platform/aas/mw/com/impl/bindings/lola/sample_allocatee_ptr.h"
#include "platform/aas/mw/com/impl/bindings/lola/skeleton.h"
#include "platform/aas/mw/com/impl/bindings/lola/skeleton_event_properties.h"
#include "platform/aas/mw/com/impl/plumbing/sample_allocatee_ptr.h"
#include "platform/aas/mw/com/impl/runtime.h"
#include "platform/aas/mw/com/impl/skeleton_event_binding.h"
#include "platform/aas/mw/com/impl/tracing/skeleton_event_tracing.h"
#include "platform/aas/mw/com/impl/tracing/skeleton_event_tracing_data.h"

#include "platform/aas/mw/log/logging.h"

#include <amp_assert.hpp>
#include <amp_optional.hpp>
#include <amp_string_view.hpp>

#include <mutex>
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

/// \brief Represents a binding specific instance (LoLa) of an event within a skeleton. It can be used to send events
/// via Shared Memory. It will be created via a Factory Method, that will instantiate this class based on deployment
/// values.
///
/// This class is _not_ user-facing.
///
/// All operations on this class are _not_ thread-safe, in a manner that they shall not be invoked in parallel by
/// different threads.
template <typename SampleType>
class SkeletonEvent final : public SkeletonEventBinding<SampleType>
{
    template <typename T>
    friend class SkeletonEventAttorney;

  public:
    using typename SkeletonEventBinding<SampleType>::SendTraceCallback;
    using typename SkeletonEventBindingBase::SubscribeTraceCallback;
    using typename SkeletonEventBindingBase::UnsubscribeTraceCallback;

    SkeletonEvent(Skeleton& parent,
                  const ElementFqId event_fqn,
                  const amp::string_view event_name,
                  const SkeletonEventProperties properties,
                  amp::optional<impl::tracing::SkeletonEventTracingData> skeleton_event_tracing_data = {}) noexcept;

    SkeletonEvent(const SkeletonEvent&) = delete;
    SkeletonEvent(SkeletonEvent&&) noexcept = delete;
    SkeletonEvent& operator=(const SkeletonEvent&) & = delete;
    SkeletonEvent& operator=(SkeletonEvent&&) & noexcept = delete;

    ~SkeletonEvent() override = default;

    /// \brief Sends a value by _copy_ towards a consumer. It will allocate the necessary space and then copy the value
    /// into Shared Memory.
    ResultBlank Send(const SampleType& value, amp::optional<SendTraceCallback> send_trace_callback) noexcept override;

    ResultBlank Send(impl::SampleAllocateePtr<SampleType> sample,
                     amp::optional<SendTraceCallback> send_trace_callback) noexcept override;

    Result<impl::SampleAllocateePtr<SampleType>> Allocate() noexcept override;

    /// @requirement 
    ResultBlank PrepareOffer() noexcept override;

    void PrepareStopOffer() noexcept override;

    BindingType GetBindingType() const noexcept override { return BindingType::kLoLa; }

    void SetSkeletonEventTracingData(impl::tracing::SkeletonEventTracingData tracing_data) noexcept override
    {
        skeleton_event_tracing_data_ = tracing_data;
    }

    ElementFqId GetElementFQId() const noexcept { return event_fqn_; };

  private:
    static lola::IRuntime& GetLoLaRuntime();

    Skeleton& parent_;
    const ElementFqId event_fqn_;
    const amp::string_view event_name_;
    const SkeletonEventProperties event_properties_;
    EventDataStorage<SampleType>* event_data_storage_;
    amp::optional<EventDataControlComposite> event_data_control_composite_;
    EventSlotStatus::EventTimeStamp current_timestamp_;
    bool qm_disconnect_;
    amp::optional<impl::tracing::SkeletonEventTracingData> skeleton_event_tracing_data_;
};

template <typename SampleType>
SkeletonEvent<SampleType>::SkeletonEvent(
    Skeleton& parent,
    const ElementFqId event_fqn,
    const amp::string_view event_name,
    const SkeletonEventProperties properties,
    amp::optional<impl::tracing::SkeletonEventTracingData> skeleton_event_tracing_data) noexcept
    : SkeletonEventBinding<SampleType>{},
      parent_{parent},
      event_fqn_{event_fqn},
      event_name_{event_name},
      event_properties_{properties},
      event_data_storage_{nullptr},
      event_data_control_composite_{amp::nullopt},
      current_timestamp_{1},
      qm_disconnect_{false},
      skeleton_event_tracing_data_{skeleton_event_tracing_data}
{
}

template <typename SampleType>
ResultBlank SkeletonEvent<SampleType>::Send(const SampleType& value,
                                            amp::optional<SendTraceCallback> send_trace_callback) noexcept
{
    auto allocated_slot_result = Allocate();
    if (!(allocated_slot_result.has_value()))
    {
        return MakeUnexpected(ComErrc::kSampleAllocationFailure, "Could not allocate slot");
    }
    auto allocated_slot = std::move(allocated_slot_result).value();
    *allocated_slot = value;

    return Send(std::move(allocated_slot), std::move(send_trace_callback));
}

template <typename SampleType>
ResultBlank SkeletonEvent<SampleType>::Send(impl::SampleAllocateePtr<SampleType> sample,
                                            amp::optional<SendTraceCallback> send_trace_callback) noexcept
{
    const impl::SampleAllocateePtrView<SampleType> view{sample};
    auto ptr = view.template As<lola::SampleAllocateePtr<SampleType>>();
    AMP_ASSERT_PRD(nullptr != ptr);
    auto slot = ptr->GetReferencedSlot();
    ++current_timestamp_;
    event_data_control_composite_->EventReady(slot, current_timestamp_);

    if (send_trace_callback.has_value())
    {
        (*send_trace_callback)(sample);
    }
    if (!qm_disconnect_)
    {
        GetLoLaRuntime().GetLolaMessaging().NotifyEvent(QualityType::kASIL_QM, event_fqn_);
    }
    if (parent_.GetInstanceQualityType() == QualityType::kASIL_B)
    {
        GetLoLaRuntime().GetLolaMessaging().NotifyEvent(QualityType::kASIL_B, event_fqn_);
    }
    return {};
}

template <typename SampleType>
Result<impl::SampleAllocateePtr<SampleType>> SkeletonEvent<SampleType>::Allocate() noexcept
{
    if (event_data_control_composite_.has_value() == false)
    {
        ::bmw::mw::log::LogError("lola") << "Tried to allocate event, but the EventDataControl does not exist!";
        return MakeUnexpected(ComErrc::kBindingFailure);
    }
    const auto slot = event_data_control_composite_->AllocateNextSlot();

    if ((qm_disconnect_ == false) && slot.second)
    {
        qm_disconnect_ = true;
        bmw::mw::log::LogWarn("lola")
            << __func__ << __LINE__
            << "Disconnecting unsafe QM consumers as slot allocation failed on an ASIL-B enabled event: " << event_fqn_;
        parent_.DisconnectQmConsumers();
    }

    if (slot.first.has_value())
    {
        return MakeSampleAllocateePtr(SampleAllocateePtr<SampleType>(
            &event_data_storage_->at(slot.first.value()), *event_data_control_composite_, slot.first.value()));
    }
    else
    {
        // we didn't get a slot! This is a contract violation.
        // @todo Shall we call std::terminate here or shall this be done by a higher layer?
        if (event_properties_.enforce_max_samples == false)
        {
            ::bmw::mw::log::LogError("lola")
                << "SkeletonEvent: Allocation of event slot failed. Hint: enforceMaxSamples was "
                   "disabled by config. Might be the root cause!";
        }
        return MakeUnexpected(ComErrc::kBindingFailure);
    }
}

template <typename SampleType>
ResultBlank SkeletonEvent<SampleType>::PrepareOffer() noexcept
{
    std::tie(event_data_storage_, event_data_control_composite_) =
        parent_.Register<SampleType>(event_fqn_, event_properties_, skeleton_event_tracing_data_);

    AMP_ASSERT_PRD_MESSAGE(event_data_control_composite_.has_value(),
                           "Defensive programming as event_data_control_composite_ is set by Register above.");
    current_timestamp_ = event_data_control_composite_.value().GetLatestTimestamp();
    return {};
}

template <typename SampleType>
void SkeletonEvent<SampleType>::PrepareStopOffer() noexcept
{
    if (event_data_control_composite_.has_value())
    {
        impl::tracing::UnRegisterTracingTransactionLog(event_data_control_composite_.value().GetQmEventDataControl());
    }
}

template <typename SampleType>
lola::IRuntime& SkeletonEvent<SampleType>::GetLoLaRuntime()
{
    auto* const lola_runtime =
        dynamic_cast<lola::IRuntime*>(mw::com::impl::Runtime::getInstance().GetBindingRuntime(BindingType::kLoLa));
    if (lola_runtime == nullptr)
    {
        ::bmw::mw::log::LogFatal("lola") << "SkeletonEvent: No lola runtime available.";
        std::terminate();
    }
    return *lola_runtime;
}

}  // namespace lola
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_SKELETON_EVENT_H
