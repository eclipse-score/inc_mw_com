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

load("//platform/aas/bazel/generators:unit_tests.bzl", "cc_unit_test_suites_for_host_and_qnx")
load("//platform/aas/mw:common_features.bzl", "COMPILER_WARNING_FEATURES")

cc_library(
    name = "plumbing",
    srcs = [
        "runtime_binding_factory.cpp",
    ],
    hdrs = [
        "runtime_binding_factory.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com/impl:__pkg__"],
    deps = [
        ":proxy_binding_factory",
        ":sample_allocatee_ptr",
        ":sample_ptr",
        ":skeleton_binding_factory",
        "//platform/aas/lib/concurrency:executor",
        "//platform/aas/mw/com/impl:instance_identifier",
        "//platform/aas/mw/com/impl/bindings/lola",
        "@amp",
    ],
)

cc_library(
    name = "i_proxy_binding_factory",
    srcs = ["i_proxy_binding_factory.cpp"],
    hdrs = ["i_proxy_binding_factory.h"],
    features = COMPILER_WARNING_FEATURES,
    tags = ["FFI"],
    deps = [
        "//platform/aas/mw/com/impl:handle_type",
        "//platform/aas/mw/com/impl:instance_identifier",
        "//platform/aas/mw/com/impl:proxy_binding",
        "@amp",
    ],
)

cc_library(
    name = "i_proxy_event_binding_factory",
    srcs = ["i_proxy_event_binding_factory.cpp"],
    hdrs = ["i_proxy_event_binding_factory.h"],
    features = COMPILER_WARNING_FEATURES,
    tags = ["FFI"],
    deps = [
        "//platform/aas/mw/com/impl:instance_identifier",
        "//platform/aas/mw/com/impl:proxy_base",
        "@amp",
    ],
)

cc_library(
    name = "i_proxy_field_binding_factory",
    srcs = ["i_proxy_field_binding_factory.cpp"],
    hdrs = ["i_proxy_field_binding_factory.h"],
    features = COMPILER_WARNING_FEATURES,
    tags = ["FFI"],
    deps = [
        "//platform/aas/mw/com/impl:instance_identifier",
        "//platform/aas/mw/com/impl:proxy_base",
        "@amp",
    ],
)

cc_library(
    name = "i_skeleton_binding_factory",
    srcs = ["i_skeleton_binding_factory.cpp"],
    hdrs = ["i_skeleton_binding_factory.h"],
    features = COMPILER_WARNING_FEATURES,
    tags = ["FFI"],
    deps = [
        "//platform/aas/mw/com/impl:instance_identifier",
        "//platform/aas/mw/com/impl:skeleton_binding",
        "@amp",
    ],
)

cc_library(
    name = "i_skeleton_event_binding_factory",
    srcs = ["i_skeleton_event_binding_factory.cpp"],
    hdrs = ["i_skeleton_event_binding_factory.h"],
    features = COMPILER_WARNING_FEATURES,
    tags = ["FFI"],
    deps = [
        "//platform/aas/mw/com/impl:instance_identifier",
        "//platform/aas/mw/com/impl:skeleton_base",
        "@amp",
    ],
)

cc_library(
    name = "i_skeleton_field_binding_factory",
    srcs = ["i_skeleton_field_binding_factory.cpp"],
    hdrs = ["i_skeleton_field_binding_factory.h"],
    features = COMPILER_WARNING_FEATURES,
    tags = ["FFI"],
    deps = [
        "//platform/aas/mw/com/impl:instance_identifier",
        "//platform/aas/mw/com/impl:skeleton_base",
        "@amp",
    ],
)

cc_library(
    name = "proxy_binding_factory_impl",
    srcs = ["proxy_binding_factory_impl.cpp"],
    hdrs = ["proxy_binding_factory_impl.h"],
    features = COMPILER_WARNING_FEATURES,
    implementation_deps = ["//platform/aas/mw/com/impl/bindings/lola"],
    tags = ["FFI"],
    deps = [
        ":i_proxy_binding_factory",
        "@amp",
    ],
)

