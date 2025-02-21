# *******************************************************************************
# Copyright (c) 2025 Contributors to the Eclipse Foundation
#
# See the NOTICE file(s) distributed with this work for additional
# information regarding copyright ownership.
#
# This program and the accompanying materials are made available under the
# terms of the Apache License Version 2.0 which is available at
# https://www.apache.org/licenses/LICENSE-2.0
#
# SPDX-License-Identifier: Apache-2.0
# *******************************************************************************

load("//platform/aas/bazel/generators:unit_tests.bzl", "cc_gtest_unit_test")
load("//platform/aas/mw:common_features.bzl", "COMPILER_WARNING_FEATURES")
load("//platform/aas/tools/itf/bazel_gen:itf_gen.bzl", "py_unittest_qnx_test")

cc_library(
    name = "com_error_domain",
    srcs = ["com_error_domain.cpp"],
    hdrs = ["com_error_domain.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com:__subpackages__"],
    deps = ["//platform/aas/mw/com/impl:error"],
)

cc_library(
    name = "types",
    srcs = ["types.cpp"],
    hdrs = ["types.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com:__subpackages__"],
    deps = [
        "//platform/aas/mw/com/impl",
        "//platform/aas/mw/com/impl:event_receive_handler",
        "@amp",
    ],
)

cc_library(
    name = "runtime",
    srcs = [
        "runtime.cpp",
    ],
    hdrs = [
        "runtime.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com:__subpackages__"],
    deps = [
        ":types",
        "//platform/aas/mw/com/impl:runtime",
        "@amp",
    ],
)

cc_library(
    name = "com",
    features = COMPILER_WARNING_FEATURES,
    tags = ["FUSA"],
    visibility = ["//visibility:public"],
    deps = [
        ":com_error_domain",
        ":runtime",
        ":types",
    ],
)

alias(
    name = "config_schema",
    actual = "//platform/aas/mw/com/impl/configuration:ara_com_config_schema",
    visibility = ["//visibility:public"],
)

cc_gtest_unit_test(
    name = "unit_test",
    srcs = [
        "runtime_test.cpp",
        "types_accessibility_test.cpp",
        "types_test.cpp",
    ],
    data = [
        "//platform/aas/mw/com/impl/configuration:ara_com_config.json",
    ],
    features = COMPILER_WARNING_FEATURES,
    deps = [
        ":runtime",
        ":types",
        "//platform/aas/mw/com/impl:runtime_mock",
        "//platform/aas/mw/log",
    ],
)

py_unittest_qnx_test(
    name = "unit_test_qnx",
    test_cases = [
        ":unit_test",
    ],
)

test_suite(
    name = "unit_test_suite_host",
    tests = [
        ":unit_test",
        "//platform/aas/mw/com/impl:unit_test_suite_host",
        "//platform/aas/mw/com/message_passing:unit_test_suite_host",
        "//platform/aas/mw/com/test/api:unit_test_suite_host",
    ],
    visibility = ["//platform/aas/mw:__pkg__"],
)

py_unittest_qnx_test(
    name = "unit_test_suite_qnx",
    test_suites = [
        ":unit_test_qnx",
        "//platform/aas/mw/com/impl:unit_test_suite_qnx",
        "//platform/aas/mw/com/message_passing:unit_test_suite_qnx",
        "//platform/aas/mw/com/message_passing:unit_tests_qnx",
        "//platform/aas/mw/com/test/api:unit_test_suite_qnx",
    ],
    visibility = ["//platform/aas/mw:__pkg__"],
)
