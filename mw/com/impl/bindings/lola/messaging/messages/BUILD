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

cc_library(
    name = "messages",
    srcs = [
        "message_common.cpp",
        "message_element_fq_id.cpp",
        "message_outdated_nodeid.cpp",
    ],
    hdrs = [
        "message_common.h",
        "message_element_fq_id.h",
        "message_outdated_nodeid.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com/impl/bindings/lola/messaging:__pkg__"],
    deps = [
        "//platform/aas/lib/os:unistd",
        "//platform/aas/mw/com/impl/bindings/lola:element_fq_id",
        "//platform/aas/mw/com/message_passing",
        "@amp",
    ],
)

cc_gtest_unit_test(
    name = "unit_test",
    srcs = [
        "message_common_test.cpp",
        "message_element_fq_id_test.cpp",
        "message_outdated_nodeid_test.cpp",
    ],
    features = COMPILER_WARNING_FEATURES,
    deps = [
        ":messages",
        "//platform/aas/mw/log",
    ],
)