cc_library(
    name = "proxy_event_binding_factory_impl",
    srcs = ["proxy_event_binding_factory_impl.cpp"],
    hdrs = ["proxy_event_binding_factory_impl.h"],
    features = COMPILER_WARNING_FEATURES,
    implementation_deps = ["//platform/aas/mw/com/impl/bindings/lola"],
    tags = ["FFI"],
    deps = [
        ":i_proxy_event_binding_factory",
        "//platform/aas/mw/com/impl/plumbing/test:proxy_event_binding_resources",
        "@amp",
    ],
)

cc_library(
    name = "proxy_field_binding_factory_impl",
    srcs = ["proxy_field_binding_factory_impl.cpp"],
    hdrs = ["proxy_field_binding_factory_impl.h"],
    features = COMPILER_WARNING_FEATURES,
    implementation_deps = ["//platform/aas/mw/com/impl/bindings/lola"],
    tags = ["FFI"],
    deps = [
        ":i_proxy_field_binding_factory",
        "//platform/aas/mw/com/impl/plumbing/test:proxy_event_binding_resources",
        "@amp",
    ],
)

cc_library(
    name = "skeleton_binding_factory_impl",
    srcs = ["skeleton_binding_factory_impl.cpp"],
    hdrs = ["skeleton_binding_factory_impl.h"],
    features = COMPILER_WARNING_FEATURES,
    implementation_deps = [
        "//platform/aas/mw/com/impl/bindings/lola",
        "//platform/aas/mw/com/impl/bindings/lola:partial_restart_path_builder",
        "//platform/aas/mw/com/impl/bindings/lola:shm_path_builder",
    ],
    tags = ["FFI"],
    deps = [
        ":i_skeleton_binding_factory",
        "@amp",
    ],
)

cc_library(
    name = "skeleton_event_binding_factory_impl",
    srcs = ["skeleton_event_binding_factory_impl.cpp"],
    hdrs = ["skeleton_event_binding_factory_impl.h"],
    features = COMPILER_WARNING_FEATURES,
    implementation_deps = ["//platform/aas/mw/com/impl/bindings/lola"],
    tags = ["FFI"],
    deps = [
        ":i_skeleton_event_binding_factory",
        "@amp",
    ],
)

cc_library(
    name = "skeleton_field_binding_factory_impl",
    srcs = ["skeleton_field_binding_factory_impl.cpp"],
    hdrs = ["skeleton_field_binding_factory_impl.h"],
    features = COMPILER_WARNING_FEATURES,
    implementation_deps = ["//platform/aas/mw/com/impl/bindings/lola"],
    tags = ["FFI"],
    deps = [
        ":i_skeleton_field_binding_factory",
        "@amp",
    ],
)

cc_library(
    name = "proxy_binding_factory",
    srcs = ["proxy_binding_factory.cpp"],
    hdrs = ["proxy_binding_factory.h"],
    features = COMPILER_WARNING_FEATURES,
    implementation_deps = [
        "proxy_binding_factory_impl",
    ],
    tags = ["FFI"],
    deps = [
        ":i_proxy_binding_factory",
        "@amp",
    ],
)

cc_library(
    name = "proxy_event_binding_factory",
    srcs = ["proxy_event_binding_factory.cpp"],
    hdrs = ["proxy_event_binding_factory.h"],
    features = COMPILER_WARNING_FEATURES,
    tags = ["FFI"],
    deps = [
        "proxy_event_binding_factory_impl",
        ":i_proxy_event_binding_factory",
        "@amp",
    ],
)

cc_library(
    name = "proxy_field_binding_factory",
    srcs = ["proxy_field_binding_factory.cpp"],
    hdrs = ["proxy_field_binding_factory.h"],
    features = COMPILER_WARNING_FEATURES,
    tags = ["FFI"],
    deps = [
        "proxy_field_binding_factory_impl",
        ":i_proxy_field_binding_factory",
        "@amp",
    ],
)

cc_library(
    name = "skeleton_binding_factory",
    srcs = ["skeleton_binding_factory.cpp"],
    hdrs = ["skeleton_binding_factory.h"],
    features = COMPILER_WARNING_FEATURES,
    implementation_deps = [
        "skeleton_binding_factory_impl",
    ],
    tags = ["FFI"],
    deps = [
        ":i_skeleton_binding_factory",
        "@amp",
    ],
)

