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
load("//bazel/tools:json_schema_validator.bzl", "validate_json_schema_test")
load("//platform/aas/bazel/packaging:adaptive_application.bzl", "pkg_adaptive_application")
load("//platform/aas/mw:common_features.bzl", "COMPILER_WARNING_FEATURES")

cc_binary(
    name = "smokeyeyes",
    srcs = [
        "smokeyeyes.cpp",
        "smokeyeyes.h",
    ],
    data = ["logging.json"],
    features = COMPILER_WARNING_FEATURES,
    deps = [
        "//platform/aas/mw/com",
        "//platform/aas/mw/log",
        "//third_party/boost:filesystem",
        "//third_party/boost:interprocess",
        "//third_party/boost:program_options",
    ],
)

validate_json_schema_test(
    name = "validate_lola_schema",
    json = "mw_com_config.json",
    schema = "//platform/aas/mw/com:config_schema",
    tags = ["lint"],
)

pkg_adaptive_application(
    name = "smokeyeyes-pkg",
    application_name = "smokeyeyes",
    bins = [":smokeyeyes"],
    etcs = [
        "mw_com_config.json",
        "logging.json",
    ],
    visibility = [
        "//platform/aas/test/mw/com:__pkg__",
    ],
)
