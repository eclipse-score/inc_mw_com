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


#ifndef PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_SAMPLE_PTR_H
#define PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_SAMPLE_PTR_H

#include "platform/aas/mw/com/impl/bindings/lola/event_data_control.h"
#include "platform/aas/mw/com/impl/bindings/lola/slot_decrementer.h"

#include <memory>
#include <optional>
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

class TransactionLogSet;

/// \brief SamplePtr behaves as unique_ptr to a sample (event slot). User get access to a SamplePtr via GetNewSamples().
/// This is the LoLa binding specific SamplePtr, which holds a link to the underlying slot in shared memory.
template <typename SampleType>
class SamplePtr final
{
  public:
    using pointer = const SampleType*;
    using element_type = SampleType;

    /// \brief default ctor giving invalid SamplePtr (owning no managed object, invalid event slot)
    SamplePtr() noexcept : SamplePtr{nullptr} {}

    /// \brief ctor from nullptr_t also giving invalid SamplePtr like default ctor.
    
    explicit SamplePtr(std::nullptr_t /* ptr */) noexcept
        
        : managed_object_{nullptr}, slot_decrementer_{}
    {
    }

    /// \brief ctor creates valid SamplePtr from its members.
    /// \param ptr pointer to managed object
    /// \param event_data_ctrl event data control structure, which manages the underlying event/sample in shmem.
    /// \param slot_index index of event slot
    SamplePtr(pointer ptr,
              EventDataControl& event_data_ctrl,
              const EventDataControl::SlotIndexType slot_index,
              TransactionLogSet::TransactionLogIndex transaction_log_idx) noexcept
        : managed_object_{ptr}, slot_decrementer_{{&event_data_ctrl, slot_index, transaction_log_idx}}
    {
    }

    ~SamplePtr() noexcept = default;

    /// \brief assign nullptr.
    /// \return reference to nullptr assigned (invalid) SamplePtr
    SamplePtr& operator=(std::nullptr_t) & noexcept
    {
        managed_object_ = nullptr;
        slot_decrementer_ = {};
        return *this;
    }

    /// \brief SamplePtr is not copyable.
    SamplePtr& operator=(const SamplePtr& other) noexcept = delete;
    SamplePtr(const SamplePtr&) noexcept = delete;

    /// \brief SamplePtr is moveable.
    SamplePtr& operator=(SamplePtr&& other) noexcept = default;
    SamplePtr(SamplePtr&& other) noexcept = default;

    /// \brief returns managed object.
    /// \todo: Maybe remove later, if not used anymore by user facing wrappers.
    pointer get() const noexcept { return managed_object_; };

    /// \brief check validity.
    /// \return true, if SamplePtr owns a valid managed object
    
    explicit operator bool() const noexcept { return managed_object_ != nullptr; }
    

    /// \brief deref underlying managed object.
    ///
    /// Only enabled if the SampleType is not void
    /// \return ref of managed object.
    template <class T = SampleType, typename std::enable_if<!std::is_same<T, void>::value>::type* = nullptr>
    typename std::add_lvalue_reference<const SampleType>::type operator*() const noexcept
    {
        return *managed_object_;
    }

    /// \brief access managed object
    /// \return pointer to managed object
    pointer operator->() const noexcept { return managed_object_; }

  private:
    pointer managed_object_;
    std::optional<SlotDecrementer> slot_decrementer_;
};

}  // namespace lola
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw


#endif  // PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_SAMPLE_PTR_H
