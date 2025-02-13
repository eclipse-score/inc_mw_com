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


#ifndef PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_SAMPLE_ALLOCATEE_PTR_H
#define PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_SAMPLE_ALLOCATEE_PTR_H

#include "platform/aas/mw/com/impl/bindings/lola/event_data_control_composite.h"

#include <amp_optional.hpp>

#include <limits>
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
namespace lola
{

/// \brief SampleAllocateePtr behaves as unique_ptr to an allocated sample (event slot). A user might manipulate the
/// value of the underlying pointer in any regard. If the value shall be transmitted to any consumer Send() most be
/// invoked.
/// If the pointer gets destroyed without invoking Send(), the changed data will be lost.
///
/// This type should not be created on its own, rather it shall be created by an Allocate() call towards an event. In
/// any case this type is the binding specific representation of an SampleAllocateePtr.
template <typename SampleType>
class SampleAllocateePtr
{
    template <typename T>
    friend class SampleAllocateePtrView;
    template <typename T>
    friend class SampleAllocateePtrMutableView;

  public:
    using pointer = SampleType*;
    using const_pointer = SampleType* const;
    using element_type = SampleType;

    /// \brief default ctor giving invalid SampleAllocateePtr (owning no managed object, invalid event slot)
    explicit SampleAllocateePtr() noexcept
        : managed_object_{nullptr},
          event_slot_index_{std::numeric_limits<EventDataControl::SlotIndexType>::max()},
          event_data_control_{amp::nullopt} {};

    /// \brief ctor from nullptr_t also giving invalid SampleAllocateePtr like default ctor.
    
        : managed_object_{nullptr},
          event_slot_index_{std::numeric_limits<EventDataControl::SlotIndexType>::max()},
          event_data_control_{amp::nullopt} {};
    

    /// \brief ctor creates valid SampleAllocateePtr from its members.
    /// \param ptr pointer to managed object
    /// \param event_data_ctrl event data control structure, which manages the underlying event/sample in shmem.
    /// \param slot_index index of event slot
    explicit SampleAllocateePtr(pointer ptr,
                                const EventDataControlComposite& event_data_ctrl,
                                const EventDataControl::SlotIndexType slot_index) noexcept
        : managed_object_{ptr}, event_slot_index_{slot_index}, event_data_control_{event_data_ctrl} {};

    /// \brief SampleAllocateePtr is not copyable.
    SampleAllocateePtr(const SampleAllocateePtr&) = delete;

    /// \brief SampleAllocateePtr is movable.
    
    /* (1) False positive: Init order doesn't apply to move constructor. (2) False positive: Rule only applies for
     * initializing data members or base classes. In this case, "other" is an lvalue passed to swap as a non-const
     * reference. Therefore, no copies will be made. */
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init): all members initialized by default constructor
    SampleAllocateePtr(SampleAllocateePtr&& other) noexcept : SampleAllocateePtr() { this->swap(other); };
    

    /// \brief dtor discards underlying event in event slot (if we have a valid event slot)
    ~SampleAllocateePtr() noexcept { internal_delete(); };

    /// \brief returns managed object.
    /// \todo: Maybe remove later, if not used anymore by user facing wrappers.
    
    pointer get() const noexcept { return managed_object_; };
    

    /// \brief reset managed object and eventually discard underlying event slot.
    
    /* (1) Reusing function name reset() to match pointer interface.
       (2) nullpointr parameter is left unnamed intentionally */
    void reset(std::nullptr_t /* ptr */ = nullptr) noexcept { internal_delete(); }
    [[deprecated("SPP_DEPRECATION: reset shall not be used (will be also removed from user facing interface).")]] void
    reset(const_pointer p)
    

    /// \brief swap content with _other_
    void swap(SampleAllocateePtr& other) noexcept
    {
        // Search for custom swap functions via ADL, and use std::swap if none are found.
        using std::swap;

        swap(this->managed_object_, other.managed_object_);
        swap(this->event_slot_index_, other.event_slot_index_);
        swap(this->event_data_control_, other.event_data_control_);
    }

    /// \brief check validity.
    /// \return true, if SampleAllocateePtr owns a valid managed object
    
    explicit operator bool() const noexcept { return managed_object_ != nullptr; }
    

    /// \brief deref underlying managed object.
    /// \return ref of managed object.
    typename std::add_lvalue_reference<SampleType>::type operator*() const noexcept { return *managed_object_; }
    /// \brief access managed object
    /// \return pointer to managed object
    
    pointer operator->() const noexcept { return managed_object_; }
    

    /// \brief assign nullptr.
    /// \return reference to nullptr assigned (invalid) SampleAllocateePtr
    SampleAllocateePtr& operator=(std::nullptr_t /* ptr */) & noexcept
    {
        internal_delete();
        return *this;
    }
    /// \brief SampleAllocateePtr is not copy assignable.
    SampleAllocateePtr& operator=(const SampleAllocateePtr& other) & = delete;

    /// \brief SampleAllocateePtr is move assignable.
    SampleAllocateePtr& operator=(SampleAllocateePtr&& other) & noexcept
    {
        this->swap(other);
        return *this;
    };

    /// \brief access to nternal slot index
    /// \return slot index of underlying shmem event slot.
    EventDataControl::SlotIndexType GetReferencedSlot() const noexcept { return event_slot_index_; };

  private:
    void internal_delete()
    {
        managed_object_ = nullptr;
        if (event_slot_index_ < std::numeric_limits<EventDataControl::SlotIndexType>::max())
        {
            if (event_data_control_.has_value())
            {
                event_data_control_->Discard(event_slot_index_);
            }
            event_slot_index_ = std::numeric_limits<EventDataControl::SlotIndexType>::max();
        }
    }

    pointer managed_object_;
    EventDataControl::SlotIndexType event_slot_index_;
    
    amp::optional<EventDataControlComposite> event_data_control_;
    
};

/// \brief Specializes the std::swap algorithm for SampleAllocateePtr. Swaps the contents of lhs and rhs. Calls
/// lhs.swap(rhs)
template <typename SampleType>

void swap(SampleAllocateePtr<SampleType>& lhs, SampleAllocateePtr<SampleType>& rhs) noexcept

{
    lhs.swap(rhs);
}

/// \brief SampleAllocateePtr is user facing, in order to interact with its internals we provide a view towards it
template <typename SampleType>
class SampleAllocateePtrView
{
  public:
    explicit SampleAllocateePtrView(const SampleAllocateePtr<SampleType>& ptr) : ptr_{ptr} {}

    const amp::optional<EventDataControlComposite>& GetEventDataControlComposite() const noexcept
    {
        return ptr_.event_data_control_;
    }

    typename SampleAllocateePtr<SampleType>::pointer GetManagedObject() const noexcept { return ptr_.managed_object_; }

  private:
    const SampleAllocateePtr<SampleType>& ptr_;
};

/// \brief SampleAllocateePtr is user facing, in order to interact with its internals we provide a view towards it
template <typename SampleType>
class SampleAllocateePtrMutableView
{
  public:
    explicit SampleAllocateePtrMutableView(SampleAllocateePtr<SampleType>& ptr) : ptr_{ptr} {}

    const amp::optional<EventDataControlComposite>& GetEventDataControlComposite() const noexcept
    {
        return ptr_.event_data_control_;
    }

  private:
    SampleAllocateePtr<SampleType>& ptr_;
};

}  // namespace lola
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw


#endif  // PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_SAMPLE_ALLOCATEE_PTR_H
