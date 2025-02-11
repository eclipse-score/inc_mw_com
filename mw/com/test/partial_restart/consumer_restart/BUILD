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

validate_json_schema_test(
    name = "validate_mw_com_config_schema",
    json = "mw_com_config.json",
    schema = "//platform/aas/mw/com/impl/configuration:ara_com_config_schema",
    tags = ["lint"],
)

cc_binary(
    name = "consumer_restart_application",
    srcs = [
        "consumer_restart_application.cpp",
        "consumer_restart_application.h",
    ],
    data = ["mw_com_config.json"],
    features = COMPILER_WARNING_FEATURES,
    deps = [
        ":consumer",
        ":provider",
        "//platform/aas/mw/com/test/common_test_resources:stop_token_sig_term_handler",
        "//platform/aas/mw/com/test/common_test_resources:test_resources",
        "//platform/aas/mw/log",
        "//third_party/boost:program_options",
        "@amp",
    ],
)

cc_library(
    name = "consumer",
    srcs = ["consumer.cpp"],
    hdrs = ["consumer.h"],
    features = COMPILER_WARNING_FEATURES,
    deps = [
        "//platform/aas/mw/com/test/common_test_resources:test_resources",
        "//platform/aas/mw/com/test/partial_restart:consumer_handle_notification_data",
        "//platform/aas/mw/com/test/partial_restart:test_datatype",
    ],
)

cc_library(
    name = "provider",
    srcs = ["provider.cpp"],
    hdrs = ["provider.h"],
    features = COMPILER_WARNING_FEATURES,
    implementation_deps = [
        "//platform/aas/mw/com",
        "//platform/aas/mw/com/test/partial_restart:test_datatype",
    ],
    deps = [
        "//platform/aas/mw/com/test/common_test_resources:test_resources",
        "@amp",
    ],
)

pkg_adaptive_application(
    name = "consumer_restart-pkg",
    application_name = "consumer_restart",
    bins = [":consumer_restart_application"],
    etcs = [
        "comtrace_filter_config.json",
        "mw_com_config.json",
        "logging.json",
    ],
    visibility = [
        "//platform/aas/mw/com/test/partial_restart/consumer_restart/sct:__subpackages__",
        "//platform/aas/test/mw/com:__pkg__",
    ],
)
