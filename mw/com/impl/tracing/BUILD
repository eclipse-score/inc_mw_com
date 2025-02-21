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

load("//platform/aas/bazel/generators:unit_tests.bzl", "cc_gtest_unit_test", "cc_unit_test_suites_for_host_and_qnx")
load("//platform/aas/mw:common_features.bzl", "COMPILER_WARNING_FEATURES")

cc_library(
    name = "type_erased_sample_ptr",
    srcs = ["type_erased_sample_ptr.cpp"],
    hdrs = ["type_erased_sample_ptr.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com/impl:__subpackages__"],
    deps = [
        "@amp",
    ],
)

cc_library(
    name = "i_tracing_runtime_binding",
    srcs = ["i_tracing_runtime_binding.cpp"],
    hdrs = ["i_tracing_runtime_binding.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com/impl:__subpackages__"],
    deps = [
        ":type_erased_sample_ptr",
        "//platform/aas/analysis/tracing/library/generic_trace_api",
        "//platform/aas/lib/memory/shared",
        "//platform/aas/mw/com/impl/tracing/configuration:service_element_instance_identifier_view",
        "@amp",
    ],
)

cc_library(
    name = "skeleton_event_tracing_data",
    srcs = ["skeleton_event_tracing_data.cpp"],
    hdrs = ["skeleton_event_tracing_data.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com/impl:__subpackages__"],
    deps = [
        ":i_tracing_runtime_binding",
        "//platform/aas/mw/com/impl/tracing/configuration:service_element_instance_identifier_view",
    ],
)

cc_library(
    name = "proxy_event_tracing_data",
    srcs = ["proxy_event_tracing_data.cpp"],
    hdrs = ["proxy_event_tracing_data.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com/impl:__subpackages__"],
    deps = [
        "//platform/aas/mw/com/impl:binding_event_receive_handler",
        "//platform/aas/mw/com/impl:event_receive_handler",
        "//platform/aas/mw/com/impl/tracing/configuration:service_element_instance_identifier_view",
    ],
)

cc_library(
    name = "common_event_tracing",
    srcs = ["common_event_tracing.cpp"],
    hdrs = ["common_event_tracing.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com/impl:__subpackages__"],
    deps = [
        "//platform/aas/mw/com/impl:instance_identifier",
        "//platform/aas/mw/com/impl:runtime",
        "//platform/aas/mw/com/impl/tracing:tracing_runtime",
        "//platform/aas/mw/com/impl/tracing/configuration:service_element_instance_identifier_view",
        "//platform/aas/mw/com/impl/tracing/configuration:service_element_type",
        "@amp",
    ],
)

cc_library(
    name = "skeleton_tracing",
    srcs = ["skeleton_tracing.cpp"],
    hdrs = ["skeleton_tracing.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com/impl:__subpackages__"],
    deps = [
        "//platform/aas/mw/com/impl:instance_identifier",
        "//platform/aas/mw/com/impl:runtime",
        "//platform/aas/mw/com/impl:skeleton_binding",
        "//platform/aas/mw/com/impl:skeleton_event_base",
        "//platform/aas/mw/com/impl:skeleton_field_base",
        "//platform/aas/mw/com/impl/tracing:tracing_runtime",
        "//platform/aas/mw/com/impl/tracing/configuration:service_element_instance_identifier_view",
        "@amp",
    ],
)

cc_library(
    name = "skeleton_event_tracing",
    srcs = ["skeleton_event_tracing.cpp"],
    hdrs = ["skeleton_event_tracing.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com/impl:__subpackages__"],
    deps = [
        ":common_event_tracing",
        "//platform/aas/mw/com/impl:binding_type",
        "//platform/aas/mw/com/impl:instance_identifier",
        "//platform/aas/mw/com/impl:runtime",
        "//platform/aas/mw/com/impl:skeleton_event_binding",
        "//platform/aas/mw/com/impl/plumbing:sample_allocatee_ptr",
        "//platform/aas/mw/com/impl/plumbing:sample_ptr",
        "//platform/aas/mw/com/impl/tracing:skeleton_event_tracing_data",
        "//platform/aas/mw/com/impl/tracing:tracing_runtime",
        "//platform/aas/mw/com/impl/tracing/configuration:service_element_instance_identifier_view",
        "@amp",
    ],
)

