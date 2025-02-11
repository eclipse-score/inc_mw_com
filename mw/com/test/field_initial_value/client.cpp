// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#include "platform/aas/mw/com/test/common_test_resources/sctf_test_runner.h"
#include "platform/aas/mw/com/test/field_initial_value/test_datatype.h"
#include "platform/aas/mw/com/types.h"

#include <amp_optional.hpp>

#include <chrono>
#include <cstddef>
#include <iostream>
#include <thread>

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

constexpr auto kMaxNumSamples{1U};

int run_client(const std::size_t num_retries, const std::chrono::milliseconds retry_backoff_time)
{
    using bmw::mw::com::test::TestDataProxy;

    auto instance_specifier_result = InstanceSpecifier::Create(kInstanceSpecifierString);
    if (!instance_specifier_result.has_value())
    {
        std::cerr << "Unable to create instance specifier, terminating\n";
        return -7;
    }
    auto instance_specifier = std::move(instance_specifier_result).value();

    auto lola_proxy_handles_result = TestDataProxy::FindService(std::move(instance_specifier));
    if (!lola_proxy_handles_result.has_value())
    {
        std::cerr << "Unable to get handles, terminating\n";
        return -1;
    }

    auto lola_proxy_handles = lola_proxy_handles_result.value();
    if (lola_proxy_handles.empty())
    {
        std::cerr << "Unable to find lola service, terminating\n";
        return -2;
    }

    auto lola_proxy_result = TestDataProxy::Create(lola_proxy_handles[0]);
    if (!lola_proxy_result.has_value())
    {
        std::cerr << "Unable to create lola proxy, terminating\n";
        return -3;
    }
    auto& lola_proxy = lola_proxy_result.value();
    amp::optional<std::int32_t> received_value;

    lola_proxy.test_field.Subscribe(kMaxNumSamples);
    std::size_t retries = num_retries;
    while (lola_proxy.test_field.GetSubscriptionState() != bmw::mw::com::impl::SubscriptionState::kSubscribed)
    {
        std::this_thread::sleep_for(retry_backoff_time);
        retries--;
        if (retries <= 0)
        {
            std::cerr << "Subscription failed!\n";
            return -4;
        }
    }
    lola_proxy.test_field.GetNewSamples(
        [&received_value](const auto& sample_ptr) noexcept { received_value = *sample_ptr; }, kMaxNumSamples);

    lola_proxy.test_field.Unsubscribe();

    if (!received_value.has_value())
    {
        std::cerr << "Lola didn't receive a sample!\n";
        return -5;
    }

    if (received_value.value() != kTestValue)
    {
        std::cerr << "Expecting:" << kTestValue << " Received:" << received_value.value() << "!\n ";
        return -6;
    }

    return 0;
}

}  // namespace

}  // namespace test
}  // namespace com
}  // namespace mw
}  // namespace bmw

int main(int argc, const char** argv)
{
    using Parameters = bmw::mw::com::test::SctfTestRunner::RunParameters::Parameters;

    const std::vector<Parameters> allowed_parameters{Parameters::NUM_RETRIES, Parameters::RETRY_BACKOFF_TIME};
    bmw::mw::com::test::SctfTestRunner test_runner(argc, argv, allowed_parameters);
    const auto& run_parameters = test_runner.GetRunParameters();
    const auto num_retries = run_parameters.GetNumRetries();
    const auto retry_backoff_time = run_parameters.GetRetryBackoffTime();
    const auto stop_token = test_runner.GetStopToken();

    return bmw::mw::com::test::run_client(num_retries, retry_backoff_time);
}
