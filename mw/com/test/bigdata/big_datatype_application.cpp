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


#include "platform/aas/mw/com/test/bigdata/big_datatype_application.h"

#include "platform/aas/mw/com/impl/instance_specifier.h"
#include "platform/aas/mw/com/test/common_test_resources/assert_handler.h"
#include "platform/aas/mw/com/test/common_test_resources/sample_sender_receiver.h"
#include "platform/aas/mw/com/test/common_test_resources/sctf_test_runner.h"

#include <amp_assert.hpp>

#include <cstdlib>
#include <iostream>
#include <vector>

using namespace std::chrono_literals;

int main(int argc, const char** argv)
{
    bmw::mw::com::test::SetupAssertHandler();
    using Parameters = bmw::mw::com::test::SctfTestRunner::RunParameters::Parameters;

    const std::vector<Parameters> allowed_parameters{Parameters::MODE, Parameters::NUM_CYCLES, Parameters::CYCLE_TIME};
    bmw::mw::com::test::SctfTestRunner test_runner(argc, argv, allowed_parameters);
    const auto& run_parameters = test_runner.GetRunParameters();
    const auto mode = run_parameters.GetMode();
    const auto num_cycles = run_parameters.GetNumCycles();
    const auto stop_token = test_runner.GetStopToken();

    bmw::mw::com::test::EventSenderReceiver event_sender_receiver{};

    const auto instance_specifier_result = bmw::mw::com::InstanceSpecifier::Create("xpad/cp60/MapApiLanesStamped");
    if (!instance_specifier_result.has_value())
    {
        std::cerr << "Invalid instance specifier, terminating." << std::endl;

        return EXIT_FAILURE;
    }
    const auto& instance_specifier = instance_specifier_result.value();

    if (mode == "send" || mode == "skeleton")
    {
        const auto cycle_time = run_parameters.GetCycleTime();
        return event_sender_receiver.RunAsSkeleton(instance_specifier, cycle_time, num_cycles, stop_token);
    }
    else if (mode == "recv" || mode == "proxy")
    {
        const auto cycle_time = run_parameters.GetOptionalCycleTime();
        return event_sender_receiver.RunAsProxy(instance_specifier, cycle_time, num_cycles, stop_token);
    }
    else
    {
        std::cerr << "Unknown mode " << mode << ", terminating." << std::endl;
        return EXIT_FAILURE;
    }
}
