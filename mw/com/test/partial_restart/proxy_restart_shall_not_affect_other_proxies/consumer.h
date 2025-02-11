// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#ifndef PLATFORM_AAS_MW_COM_TEST_PARTIAL_RESTART_PROXY_RESTART_SHALL_NOT_AFFECT_OTHER_PROXIES_CONSUMER_H
#define PLATFORM_AAS_MW_COM_TEST_PARTIAL_RESTART_PROXY_RESTART_SHALL_NOT_AFFECT_OTHER_PROXIES_CONSUMER_H
#include "platform/aas/mw/com/test/common_test_resources/check_point_control.h"

namespace bmw::mw::com::test
{
void PerformFirstConsumerActions(CheckPointControl& check_point_control, amp::stop_token stop_token);

void PerformSecondConsumerActions(CheckPointControl& check_point_control,
                                  amp::stop_token stop_token,
                                  const size_t create_proxy_and_receive_M_times);
}  // namespace bmw::mw::com::test
#endif  // PLATFORM_AAS_MW_COM_TEST_PARTIAL_RESTART_PROXY_RESTART_SHALL_NOT_AFFECT_OTHER_PROXIES_CONSUMER_H
