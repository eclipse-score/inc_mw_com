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

load("//bazel/tools:json_schema_validator.bzl", "validate_json_schema_test")
load("//platform/aas/bazel/packaging:adaptive_application.bzl", "pkg_adaptive_application")
load("//platform/aas/mw:common_features.bzl", "COMPILER_WARNING_FEATURES")

cc_binary(
    name = "proxy_restart_shall_not_affect_other_proxies",
    srcs = ["application.cpp"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com/test:__pkg__"],
    deps = [
        ":consumer",
        ":provider",
        "//platform/aas/mw/com/test/common_test_resources:shared_memory_object_guard",
        "//platform/aas/mw/com/test/common_test_resources:stop_token_sig_term_handler",
        "//platform/aas/mw/com/test/common_test_resources:test_resources",
        "//third_party/boost:program_options",
        "@amp",
    ],
)

cc_library(
    name = "consumer",
    srcs = ["consumer.cpp"],
    hdrs = ["consumer.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com/test:__pkg__"],
    deps = [
        "//platform/aas/mw/com/test/common_test_resources:stop_token_sig_term_handler",
        "//platform/aas/mw/com/test/common_test_resources:test_resources",
        "//platform/aas/mw/com/test/partial_restart:consumer_handle_notification_data",
        "//platform/aas/mw/com/test/partial_restart:test_datatype",
        "@amp",
    ],
)

cc_library(
    name = "provider",
    srcs = ["provider.cpp"],
    hdrs = ["provider.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com/test:__pkg__"],
    deps = [
        "//platform/aas/mw/com/test/common_test_resources:stop_token_sig_term_handler",
        "//platform/aas/mw/com/test/common_test_resources:test_resources",
        "//platform/aas/mw/com/test/partial_restart:test_datatype",
        "@amp",
    ],
)

validate_json_schema_test(
    name = "validate_mw_com_config_schema",
    json = "mw_com_config.json",
    schema = "//platform/aas/mw/com/impl/configuration:ara_com_config_schema",
    tags = ["unit"],
)

pkg_adaptive_application(
    name = "proxy_restart_shall_not_affect_other_proxies-pkg",
    application_name = "proxy_restart_shall_not_affect_other_proxies",
    bins = [":proxy_restart_shall_not_affect_other_proxies"],
    etcs = [
        "mw_com_config.json",
        "logging.json",
    ],
    visibility = [
        "//platform/aas/mw/com/test/partial_restart/proxy_restart_shall_not_affect_other_proxies/sct:__pkg__",
        "//platform/aas/test/mw/com:__pkg__",
    ],
)
