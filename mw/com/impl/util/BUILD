# *******************************************************************************>
# Copyright (c) 2024 Contributors to the Eclipse Foundation
# See the NOTICE file(s) distributed with this work for additional
# information regarding copyright ownership.
# This program and the accompanying materials are made available under the
# terms of the Apache License Version 2.0 which is available at
# https://www.apache.org/licenses/LICENSE-2.0
# SPDX-License-Identifier: Apache-2.0 #
# *******************************************************************************


# 
load("//platform/aas/bazel/generators:unit_tests.bzl", "cc_gtest_unit_test", "cc_unit_test_suites_for_host_and_qnx")
load("//platform/aas/mw:common_features.bzl", "COMPILER_WARNING_FEATURES")

cc_library(
    name = "copyable_atomic",
    srcs = ["copyable_atomic.cpp"],
    hdrs = ["copyable_atomic.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com/impl:__subpackages__"],
    deps = [
    ],
)

cc_gtest_unit_test(
    name = "copyable_atomic_test",
    srcs = ["copyable_atomic_test.cpp"],
    features = COMPILER_WARNING_FEATURES,
    deps = [
        ":copyable_atomic",
    ],
)

cc_unit_test_suites_for_host_and_qnx(
    name = "unit_test_suite",
    cc_unit_tests = [
        ":copyable_atomic_test",
    ],
    visibility = ["//platform/aas/mw/com/impl/util:__pkg__"],
)
