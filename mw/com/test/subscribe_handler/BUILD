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
    name = "subscribe_handler",
    srcs = [
        "subscribe_handler_application.cpp",
        "subscribe_handler_application.h",
    ],
    data = ["mw_com_config.json"],
    features = COMPILER_WARNING_FEATURES,
    deps = [
        "//platform/aas/mw/com",
        "//platform/aas/mw/com/test/common_test_resources:sample_sender_receiver",
        "//platform/aas/mw/com/test/common_test_resources:sctf_test_runner",
        "//platform/aas/mw/com/test/common_test_resources:shared_memory_object_guard",
        "//platform/aas/mw/log",
    ],
)

pkg_adaptive_application(
    name = "subscribe_handler-pkg",
    application_name = "subscribe_handler",
    bins = [":subscribe_handler"],
    etcs = [
        "mw_com_config.json",
        "logging.json",
    ],
    visibility = [
        "//platform/aas/mw/com/test/subscribe_handler/sct:__pkg__",
        "//platform/aas/test/mw/com:__pkg__",
    ],
)
