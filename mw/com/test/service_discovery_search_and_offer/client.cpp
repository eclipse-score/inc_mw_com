// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#include "platform/aas/lib/concurrency/future/interruptible_promise.h"
#include "platform/aas/mw/com/impl/bindings/lola/service_discovery_client.h"
#include "platform/aas/mw/com/impl/instance_identifier.h"
#include "platform/aas/mw/com/impl/runtime.h"
#include "platform/aas/mw/com/test/common_test_resources/proxy_observer.h"
#include "platform/aas/mw/com/test/common_test_resources/sctf_test_runner.h"
#include "platform/aas/mw/com/test/common_test_resources/sync_utils.h"
#include "platform/aas/mw/com/test/service_discovery_search_and_offer/test_datatype.h"
#include "platform/aas/mw/com/types.h"

#include <amp_optional.hpp>

#include <fstream>
#include <iostream>
#include <utility>
#include <vector>

namespace bmw
{
namespace mw
{
namespace com
{
namespace test
{

namespace
{

int run_client(const amp::stop_token& stop_token)
{
    using bmw::mw::com::test::TestDataProxy;
    ProxyObserver<TestDataProxy> proxy_observer(kInstanceSpecifierString);

    const auto find_service_handle_result = proxy_observer.StartServiceDiscovery(kNumberOfOfferedServices, stop_token);
    if (!find_service_handle_result.has_value())
    {
        std::cerr << "unable to start service discovery" << find_service_handle_result.error().Message().data()
                  << std::endl;
    }

    bmw::mw::com::test::SyncCoordinator sync_coordinator(kFileName);
    sync_coordinator.Signal();

    return proxy_observer.CheckProxyCreation(stop_token);
}

}  // namespace

}  // namespace test
}  // namespace com
}  // namespace mw
}  // namespace bmw

int main(int argc, const char** argv)
{
    bmw::mw::com::test::SctfTestRunner test_runner(argc, argv, {});

    const auto stop_token = test_runner.GetStopToken();

    bmw::mw::com::test::run_client(stop_token);

    bmw::mw::com::test::SyncCoordinator::CleanUp(bmw::mw::com::test::kFileName);

    return 0;
}
