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
    name = "validate_lola_service_schema",
    json = "config/service/mw_com_config.json",
    schema = "//platform/aas/mw/com:config_schema",
    tags = ["lint"],
)

validate_json_schema_test(
    name = "validate_lola_client_schema",
    json = "config/client/mw_com_config.json",
    schema = "//platform/aas/mw/com:config_schema",
    tags = ["lint"],
)

cc_library(
    name = "test_datatype",
    srcs = ["test_datatype.cpp"],
    hdrs = ["test_datatype.h"],
    features = COMPILER_WARNING_FEATURES,
    deps = ["//platform/aas/mw/com"],
)

cc_binary(
    name = "service",
    srcs = ["service.cpp"],
    data = ["config/service/mw_com_config.json"],
    features = COMPILER_WARNING_FEATURES + [
        "aborts_upon_exception",
    ],
    visibility = ["//platform/aas/mw/com/test/find_all_semantics:__pkg__"],
    deps = [
        ":test_datatype",
        "//platform/aas/lib/result",
        "//platform/aas/mw/com",
        "//platform/aas/mw/com/test/common_test_resources:sctf_test_runner",
        "//platform/aas/mw/log",
    ],
)

cc_binary(
    name = "client",
    srcs = ["client.cpp"],
    data = ["config/client/mw_com_config.json"],
    features = COMPILER_WARNING_FEATURES + [
        "aborts_upon_exception",
    ],
    visibility = ["//platform/aas/mw/com/test/find_all_semantics:__pkg__"],
    deps = [
        ":test_datatype",
        "//platform/aas/mw/com",
        "//platform/aas/mw/com/test/common_test_resources:sctf_test_runner",
        "//platform/aas/mw/log",
    ],
)

pkg_adaptive_application(
    name = "service-pkg",
    application_name = "ServiceApp",
    bins = [":service"],
    etcs = [
        "config/service/mw_com_config.json",
        "config/logging.json",
    ],
    visibility = [
        "//platform/aas/mw/com/test/find_any_semantics/sct:__pkg__",
        "//platform/aas/test/mw/com:__pkg__",
    ],
)

pkg_adaptive_application(
    name = "client-pkg",
    application_name = "ClientApp",
    bins = [":client"],
    etcs = [
        "config/client/mw_com_config.json",
        "config/logging.json",
    ],
    visibility = [
        "//platform/aas/mw/com/test/find_any_semantics/sct:__pkg__",
        "//platform/aas/test/mw/com:__pkg__",
    ],
)