cc_library(
    name = "proxy_event_tracing",
    srcs = ["proxy_event_tracing.cpp"],
    hdrs = ["proxy_event_tracing.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com/impl:__subpackages__"],
    deps = [
        ":common_event_tracing",
        "//platform/aas/mw/com/impl:generic_proxy_event_binding",
        "//platform/aas/mw/com/impl:instance_identifier",
        "//platform/aas/mw/com/impl:proxy_event_binding",
        "//platform/aas/mw/com/impl:runtime",
        "//platform/aas/mw/com/impl/tracing:proxy_event_tracing_data",
        "//platform/aas/mw/com/impl/tracing:tracing_runtime",
        "//platform/aas/mw/com/impl/tracing/configuration:service_element_instance_identifier_view",
        "//platform/aas/mw/log",
        "@amp",
    ],
)

cc_library(
    name = "trace_error",
    srcs = ["trace_error.cpp"],
    hdrs = ["trace_error.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com/impl:__subpackages__"],
    deps = ["//platform/aas/lib/result"],
)

cc_library(
    name = "i_tracing_runtime",
    srcs = ["i_tracing_runtime.cpp"],
    hdrs = ["i_tracing_runtime.h"],
    features = COMPILER_WARNING_FEATURES,
    implementation_deps = [],
    visibility = ["//platform/aas/mw/com/impl:__subpackages__"],
    deps = [
        ":i_tracing_runtime_binding",
        ":type_erased_sample_ptr",
        "//platform/aas/analysis/tracing/library/generic_trace_api",
        "//platform/aas/lib/memory/shared:i_shared_memory_resource",
        "//platform/aas/mw/com/impl:binding_type",
        "//platform/aas/mw/com/impl/tracing/configuration:proxy_event_trace_point_type",
        "//platform/aas/mw/com/impl/tracing/configuration:proxy_field_trace_point_type",
        "//platform/aas/mw/com/impl/tracing/configuration:service_element_instance_identifier_view",
        "//platform/aas/mw/com/impl/tracing/configuration:skeleton_event_trace_point_type",
        "//platform/aas/mw/com/impl/tracing/configuration:skeleton_field_trace_point_type",
        "@amp",
    ],
)

cc_library(
    name = "tracing_runtime_mock",
    testonly = True,
    srcs = ["tracing_runtime_mock.cpp"],
    hdrs = ["tracing_runtime_mock.h"],
    features = COMPILER_WARNING_FEATURES,
    implementation_deps = [],
    visibility = ["//platform/aas/mw/com/impl:__subpackages__"],
    deps = [
        "//platform/aas/mw/com/impl/tracing:i_tracing_runtime",
        "//third_party/googletest",
    ],
)

cc_library(
    name = "tracing_runtime",
    srcs = ["tracing_runtime.cpp"],
    hdrs = ["tracing_runtime.h"],
    features = COMPILER_WARNING_FEATURES,
    implementation_deps = [
        "//platform/aas/analysis/tracing/library/generic_trace_api",
        "//platform/aas/lib/memory/shared:pointer_arithmetic_util",
    ],
    visibility = ["//platform/aas/mw/com/impl:__subpackages__"],
    deps = [
        "//platform/aas/mw/com/impl/tracing:i_tracing_runtime",
        "//platform/aas/mw/com/impl/tracing:i_tracing_runtime_binding",
        "//platform/aas/mw/com/impl/tracing:trace_error",
        "@amp",
    ],
)

cc_gtest_unit_test(
    name = "trace_error_test",
    srcs = ["trace_error_test.cpp"],
    features = COMPILER_WARNING_FEATURES,
    deps = [
        ":trace_error",
    ],
)

