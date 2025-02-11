// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#ifndef PLATFORM_AAS_MW_COM_TEST_COMMON_TEST_RESOURCES_CONSUMER_RESOURCES_H
#define PLATFORM_AAS_MW_COM_TEST_COMMON_TEST_RESOURCES_CONSUMER_RESOURCES_H

#include "platform/aas/mw/com/test/common_test_resources/check_point_control.h"
#include "platform/aas/mw/com/types.h"

#include "platform/aas/lib/result/result.h"

#include <amp_string_view.hpp>

#include <condition_variable>
#include <iostream>
#include <memory>
#include <mutex>
#include <utility>

namespace bmw::mw::com::test
{

template <typename ProxyInterface>
Result<ProxyInterface> CreateProxy(const amp::string_view message_prefix,
                                   const typename ProxyInterface::HandleType& handle,
                                   CheckPointControl& check_point_control)
{
    auto lola_proxy_result = ProxyInterface::Create(handle);
    if (!(lola_proxy_result.has_value()))
    {
        std::cerr << message_prefix.data() << ": Unable to create lola proxy:" << lola_proxy_result.error() << "\n";
        check_point_control.ErrorOccurred();
        return MakeUnexpected<ProxyInterface>(std::move(lola_proxy_result).error());
    }
    std::cerr << message_prefix.data() << ": Successfully created lola proxy" << std::endl;
    return lola_proxy_result;
}

template <typename ProxyInterface>
Result<FindServiceHandle> StartFindService(const amp::string_view message_prefix,
                                           FindServiceHandler<typename ProxyInterface::HandleType> handler,
                                           const InstanceSpecifier& instance_specifier,
                                           CheckPointControl& check_point_control)
{
    auto start_find_service_result = ProxyInterface::StartFindService(std::move(handler), instance_specifier);
    if (!(start_find_service_result.has_value()))
    {
        std::cerr << message_prefix.data() << ": Unable to call StartFindService:" << start_find_service_result.error()
                  << "\n";
        check_point_control.ErrorOccurred();
        return MakeUnexpected<FindServiceHandle>(std::move(start_find_service_result).error());
    }
    std::cerr << message_prefix.data() << ": Successfully called StartFindService" << std::endl;
    return start_find_service_result;
}

template <typename ProxyEventInterface>
ResultBlank SubscribeProxyEvent(const amp::string_view message_prefix,
                                ProxyEventInterface& proxy_event,
                                std::size_t max_sample_count,
                                CheckPointControl& check_point_control)
{
    const auto subscribe_result = proxy_event.Subscribe(max_sample_count);
    if (!(subscribe_result.has_value()))
    {
        std::cerr << message_prefix.data() << ": Subscription failed with error: " << subscribe_result.error()
                  << std::endl;
        check_point_control.ErrorOccurred();
        return subscribe_result;
    }
    std::cerr << message_prefix.data() << ": Successfully subscribed" << std::endl;
    return {};
}

template <typename ProxyEventInterface>
ResultBlank SetBasicNotifierReceiveHandler(const amp::string_view message_prefix,
                                           ProxyEventInterface& proxy_event,
                                           concurrency::Notification& event_received,
                                           CheckPointControl& check_point_control)
{
    const auto set_receive_handler_result = proxy_event.SetReceiveHandler([&event_received, message_prefix] {
        std::cerr << message_prefix.data() << ": Event receive handler called" << std::endl;
        event_received.notify();
    });
    if (!(set_receive_handler_result.has_value()))
    {
        std::cerr << message_prefix.data()
                  << ": SetReceiveHandler failed with error: " << set_receive_handler_result.error() << "\n";
        check_point_control.ErrorOccurred();
        return set_receive_handler_result;
    }
    std::cerr << message_prefix.data() << ": SetReceiveHandler succeeded" << std::endl;
    return {};
}

}  // namespace bmw::mw::com::test

#endif  // PLATFORM_AAS_MW_COM_TEST_COMMON_TEST_RESOURCES_CONSUMER_RESOURCES_H
