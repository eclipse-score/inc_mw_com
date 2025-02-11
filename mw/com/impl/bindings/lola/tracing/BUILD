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
    name = "tracing_runtime",
    srcs = ["tracing_runtime.cpp"],
    hdrs = ["tracing_runtime.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = [
        "//platform/aas/mw/com/impl/bindings/lola:__subpackages__",
        "//platform/aas/mw/com/impl/plumbing:__subpackages__",
    ],
    deps = [
        "//platform/aas/language/safecpp/scoped_function:move_only_scoped_function",
        "//platform/aas/lib/containers:dynamic_array",
        "//platform/aas/lib/memory/shared:i_shared_memory_resource",
        "//platform/aas/mw/com/impl/configuration",
        "//platform/aas/mw/com/impl/plumbing:sample_ptr",
        "//platform/aas/mw/com/impl/tracing:i_tracing_runtime_binding",
        "//platform/aas/mw/com/impl/tracing/configuration:service_element_instance_identifier_view",
        "//platform/aas/mw/com/impl/tracing/configuration:service_element_type",
        "//platform/aas/mw/log",
        "@amp",
    ],
)

cc_gtest_unit_test(
    name = "tracing_runtime_test",
    srcs = ["tracing_runtime_test.cpp"],
    features = COMPILER_WARNING_FEATURES,
    deps = [
        ":tracing_runtime",
        "//platform/aas/analysis/tracing/library/generic_trace_api/mocks:trace_library_mock",
        "//platform/aas/mw/com/impl/bindings/lola/test:skeleton_test_resources",
    ],
)

cc_unit_test_suites_for_host_and_qnx(
    name = "unit_test_suite",
    cc_unit_tests = [
        ":tracing_runtime_test",
    ],
    visibility = ["//platform/aas/mw/com/impl/bindings/lola:__pkg__"],
)
