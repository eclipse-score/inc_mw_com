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
load("//platform/aas/bazel/generators:unit_tests.bzl", "cc_gtest_unit_test", "cc_unit_test_suites_for_host_and_qnx")
load("//platform/aas/mw:common_features.bzl", "COMPILER_WARNING_FEATURES")

validate_json_schema_test(
    name = "validate_comtrace_filter_config_schema",
    json = "example/comtrace_filter_config_small.json",
    schema = ":comtrace_filter_config_schema",
    tags = ["lint"],
)

# schema will be moved later to //platform/aas/analysis/common
filegroup(
    name = "comtrace_filter_config_schema",
    srcs = ["comtrace_config_schema.json"],
    visibility = [
        "//platform/aas/mw/com:__pkg__",
    ],
)

cc_library(
    name = "i_tracing_filter_config",
    srcs = ["i_tracing_filter_config.cpp"],
    hdrs = ["i_tracing_filter_config.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com/impl:__subpackages__"],
    deps = [
        "proxy_event_trace_point_type",
        "proxy_field_trace_point_type",
        "skeleton_event_trace_point_type",
        "skeleton_field_trace_point_type",
        "@amp",
    ],
)

cc_library(
    name = "tracing_filter_config",
    srcs = ["tracing_filter_config.cpp"],
    hdrs = ["tracing_filter_config.h"],
    features = COMPILER_WARNING_FEATURES,
    implementation_deps = ["//platform/aas/mw/log"],
    visibility = ["//platform/aas/mw/com/impl:__subpackages__"],
    deps = [
        ":i_tracing_filter_config",
        ":service_element_identifier_view",
        ":service_element_type",
        ":trace_point_key",
    ],
)

cc_library(
    name = "tracing_filter_config_mock",
    testonly = True,
    srcs = ["tracing_filter_config_mock.cpp"],
    hdrs = ["tracing_filter_config_mock.h"],
    features = COMPILER_WARNING_FEATURES,
    implementation_deps = ["//platform/aas/mw/log"],
    visibility = ["//platform/aas/mw/com/impl:__subpackages__"],
    deps = [
        ":i_tracing_filter_config",
        "//third_party/googletest",
    ],
)

cc_library(
    name = "tracing_config",
    srcs = ["tracing_config.cpp"],
    hdrs = ["tracing_config.h"],
    features = COMPILER_WARNING_FEATURES,
    implementation_deps = ["//platform/aas/mw/log"],
    visibility = [
        "//platform/aas/mw/com/impl/configuration:__pkg__",
    ],
)

cc_library(
    name = "service_element_type",
    srcs = ["service_element_type.cpp"],
    hdrs = ["service_element_type.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com/impl:__subpackages__"],
    deps = ["//platform/aas/mw/log"],
)

cc_library(
    name = "hash_helper_for_service_element_and_se_view",
    srcs = ["hash_helper_for_service_element_and_se_view.cpp"],
    hdrs = ["hash_helper_for_service_element_and_se_view.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com/impl:__subpackages__"],
    deps = [
        "//platform/aas/mw/log",
        "@amp",
    ],
)

cc_library(
    name = "service_element_identifier",
    srcs = ["service_element_identifier.cpp"],
    hdrs = ["service_element_identifier.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com/impl:__subpackages__"],
    deps = [
        ":hash_helper_for_service_element_and_se_view",
        ":service_element_type",
        "//platform/aas/mw/log",
        "@amp",
    ],
)

cc_library(
    name = "service_element_identifier_view",
    srcs = ["service_element_identifier_view.cpp"],
    hdrs = ["service_element_identifier_view.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com/impl:__subpackages__"],
    deps = [
        ":hash_helper_for_service_element_and_se_view",
        ":service_element_type",
        "//platform/aas/mw/log",
        "@amp",
    ],
)

cc_library(
    name = "service_element_instance_identifier_view",
    srcs = ["service_element_instance_identifier_view.cpp"],
    hdrs = ["service_element_instance_identifier_view.h"],
    features = COMPILER_WARNING_FEATURES,
    implementation_deps = [
        "//platform/aas/mw/log",
    ],
    visibility = ["//platform/aas/mw/com/impl:__subpackages__"],
    deps = [
        ":service_element_identifier_view",
        "@amp",
    ],
)

cc_library(
    name = "trace_point_key",
    srcs = ["trace_point_key.cpp"],
    hdrs = ["trace_point_key.h"],
    features = COMPILER_WARNING_FEATURES,
    deps = [
        ":service_element_identifier_view",
        "//platform/aas/mw/log",
        "@amp",
    ],
)

cc_library(
    name = "skeleton_event_trace_point_type",
    srcs = ["skeleton_event_trace_point_type.cpp"],
    hdrs = ["skeleton_event_trace_point_type.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com/impl:__subpackages__"],
)

cc_library(
    name = "skeleton_field_trace_point_type",
    srcs = ["skeleton_field_trace_point_type.cpp"],
    hdrs = ["skeleton_field_trace_point_type.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com/impl:__subpackages__"],
)

