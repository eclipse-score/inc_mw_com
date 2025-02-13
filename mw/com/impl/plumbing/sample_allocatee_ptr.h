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


#ifndef PLATFORM_AAS_MW_COM_IMPL_PLUMBING_SAMPLE_ALLOCATEE_PTR_H
#define PLATFORM_AAS_MW_COM_IMPL_PLUMBING_SAMPLE_ALLOCATEE_PTR_H

#include "platform/aas/mw/com/impl/bindings/lola/sample_allocatee_ptr.h"

#include <amp_blank.hpp>
#include <amp_overload.hpp>
#include <amp_variant.hpp>

#include <cstddef>
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

/// \brief Pointer to a data sample allocated by the Communication Management implementation (mimics std::unique_ptr)
///
/// \details We try to implement certain functionality that facades an std::unique_ptr, but some functionalities (e.g.
/// custom deleter) are not implemented since they would provoke error prone usage. Sticking with the case of a custom
/// deleter, since the memory is allocated by the middleware, we also need to ensure that we reclaim the memory. If this
/// is overwritten (or intercepted by the user), we will provoke a memory leak.
///
/// \pre Created by Allocate() call towards a specific event.
template <typename SampleType>
class SampleAllocateePtr
{
  public:
    /// \brief SampleType*, raw-pointer to the object managed by this SampleAllocateePtr
    using pointer = SampleType*;

    /// \brief The type of the object managed by this SampleAllocateePtr
    using element_type = SampleType;

    /// \brief Constructs a SampleAllocateePtr that owns nothing.
    constexpr SampleAllocateePtr() noexcept : SampleAllocateePtr{amp::blank{}} {}

    /// \brief Constructs a SampleAllocateePtr that owns nothing.

    // no copy due to unique ownership
    SampleAllocateePtr(const SampleAllocateePtr<SampleType>&) = delete;
    SampleAllocateePtr& operator=(const SampleAllocateePtr<SampleType>&) & = delete;

    /// \brief Constructs a SampleAllocateePtr by transferring ownership from other to *this.
    SampleAllocateePtr(SampleAllocateePtr<SampleType>&& other) noexcept : SampleAllocateePtr() { this->Swap(other); }

    /// \brief Move assignment operator. Transfers ownership from other to *this
    SampleAllocateePtr& operator=(SampleAllocateePtr<SampleType>&& other) & noexcept
    {
        this->Swap(other);
        return *this;
    }

    SampleAllocateePtr& operator=(std::nullptr_t) noexcept
    {
        reset();
        return *this;
    }

    ~SampleAllocateePtr() noexcept = default;

    /// \brief Replaces the managed object.
    void reset(const pointer ptr = pointer()) noexcept
    {
        auto internal_ptr = amp::get_if<lola::SampleAllocateePtr<SampleType>>(&internal_);
        if (internal_ptr != nullptr)
        {
            internal_ptr->reset(ptr);
        }
    }

    /// \brief Swaps the managed objects of *this and another SampleAllocateePtr object other.
    void Swap(SampleAllocateePtr<SampleType>& other) noexcept
    {
        // Search for custom swap functions via ADL, and use std::swap if none are found.
        using std::swap;
        swap(internal_, other.internal_);
    }

    /// \brief Returns a pointer to the managed object or nullptr if no object is owned.
    pointer Get() const noexcept
    {
        using ReturnType = pointer;

        auto visitor = amp::overload(
            [](const lola::SampleAllocateePtr<SampleType>& internal_ptr) -> ReturnType { return internal_ptr.get(); },
            [](const std::unique_ptr<SampleType>& internal_ptr) -> ReturnType { return internal_ptr.get(); },
            // 
            [](const amp::blank&) -> ReturnType { return nullptr; });

        return amp::visit(visitor, internal_);
    }

    /// \brief Checks whether *this owns an object, i.e. whether Get() != nullptr
    /// \return true if *this owns an object, false otherwise.
    explicit operator bool() const noexcept
    {
        auto visitor = amp::overload(
            [](const lola::SampleAllocateePtr<SampleType>& internal_ptr) -> bool {
                return static_cast<bool>(internal_ptr);
            },
            [](const std::unique_ptr<SampleType>& internal_ptr) -> bool { return static_cast<bool>(internal_ptr); },
            // 
            [](const amp::blank&) -> bool { return false; });

        return amp::visit(visitor, internal_);
    }

