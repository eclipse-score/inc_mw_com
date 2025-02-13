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


#ifndef PLATFORM_AAS_MW_COM_TEST_PARTIAL_RESTART_PROVIDER_RESTART_PROVIDER_H
#define PLATFORM_AAS_MW_COM_TEST_PARTIAL_RESTART_PROVIDER_RESTART_PROVIDER_H

#include "platform/aas/mw/com/test/common_test_resources/check_point_control.h"

namespace bmw::mw::com::test
{

struct ProviderParameters
{
    size_t num_samples_sent_to_notify_check_point{5};
};

/// \brief
/// \param check_point_control
/// \param test_stop_token
/// \param argc
/// \param argv
void DoProviderActions(CheckPointControl& check_point_control,
                       amp::stop_token test_stop_token,
                       int argc,
                       const char** argv) noexcept;

}  // namespace bmw::mw::com::test

#endif  // PLATFORM_AAS_MW_COM_TEST_PARTIAL_RESTART_PROVIDER_RESTART_PROVIDER_H
