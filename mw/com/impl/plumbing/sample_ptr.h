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


#ifndef PLATFORM_AAS_MW_COM_IMPL_PLUMBING_SAMPLE_PTR_H
#define PLATFORM_AAS_MW_COM_IMPL_PLUMBING_SAMPLE_PTR_H

#include "platform/aas/mw/com/impl/sample_reference_tracker.h"

#include "platform/aas/mw/com/impl/bindings/lola/sample_ptr.h"
#include "platform/aas/mw/com/impl/bindings/mock_binding/sample_ptr.h"

#include <amp_blank.hpp>
#include <amp_overload.hpp>

#include <utility>
#include <variant>

namespace bmw
{
namespace mw
{
namespace com
{
namespace impl
{

/// \brief Binding agnostic reference to a sample received from a proxy event binding.
///
/// The class resembles std::unique_ptr but does not allocate. Instead, all pointer types from all supported bindings
/// need to be added to the variant so that this class can hold such an instance.
///
/// \tparam SampleType Data type referenced by this pointer.
template <typename SampleType>
class SamplePtr final
{
  public:
    using pointer = const SampleType*;
    using element_type = const SampleType;

    /// Create an instance by taking ownership of one of the supported inner sample pointer types.
    ///
    /// \tparam SamplePtrType The type of the sample pointer. Needs to be one of the types listed in the variant holding
    ///                       the actual pointer.
    /// \param binding_sample_ptr The binding-specific sample pointer.
    template <typename SamplePtrType>
    SamplePtr(SamplePtrType&& binding_sample_ptr, SampleReferenceGuard reference_guard)
        : binding_sample_ptr_{std::forward<SamplePtrType>(binding_sample_ptr)},
          reference_guard_{std::move(reference_guard)}
    {
    }

    constexpr SamplePtr() noexcept : SamplePtr{amp::blank{}, SampleReferenceGuard{}} {}

    // NOLINTBEGIN(google-explicit-constructor): Explanation provided below.
    // Clang tidy deviation: Constructor is not made explicit to allow implicit creation of a SamplePtr from a nullptr.
    // This simplifies code in many places e.g. allowing the default argument for a SamplePtr function argument to be a
    // nullptr: void TestFunc(SamplePtr<T> ptr = nullptr)
    //  see above rationale for clang-tidy
    // 
    constexpr SamplePtr(std::nullptr_t /* ptr */) noexcept : SamplePtr() {}
    // NOLINTEND(google-explicit-constructor): see above for detailed explanation

    SamplePtr(const SamplePtr<SampleType>&) = delete;
    // 
    SamplePtr(SamplePtr<SampleType>&& other) noexcept
        : binding_sample_ptr_{std::move(other.binding_sample_ptr_)}, reference_guard_{std::move(other.reference_guard_)}
    {
        other.binding_sample_ptr_ = amp::blank{};
    }
    SamplePtr& operator=(const SamplePtr<SampleType>&) = delete;
    SamplePtr& operator=(SamplePtr<SampleType>&& other) & noexcept = default;

    SamplePtr& operator=(std::nullptr_t) noexcept
    {
        Reset();
        return *this;
    }

    ~SamplePtr() noexcept = default;

    pointer get() const noexcept
    {
        auto ptr_visitor = amp::overload(
            [](const lola::SamplePtr<SampleType>& lola_ptr) noexcept -> pointer { return lola_ptr.get(); },
            [](const mock_binding::SamplePtr<SampleType>& mock_ptr) noexcept -> pointer { return mock_ptr.get(); },
            // 
            [](const amp::blank&) noexcept -> pointer { return nullptr; });
        return std::visit(ptr_visitor, binding_sample_ptr_);
    }

    // 
    pointer Get() const noexcept { return get(); }

    /// \brief deref underlying managed object.
    ///
    /// Only enabled if the SampleType is not void
    /// \return ref of managed object.
    template <class T = SampleType, typename std::enable_if<!std::is_same<T, void>::value>::type* = nullptr>
    // 
    std::add_lvalue_reference_t<element_type> operator*() const noexcept
    {
        return *get();
    }

    // 
    pointer operator->() const noexcept { return get(); }

    explicit operator bool() const noexcept { return std::holds_alternative<amp::blank>(binding_sample_ptr_) == false; }

    void Swap(SamplePtr& other) noexcept
    {
        // Search for custom swap functions via ADL, and use std::swap if none are found.
        using std::swap;

        swap(binding_sample_ptr_, other.binding_sample_ptr_);
        swap(reference_guard_, other.reference_guard_);
    }

    void Reset(SamplePtr other = nullptr) noexcept { Swap(other); }

  private:
    std::variant<amp::blank, lola::SamplePtr<SampleType>, mock_binding::SamplePtr<SampleType>> binding_sample_ptr_;
    SampleReferenceGuard reference_guard_;
};

template <typename SampleType>
{
    return !static_cast<bool>(ptr);
}

template <typename SampleType>
// 
bool operator==(const SamplePtr<SampleType>& ptr, std::nullptr_t /* ptr */) noexcept
{
    return nullptr == ptr;
}

template <typename SampleType>
// 
bool operator!=(std::nullptr_t /* ptr */, const SamplePtr<SampleType>& ptr) noexcept
{
    return !(nullptr == ptr);
}

template <typename SampleType>
// 
bool operator!=(const SamplePtr<SampleType>& ptr, std::nullptr_t /* ptr */) noexcept
{
    return nullptr != ptr;
}

template <typename SampleType>
void swap(SamplePtr<SampleType>& lhs, SamplePtr<SampleType>& rhs) noexcept
{
    lhs.Swap(rhs);
}

}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_PLUMBING_SAMPLE_PTR_H