cc_library(
    name = "proxy_event_trace_point_type",
    srcs = ["proxy_event_trace_point_type.cpp"],
    hdrs = ["proxy_event_trace_point_type.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com/impl:__subpackages__"],
)

cc_library(
    name = "proxy_field_trace_point_type",
    srcs = ["proxy_field_trace_point_type.cpp"],
    hdrs = ["proxy_field_trace_point_type.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com/impl:__subpackages__"],
)

cc_library(
    name = "tracing_filter_config_parser",
    srcs = ["tracing_filter_config_parser.cpp"],
    hdrs = ["tracing_filter_config_parser.h"],
    features = COMPILER_WARNING_FEATURES,
    implementation_deps = [
        "//platform/aas/mw/log",
    ],
    visibility = ["//platform/aas/mw/com/impl:__subpackages__"],
    deps = [
        ":service_element_type",
        ":tracing_filter_config",
        "//platform/aas/lib/json:json_parser",
        "//platform/aas/mw/com/impl/configuration",
        "//platform/aas/mw/com/impl/tracing:trace_error",
        "@amp",
    ],
)

cc_gtest_unit_test(
    name = "service_element_type_test",
    srcs = ["service_element_type_test.cpp"],
    features = COMPILER_WARNING_FEATURES,
    deps = [
        ":service_element_type",
    ],
)

cc_gtest_unit_test(
    name = "service_element_identifier_test",
    srcs = ["service_element_identifier_test.cpp"],
    features = COMPILER_WARNING_FEATURES,
    deps = [
        ":service_element_identifier",
        ":service_element_type",
    ],
)

cc_gtest_unit_test(
    name = "service_element_identifier_view_test",
    srcs = ["service_element_identifier_view_test.cpp"],
    features = COMPILER_WARNING_FEATURES,
    deps = [
        ":service_element_identifier_view",
        ":service_element_type",
    ],
)

cc_gtest_unit_test(
    name = "service_element_instance_identifier_view_test",
    srcs = ["service_element_instance_identifier_view_test.cpp"],
    features = COMPILER_WARNING_FEATURES,
    deps = [
        ":service_element_instance_identifier_view",
    ],
)

cc_gtest_unit_test(
    name = "trace_point_key_test",
    srcs = ["trace_point_key_test.cpp"],
    features = COMPILER_WARNING_FEATURES,
    deps = [
        ":service_element_identifier",
        ":service_element_type",
        ":trace_point_key",
    ],
)

cc_gtest_unit_test(
    name = "tracing_filter_config_test",
    srcs = ["tracing_filter_config_test.cpp"],
    features = COMPILER_WARNING_FEATURES,
    deps = [
        ":tracing_filter_config",
    ],
)

cc_gtest_unit_test(
    name = "skeleton_event_trace_point_type_test",
    srcs = ["skeleton_event_trace_point_type_test.cpp"],
    features = COMPILER_WARNING_FEATURES,
    deps = [
        ":skeleton_event_trace_point_type",
    ],
)

cc_gtest_unit_test(
    name = "skeleton_field_trace_point_type_test",
    srcs = ["skeleton_field_trace_point_type_test.cpp"],
    features = COMPILER_WARNING_FEATURES,
    deps = [
        ":skeleton_field_trace_point_type",
    ],
)

cc_gtest_unit_test(
    name = "proxy_event_trace_point_type_test",
    srcs = ["proxy_event_trace_point_type_test.cpp"],
    features = COMPILER_WARNING_FEATURES,
    deps = [
        ":proxy_event_trace_point_type",
    ],
)

cc_gtest_unit_test(
    name = "proxy_field_trace_point_type_test",
    srcs = ["proxy_field_trace_point_type_test.cpp"],
    features = COMPILER_WARNING_FEATURES,
    deps = [
        ":proxy_field_trace_point_type",
    ],
)

cc_gtest_unit_test(
    name = "tracing_filter_config_parser_test",
    srcs = ["tracing_filter_config_parser_test.cpp"],
    data = ["example/comtrace_filter_config_small.json"],
    features = COMPILER_WARNING_FEATURES,
    deps = [
        ":tracing_filter_config_parser",
    ],
)

cc_unit_test_suites_for_host_and_qnx(
    name = "unit_test_suite",
    cc_unit_tests = [
        ":proxy_event_trace_point_type_test",
        ":proxy_field_trace_point_type_test",
        ":service_element_identifier_test",
        ":service_element_identifier_view_test",
        ":service_element_instance_identifier_view_test",
        ":service_element_type_test",
        ":skeleton_event_trace_point_type_test",
        ":skeleton_field_trace_point_type_test",
        ":trace_point_key_test",
        ":tracing_filter_config_parser_test",
        ":tracing_filter_config_test",
    ],
    visibility = ["//platform/aas/mw/com/impl/tracing:__pkg__"],
)