cc_library(
    name = "skeleton_event_binding_factory",
    srcs = ["skeleton_event_binding_factory.cpp"],
    hdrs = ["skeleton_event_binding_factory.h"],
    features = COMPILER_WARNING_FEATURES,
    tags = ["FFI"],
    deps = [
        "skeleton_event_binding_factory_impl",
        ":i_skeleton_event_binding_factory",
        "@amp",
    ],
)

cc_library(
    name = "skeleton_field_binding_factory",
    srcs = ["skeleton_field_binding_factory.cpp"],
    hdrs = ["skeleton_field_binding_factory.h"],
    features = COMPILER_WARNING_FEATURES,
    tags = ["FFI"],
    deps = [
        "skeleton_field_binding_factory_impl",
        ":i_skeleton_field_binding_factory",
        "@amp",
    ],
)

cc_library(
    name = "proxy_binding_factory_mock",
    testonly = True,
    srcs = ["proxy_binding_factory_mock.cpp"],
    hdrs = ["proxy_binding_factory_mock.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas:__subpackages__"],
    deps = [
        ":i_proxy_binding_factory",
        "//third_party/googletest",
    ],
)

cc_library(
    name = "proxy_event_binding_factory_mock",
    testonly = True,
    srcs = ["proxy_event_binding_factory_mock.cpp"],
    hdrs = ["proxy_event_binding_factory_mock.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas:__subpackages__"],
    deps = [
        ":i_proxy_event_binding_factory",
        "//third_party/googletest",
    ],
)

cc_library(
    name = "proxy_field_binding_factory_mock",
    testonly = True,
    srcs = ["proxy_field_binding_factory_mock.cpp"],
    hdrs = ["proxy_field_binding_factory_mock.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas:__subpackages__"],
    deps = [
        ":i_proxy_field_binding_factory",
        "//third_party/googletest",
    ],
)

cc_library(
    name = "skeleton_binding_factory_mock",
    testonly = True,
    srcs = ["skeleton_binding_factory_mock.cpp"],
    hdrs = ["skeleton_binding_factory_mock.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas:__subpackages__"],
    deps = [
        ":i_skeleton_binding_factory",
        "//third_party/googletest",
    ],
)

cc_library(
    name = "skeleton_event_binding_factory_mock",
    testonly = True,
    srcs = ["skeleton_event_binding_factory_mock.cpp"],
    hdrs = ["skeleton_event_binding_factory_mock.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas:__subpackages__"],
    deps = [
        ":i_skeleton_event_binding_factory",
        "//third_party/googletest",
    ],
)

cc_library(
    name = "skeleton_field_binding_factory_mock",
    testonly = True,
    srcs = ["skeleton_field_binding_factory_mock.cpp"],
    hdrs = ["skeleton_field_binding_factory_mock.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas:__subpackages__"],
    deps = [
        ":i_skeleton_field_binding_factory",
        "//third_party/googletest",
    ],
)

cc_library(
    name = "runtime",
    srcs = [
        "runtime_binding_factory.cpp",
    ],
    hdrs = [
        "runtime_binding_factory.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    implementation_deps = [
        "//platform/aas/mw/com/impl/bindings/lola/tracing:tracing_runtime",
    ],
    visibility = ["//platform/aas/mw/com/impl:__pkg__"],
    deps = [
        "//platform/aas/mw/com/impl:runtime_interfaces",
        "//platform/aas/mw/com/impl/bindings/lola:runtime",
        "//platform/aas/mw/com/impl/configuration",
        "//platform/aas/mw/com/impl/tracing/configuration:tracing_filter_config",
        "@amp",
    ],
)

