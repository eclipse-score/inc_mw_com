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
    name = "messaging_app_qnxresmgr",
    srcs = [
        "messaging_application.cpp",
    ],
    features = COMPILER_WARNING_FEATURES + [
        "aborts_upon_exception",
    ],
    target_compatible_with = ["@platforms//os:qnx"],
    deps = [
        "//platform/aas/mw/com/message_passing:message_passing_resmgr",
        "//third_party/boost:program_options",
    ],
)

pkg_adaptive_application(
    name = "messaging_app_qnxresmgr-pkg",
    application_name = "messaging_app_qnxresmgr",
    bins = [":messaging_app_qnxresmgr"],
    etcs = [],
    visibility = [
        "//platform/aas/test/mw/com:__pkg__",
    ],
)

cc_binary(
    name = "messaging_app_mqueue",
    srcs = [
        "messaging_application.cpp",
    ],
    features = COMPILER_WARNING_FEATURES,
    deps = [
        "//platform/aas/mw/com/message_passing:message_passing_mqueue",
        "//third_party/boost:program_options",
    ],
)

pkg_adaptive_application(
    name = "messaging_app_mqueue-pkg",
    application_name = "messaging_app_mqueue",
    bins = [":messaging_app_mqueue"],
    etcs = [],
    visibility = [
        "//platform/aas/mw/com/message_passing/test:__subpackages__",
        "//platform/aas/test/mw/com:__pkg__",
    ],
)
