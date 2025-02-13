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


#ifndef PLATFORM_AAS_MW_COM_TEST_SHARED_MEMORY_STORAGE_TEST_RESOURCES_H
#define PLATFORM_AAS_MW_COM_TEST_SHARED_MEMORY_STORAGE_TEST_RESOURCES_H

#include "platform/aas/mw/com/impl/bindings/lola/element_fq_id.h"
#include "platform/aas/mw/com/impl/bindings/lola/proxy.h"
#include "platform/aas/mw/com/impl/bindings/lola/service_data_storage.h"
#include "platform/aas/mw/com/impl/bindings/lola/skeleton.h"

#include "platform/aas/lib/memory/shared/pointer_arithmetic_util.h"
#include "platform/aas/lib/os/utils/interprocess/interprocess_notification.h"
#include "platform/aas/mw/com/test/common_test_resources/big_datatype.h"

#include <amp_optional.hpp>

#include <stdint.h>
#include <array>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <ostream>

namespace bmw::mw::com
{

namespace impl::lola
{

class ProxyTestAttorney
{
  public:
    explicit ProxyTestAttorney(Proxy& proxy) : proxy_{proxy} {}

    amp::optional<uintptr_t> GetEventMetaInfoAddress(const ElementFqId element_fq_id) const noexcept
    {
        auto* const service_data_storage = static_cast<ServiceDataStorage*>(proxy_.data_->getUsableBaseAddress());
        if (service_data_storage == nullptr)
        {
            return {};
        }
        const auto event_meta_info_entry = service_data_storage->events_metainfo_.find(element_fq_id);
        if (event_meta_info_entry == service_data_storage->events_metainfo_.end())
        {
            return {};
        }
        void* const meta_info_address = &event_meta_info_entry->second;
        return memory::shared::SubtractPointers(meta_info_address, proxy_.data_->getBaseAddress());
    }

  private:
    Proxy& proxy_;
};

class SkeletonAttorney
{
  public:
    explicit SkeletonAttorney(Skeleton& skeleton) : skeleton_{skeleton} {}

    amp::optional<uintptr_t> GetEventMetaInfoAddress(const ElementFqId element_fq_id) const noexcept
    {
        auto search = skeleton_.storage_->events_metainfo_.find(element_fq_id);
        if (search == skeleton_.storage_->events_metainfo_.cend())
        {
            return {};
        }
        void* const meta_info_address = &search->second;
        return memory::shared::SubtractPointers(meta_info_address, skeleton_.storage_resource_->getBaseAddress());
    }

  private:
    Skeleton& skeleton_;
};

}  // namespace impl::lola

namespace test
{

class NotifierGuard
{
  public:
    NotifierGuard(bmw::os::InterprocessNotification& notifier) noexcept : notifier_{notifier} {}
    ~NotifierGuard() noexcept { notifier_.notify(); }

  private:
    bmw::os::InterprocessNotification& notifier_;
};

struct ProxyCreationData
{
    std::unique_ptr<BigDataProxy::HandleType> handle{nullptr};
    std::mutex mutex{};
    std::condition_variable condition_variable{};
};

struct BigDataServiceElementData
{
    std::array<bmw::mw::com::impl::lola::ElementFqId, 2> service_element_element_fq_ids;
    std::array<uintptr_t, 2> service_element_type_meta_information_addresses;
};

bool operator==(const BigDataServiceElementData& lhs, const BigDataServiceElementData& rhs) noexcept;
bool operator!=(const BigDataServiceElementData& lhs, const BigDataServiceElementData& rhs) noexcept;
std::ostream& operator<<(std::ostream& output_stream, const BigDataServiceElementData& service_element_data);

template <typename ImplViewType, typename LolaType, typename ImplType>
LolaType* GetLolaBinding(ImplType& element)
{
    auto* const binding = ImplViewType(element).GetBinding();
    auto* const lola_binding = dynamic_cast<LolaType*>(binding);
    return lola_binding;
}

}  // namespace test
}  // namespace bmw::mw::com

#endif  // PLATFORM_AAS_MW_COM_TEST_SHARED_MEMORY_STORAGE_TEST_RESOURCES_H