cc_library(
    name = "event",
    srcs = [
    ],
    hdrs = [
    ],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com/impl:__pkg__"],
    deps = [
        ":proxy_event_binding_factory",
        ":sample_allocatee_ptr",
        ":skeleton_event_binding_factory",
        "//platform/aas/mw/com/impl:instance_identifier",
        "//platform/aas/mw/com/impl:proxy_base",
        "//platform/aas/mw/com/impl:skeleton_base",
        "//platform/aas/mw/com/impl/plumbing/test:proxy_event_binding_resources",
        "//platform/aas/mw/com/impl/plumbing/test:skeleton_event_binding_resources",
        "@amp",
    ],
)

cc_library(
    name = "field",
    srcs = [
    ],
    hdrs = [
    ],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com/impl:__pkg__"],
    deps = [
        ":proxy_field_binding_factory",
        ":sample_allocatee_ptr",
        ":skeleton_field_binding_factory",
        "//platform/aas/mw/com/impl:instance_identifier",
        "//platform/aas/mw/com/impl:proxy_base",
        "//platform/aas/mw/com/impl:skeleton_base",
        "//platform/aas/mw/com/impl/plumbing/test:proxy_event_binding_resources",
        "//platform/aas/mw/com/impl/plumbing/test:skeleton_event_binding_resources",
        "@amp",
    ],
)

cc_library(
    name = "sample_allocatee_ptr",
    srcs = [
        "sample_allocatee_ptr.cpp",
    ],
    hdrs = [
        "sample_allocatee_ptr.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com/impl:__subpackages__"],
    deps = ["//platform/aas/mw/com/impl/bindings/lola:event"],
)

cc_library(
    name = "sample_ptr",
    srcs = [
        "sample_ptr.cpp",
    ],
    hdrs = [
        "sample_ptr.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com/impl:__subpackages__"],
    deps = [
        "//platform/aas/mw/com/impl:sample_reference_tracker",
        "//platform/aas/mw/com/impl/bindings/lola:event",
        "//platform/aas/mw/com/impl/bindings/mock_binding:sample_ptr",
        "@amp",
    ],
)

cc_library(
    name = "dummy_instance_identifier_builder",
    srcs = [
        "dummy_instance_identifier_builder.cpp",
    ],
    hdrs = [
        "dummy_instance_identifier_builder.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    visibility = [
        "//platform/aas/mw/com/impl/plumbing:__subpackages__",
    ],
    deps = [
        "//platform/aas/mw/com",
    ],
)

cc_test(
    name = "unit_test",
    size = "medium",
    srcs = [
        "proxy_binding_factory_test.cpp",
        "proxy_event_binding_factory_test.cpp",
        "proxy_field_binding_factory_test.cpp",
        "runtime_binding_factory_test.cpp",
        "sample_allocatee_ptr_test.cpp",
        "sample_ptr_test.cpp",
        "skeleton_binding_factory_test.cpp",
        "skeleton_event_binding_factory_test.cpp",
        "skeleton_field_binding_factory_test.cpp",
    ],
    features = COMPILER_WARNING_FEATURES + [
        "aborts_upon_exception",
    ],
    tags = ["unit"],
    visibility = ["//platform/aas/mw/com/impl:__pkg__"],
    deps = [
        ":dummy_instance_identifier_builder",
        ":event",
        ":plumbing",
        ":proxy_binding_factory_mock",
        "//platform/aas/language/safecpp/coverage_termination_handler",
        "//platform/aas/lib/os/mocklib:fcntl_mock",
        "//platform/aas/lib/utils:coverage_termination_handler",
        "//platform/aas/mw/com/impl:runtime_mock",
        "//platform/aas/mw/com/impl/bindings/lola:runtime_mock",
        "//platform/aas/mw/com/impl/bindings/lola/messaging:mock",
        "//platform/aas/mw/com/impl/bindings/lola/test:proxy_event_test_resources",
        "//platform/aas/mw/com/impl/bindings/lola/test_doubles",
        "//platform/aas/mw/com/impl/bindings/mock_binding",
        "//platform/aas/mw/com/message_passing:mock",
        "//platform/aas/mw/log",
        "//third_party/googletest:main",
    ],
)

cc_unit_test_suites_for_host_and_qnx(
    name = "unit_test_suite",
    cc_unit_tests = [
        ":unit_test",
    ],
    visibility = ["//platform/aas/mw/com/impl:__pkg__"],
)