cc_gtest_unit_test(
    name = "tracing_runtime_test",
    srcs = ["tracing_runtime_test.cpp"],
    features = COMPILER_WARNING_FEATURES,
    deps = [
        ":tracing_runtime",
        "//platform/aas/analysis/tracing/library/generic_trace_api/mocks:trace_library_mock",
        "//platform/aas/mw/com/impl/bindings/mock_binding/tracing:tracing_runtime",
    ],
)

cc_gtest_unit_test(
    name = "type_erased_sample_ptr_test",
    srcs = ["type_erased_sample_ptr_test.cpp"],
    features = COMPILER_WARNING_FEATURES,
    deps = [
        ":type_erased_sample_ptr",
        "@amp",
    ],
)

cc_gtest_unit_test(
    name = "proxy_event_tracing_test",
    srcs = ["proxy_event_tracing_test.cpp"],
    features = COMPILER_WARNING_FEATURES,
    deps = [
        "//platform/aas/mw/com/impl",
        "//platform/aas/mw/com/impl:runtime_mock",
        "//platform/aas/mw/com/impl/bindings/mock_binding",
        "//platform/aas/mw/com/impl/test:binding_factory_resources",
        "//platform/aas/mw/com/impl/test:proxy_resources",
        "//platform/aas/mw/com/impl/tracing:tracing_runtime_mock",
        "//platform/aas/mw/com/impl/tracing/configuration:tracing_filter_config_mock",
    ],
)

cc_gtest_unit_test(
    name = "skeleton_event_tracing_test",
    srcs = ["proxy_event_tracing_test.cpp"],
    features = COMPILER_WARNING_FEATURES,
    deps = [
        "//platform/aas/mw/com/impl",
        "//platform/aas/mw/com/impl:runtime_mock",
        "//platform/aas/mw/com/impl/bindings/mock_binding",
        "//platform/aas/mw/com/impl/test:binding_factory_resources",
        "//platform/aas/mw/com/impl/test:proxy_resources",
        "//platform/aas/mw/com/impl/tracing:tracing_runtime_mock",
        "//platform/aas/mw/com/impl/tracing/configuration:tracing_filter_config_mock",
    ],
)

cc_gtest_unit_test(
    name = "skeleton_field_tracing_test",
    srcs = ["proxy_event_tracing_test.cpp"],
    features = COMPILER_WARNING_FEATURES,
    deps = [
        "//platform/aas/mw/com/impl",
        "//platform/aas/mw/com/impl:runtime_mock",
        "//platform/aas/mw/com/impl/bindings/mock_binding",
        "//platform/aas/mw/com/impl/test:binding_factory_resources",
        "//platform/aas/mw/com/impl/test:proxy_resources",
        "//platform/aas/mw/com/impl/tracing:tracing_runtime_mock",
        "//platform/aas/mw/com/impl/tracing/configuration:tracing_filter_config_mock",
    ],
)

cc_gtest_unit_test(
    name = "skeleton_tracing_test",
    srcs = ["proxy_event_tracing_test.cpp"],
    features = COMPILER_WARNING_FEATURES,
    deps = [
        "//platform/aas/mw/com/impl",
        "//platform/aas/mw/com/impl:runtime_mock",
        "//platform/aas/mw/com/impl/bindings/mock_binding",
        "//platform/aas/mw/com/impl/test:binding_factory_resources",
        "//platform/aas/mw/com/impl/test:proxy_resources",
        "//platform/aas/mw/com/impl/tracing:tracing_runtime_mock",
        "//platform/aas/mw/com/impl/tracing/configuration:tracing_filter_config_mock",
    ],
)

cc_unit_test_suites_for_host_and_qnx(
    name = "unit_test_suite",
    cc_unit_tests = [
        ":trace_error_test",
        ":proxy_event_tracing_test",
        ":skeleton_event_tracing_test",
        ":skeleton_field_tracing_test",
        ":skeleton_tracing_test",
        ":type_erased_sample_ptr_test",
        ":tracing_runtime_test",
    ],
    test_suites_from_sub_packages = [
        "//platform/aas/mw/com/impl/tracing/configuration:unit_test_suite",
    ],
    visibility = ["//platform/aas/mw/com/impl:__pkg__"],
)