    /// \brief operator* and operator-> provide access to the object owned by *this. If no object is hold, will
    /// terminate.
    typename std::add_lvalue_reference<SampleType>::type operator*() const noexcept(noexcept(*std::declval<pointer>()))
    {
        using ReturnType = typename std::add_lvalue_reference<SampleType>::type;

        auto visitor = amp::overload(
            [](const lola::SampleAllocateePtr<SampleType>& internal_ptr) -> ReturnType { return *internal_ptr; },
            [](const std::unique_ptr<SampleType>& internal_ptr) -> ReturnType { return *internal_ptr; },
            // 
            [](const amp::blank&) -> ReturnType { std::terminate(); });

        return amp::visit(visitor, internal_);
    }

    /// \brief operator* and operator-> provide access to the object owned by *this. If no object is hold, will
    /// terminate.
    pointer operator->() const noexcept
    {
        using ReturnType = pointer;

        auto visitor = amp::overload(
            [](const lola::SampleAllocateePtr<SampleType>& internal_ptr) -> ReturnType { return internal_ptr.get(); },
            [](const std::unique_ptr<SampleType>& internal_ptr) -> ReturnType { return internal_ptr.get(); },
            // 
            [](const amp::blank&) -> ReturnType { std::terminate(); });

        return amp::visit(visitor, internal_);
    }

  private:
    template <typename T>
    explicit SampleAllocateePtr(T ptr) : internal_{std::move(ptr)}
    {
    }

    
    template <typename T>
    // 
    friend auto MakeSampleAllocateePtr(T ptr) noexcept -> SampleAllocateePtr<typename T::element_type>;

    template <typename T>
    // 
    friend class SampleAllocateePtrView;

    template <typename T>
    // 
    friend class SampleAllocateePtrMutableView;

    // We don't use the pimpl idiom because it would require dynamic memory allocation (that we want to avoid)
    amp::variant<amp::blank, lola::SampleAllocateePtr<SampleType>, std::unique_ptr<SampleType>> internal_;
};

/// \brief Compares the pointer values of two SampleAllocateePtr, or a SampleAllocateePtr and nullptr
template <class T1, class T2>
bool operator==(const SampleAllocateePtr<T1>& lhs, const SampleAllocateePtr<T2>& rhs) noexcept
{
    return lhs.Get() == rhs.Get();
}

/// \brief Compares the pointer values of two SampleAllocateePtr, or a SampleAllocateePtr and nullptr
template <class T1, class T2>
bool operator!=(const SampleAllocateePtr<T1>& lhs, const SampleAllocateePtr<T2>& rhs) noexcept
{
    return lhs.Get() != rhs.Get();
}

/// \brief Specializes the std::swap algorithm for SampleAllocateePtr. Swaps the contents of lhs and rhs. Calls
/// lhs.swap(rhs)
template <class T>
void swap(SampleAllocateePtr<T>& lhs, SampleAllocateePtr<T>& rhs) noexcept
{
    lhs.swap(rhs);
}

/// \brief Helper function to create a SampleAllocateePtr within the middleware (not to be used by the user)
template <typename T>
auto MakeSampleAllocateePtr(T ptr) noexcept -> SampleAllocateePtr<typename T::element_type>
{
    return SampleAllocateePtr<typename T::element_type>{std::move(ptr)};
}

/// \brief SampleAllocateePtr is user facing, in order to interact with its internals we provide a view towards it
template <typename SampleType>
class SampleAllocateePtrView
{
  public:
    explicit SampleAllocateePtrView(const SampleAllocateePtr<SampleType>& ptr) : ptr_{ptr} {}

    /// \brief Interpret the binding independent SampleAllocateePtr as a binding specific one
    /// \return pointer to the underlying binding specific type, nullptr if this type is not the expected one
    template <typename T>
    const T* As() const noexcept
    {
        return amp::get_if<T>(&ptr_.internal_);
    }

    const amp::variant<amp::blank, lola::SampleAllocateePtr<SampleType>, std::unique_ptr<SampleType>>&
    GetUnderlyingVariant() const noexcept
    {
        return ptr_.internal_;
    }

  private:
    const impl::SampleAllocateePtr<SampleType>& ptr_;
};

/// \brief SampleAllocateePtr is user facing, in order to interact with its internals we provide a view towards it
template <typename SampleType>
class SampleAllocateePtrMutableView
{
  public:
    explicit SampleAllocateePtrMutableView(SampleAllocateePtr<SampleType>& ptr) : ptr_{ptr} {}

    amp::variant<amp::blank, lola::SampleAllocateePtr<SampleType>, std::unique_ptr<SampleType>>&
    GetUnderlyingVariant() noexcept
    {
        return ptr_.internal_;
    }

  private:
    impl::SampleAllocateePtr<SampleType>& ptr_;
};

}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_PLUMBING_SAMPLE_ALLOCATEE_PTR_H
