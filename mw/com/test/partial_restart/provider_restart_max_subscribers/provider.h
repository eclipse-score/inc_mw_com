// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#ifndef PLATFORM_AAS_MW_COM_TEST_PARTIAL_RESTART_PROVIDER_RESTART_MAX_SUBSCRIBERS_PROVIDER_H
#define PLATFORM_AAS_MW_COM_TEST_PARTIAL_RESTART_PROVIDER_RESTART_MAX_SUBSCRIBERS_PROVIDER_H

#include "platform/aas/mw/com/test/common_test_resources/check_point_control.h"

#include <amp_stop_token.hpp>

namespace bmw::mw::com::test
{

void DoProviderActions(CheckPointControl& check_point_control,
                       amp::stop_token test_stop_token,
                       int argc,
                       const char** argv) noexcept;

}  // namespace bmw::mw::com::test

#endif  // PLATFORM_AAS_MW_COM_TEST_PARTIAL_RESTART_PROVIDER_RESTART_MAX_SUBSCRIBERS_PROVIDER_H
