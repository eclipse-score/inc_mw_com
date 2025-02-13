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

load("//platform/aas/bazel/packaging:adaptive_application.bzl", "pkg_adaptive_application")
load("//platform/aas/mw:common_features.bzl", "COMPILER_WARNING_FEATURES")

cc_binary(
    name = "separate_reception_threads",
    srcs = [
        "separate_reception_threads_application.cpp",
        "separate_reception_threads_application.h",
    ],
    data = [
        "logging.json",
        "mw_com_config.json",
    ],
    features = COMPILER_WARNING_FEATURES,
    deps = [
        "//platform/aas/language/safecpp/scoped_function:move_only_scoped_function",
        "//platform/aas/mw/com",
        "//platform/aas/mw/com/test/common_test_resources:sctf_test_runner",
        "//platform/aas/mw/log",
        "//third_party/boost:filesystem",
        "//third_party/boost:program_options",
        "@amp",
    ],
)

pkg_adaptive_application(
    name = "separate_reception_threads-pkg",
    application_name = "separate_reception_threads",
    bins = [":separate_reception_threads"],
    etcs = [
        "mw_com_config.json",
        "logging.json",
    ],
    visibility = [
        "//platform/aas/mw/com/test/separate_reception_threads/sct:__pkg__",
        "//platform/aas/test/mw/com:__pkg__",
    ],
)
