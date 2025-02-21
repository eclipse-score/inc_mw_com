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


#ifndef PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_ELEMENT_FQ_ID_H
#define PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_ELEMENT_FQ_ID_H

#include <cstdint>
#include <functional>
#include <string>
#include <type_traits>

namespace bmw::mw
{
namespace log
{
class LogStream;
}  // namespace log

namespace com::impl::lola
{

/// \brief enum used to differentiate between difference service element types
enum class ElementType : std::uint8_t
{
    INVALID = 0,
    EVENT,
    FIELD
};

/// \brief unique identification of a service element (event, field, method) instance within one bmw::mw runtime/process
///
/// \detail Identification consists of the four dimensions: Service-Type (service_id), instance of service
///         (instance_id), the id of the element (element_id) within this service and an enum which tracks the type of
///         the element. The first two (service_id, element_id) are defined at generation time (ara_gen). The
///         instance_id is a deployment/runtime parameter.

// Keep struct type even if it breaks the rule A11-0-2 that says: struct must not provide any special member functions
// or methods (such as user defined constructors, etc). Public access to members is needed and special member functions
// don't break ability to trivial copy it. The trivial copiness is statically checked below is safe to be struct and can
// be trivially copied even providing special member functions.
// NOLINTNEXTLINE(bmw-struct-usage-compliance): Intended struct semantic.
struct ElementFqId
{
    /// \brief default ctor initializing all members to their related max value.
    /// \details constructs an "invalid" ElementFqId.
    ElementFqId() noexcept;
    ElementFqId(const std::uint16_t service_id,
                const std::uint8_t element_id,
                const std::uint16_t instance_id,
                const std::uint8_t element_type) noexcept;

    ElementFqId(const std::uint16_t service_id,
                const std::uint8_t element_id,
                const std::uint16_t instance_id,
                const ElementType element_type) noexcept;

    std::string ToString() const noexcept;

    std::uint16_t service_id_;
    std::uint8_t element_id_;
    std::uint16_t instance_id_;
    ElementType element_type_;
};
// guarantee memcpy usage
static_assert(std::is_trivially_copyable<ElementFqId>::value,
              "ElementFqId must be trivially copyable to perform memcpy op");

bool IsElementEvent(const ElementFqId& element_fq_id) noexcept;
bool IsElementField(const ElementFqId& element_fq_id) noexcept;

// Note. Equality / comparison operators do not use ElementType since the other 3 elements already uniquely identify a
// service element.

bool operator==(const ElementFqId& lhs, const ElementFqId& rhs) noexcept;

/// \brief We need to store ElementFqId in a map, so we need to be able to sort it
bool operator<(const ElementFqId&, const ElementFqId&) noexcept;

::bmw::mw::log::LogStream& operator<<(::bmw::mw::log::LogStream& log_stream, const ElementFqId& element_fq_id);

}  // namespace com::impl::lola

}  // namespace bmw::mw

namespace std
{
/// \brief ElementFqId is used as a key for maps, so we need a hash func for it.
///
/// The ElementType enum is not used in the hash function since the other 3 elements already uniquely identify a service
/// element.
template <>
// NOLINTNEXTLINE(bmw-struct-usage-compliance):struct type for consistency with STL
struct hash<bmw::mw::com::impl::lola::ElementFqId>
{
    std::size_t operator()(const bmw::mw::com::impl::lola::ElementFqId& element_fq_id) const noexcept
    {
        return std::hash<std::uint64_t>{}((static_cast<std::uint64_t>(element_fq_id.service_id_) << 24U) |
                                          (static_cast<std::uint64_t>(element_fq_id.element_id_) << 16U) |
                                          static_cast<std::uint64_t>(element_fq_id.instance_id_));
    }
};
}  // namespace std

#endif  // PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_ELEMENT_FQ_ID_H
