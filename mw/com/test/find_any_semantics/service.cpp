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


#include "platform/aas/mw/com/test/common_test_resources/sctf_test_runner.h"
#include "platform/aas/mw/com/test/find_any_semantics/test_datatype.h"

#include "platform/aas/lib/result/error.h"
#include "platform/aas/lib/result/result.h"

#include <amp_stop_token.hpp>

#include <chrono>
#include <thread>
#include <utility>

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

using namespace std::chrono_literals;

enum class TestErrorCode : bmw::result::ErrorCode
{
    kCreateInstanceSpecifierFailed = 1,
    kCreateSkeletonFailed = 2,
    kOfferServiceFailed = 3,
};

class TestErrorDomain final : public bmw::result::ErrorDomain
{
  public:
    amp::string_view MessageFor(const bmw::result::ErrorCode& code) const noexcept override
    {
        switch (static_cast<TestErrorCode>(code))
        {
            case TestErrorCode::kCreateInstanceSpecifierFailed:
                return "Failed to create instance specifier.";
            case TestErrorCode::kCreateSkeletonFailed:
                return "Failed to create skeleton.";
            case TestErrorCode::kOfferServiceFailed:
                return "Failed to offer service.";
            default:
                return "Unknown Error!";
        }
    }
};

constexpr TestErrorDomain test_error_domain;

bmw::result::Error MakeError(TestErrorCode code, std::string_view user_message = "") noexcept
{
    return {static_cast<bmw::result::ErrorCode>(code), test_error_domain, user_message};
}

bmw::Result<TestDataSkeleton> offer_service(const std::string& instance_specifier_string)
{
    auto instance_specifier_result = InstanceSpecifier::Create(instance_specifier_string);
    if (!instance_specifier_result.has_value())
    {
        std::cerr << "Unable to create instance specifier\n";
        return MakeUnexpected(TestErrorCode::kCreateInstanceSpecifierFailed);
    }
    auto instance_specifier = std::move(instance_specifier_result).value();

    auto service_result = TestDataSkeleton::Create(std::move(instance_specifier));
    if (!service_result.has_value())
    {
        std::cerr << "Unable to construct TestDataSkeleton: " << service_result.error() << "\n";
        return MakeUnexpected(TestErrorCode::kCreateSkeletonFailed);
    }
    TestDataSkeleton& lola_service = service_result.value();
    lola_service.test_field.Update(kTestValue);
    const auto offer_service_result = lola_service.OfferService();
    if (!offer_service_result.has_value())
    {
        std::cerr << "Unable to offer service for TestDataSkeleton: " << offer_service_result.error() << "\n";
        return MakeUnexpected(TestErrorCode::kOfferServiceFailed);
    }

    return service_result;
}

int run_service(const std::chrono::milliseconds& cycle_time, const amp::stop_token& stop_token)
{
    auto service_result_first = offer_service(kInstanceSpecifierStringServiceFirst);
    if (!service_result_first.has_value())
    {
        return -1;
    }

    auto service_result_second = offer_service(kInstanceSpecifierStringServiceSecond);
    if (!service_result_second.has_value())
    {
        return -2;
    }

    while (!stop_token.stop_requested())
    {
        std::this_thread::sleep_for(cycle_time);
    }

    service_result_first.value().StopOfferService();
    service_result_second.value().StopOfferService();

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

    const std::vector<Parameters> allowed_parameters{Parameters::CYCLE_TIME};
    bmw::mw::com::test::SctfTestRunner test_runner(argc, argv, allowed_parameters);
    const auto& run_parameters = test_runner.GetRunParameters();
    const auto cycle_time = run_parameters.GetCycleTime();
    const auto stop_token = test_runner.GetStopToken();

    return bmw::mw::com::test::run_service(cycle_time, stop_token);
}
