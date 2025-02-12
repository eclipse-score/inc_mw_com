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

load("//platform/aas/bazel/generators:unit_tests.bzl", "cc_gtest_unit_test")
load("//platform/aas/mw:common_features.bzl", "COMPILER_WARNING_FEATURES")

cc_library(
    name = "proxy_event_test_resources",
    testonly = True,
    srcs = ["proxy_event_test_resources.cpp"],
    hdrs = ["proxy_event_test_resources.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com/impl:__subpackages__"],
    deps = [
        "//platform/aas/lib/memory/shared:shared_memory_factory_mock",
        "//platform/aas/lib/os/mocklib:fcntl_mock",
        "//platform/aas/lib/os/mocklib:unistd_mock",
        "//platform/aas/mw/com/impl:runtime",
        "//platform/aas/mw/com/impl:runtime_mock",
        "//platform/aas/mw/com/impl:service_discovery_mock",
        "//platform/aas/mw/com/impl/bindings/lola:event",
        "//platform/aas/mw/com/impl/bindings/lola:proxy",
        "//platform/aas/mw/com/impl/bindings/lola/messaging:mock",
        "//platform/aas/mw/com/impl/bindings/lola/test_doubles",
        "//third_party/googletest:main",
        "@amp",
    ],
)

cc_library(
    name = "skeleton_test_resources",
    testonly = True,
    srcs = ["skeleton_test_resources.cpp"],
    hdrs = ["skeleton_test_resources.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com/impl/bindings/lola:__subpackages__"],
    deps = [
        "//platform/aas/lib/filesystem:mock",
        "//platform/aas/lib/memory/shared:shared_memory_factory_mock",
        "//platform/aas/lib/memory/shared:shared_memory_resource_heap_allocator_mock",
        "//platform/aas/lib/memory/shared:shared_memory_resource_mock",
        "//platform/aas/lib/os/mocklib:fcntl_mock",
        "//platform/aas/lib/os/mocklib:unistd_mock",
        "//platform/aas/mw/com/impl:runtime_mock",
        "//platform/aas/mw/com/impl/bindings/lola:partial_restart_path_builder_mock",
        "//platform/aas/mw/com/impl/bindings/lola:runtime_mock",
        "//platform/aas/mw/com/impl/bindings/lola:shm_path_builder_mock",
        "//platform/aas/mw/com/impl/bindings/lola:skeleton",
        "//platform/aas/mw/com/impl/configuration",
        "//third_party/googletest:main",
        "@amp",
    ],
)

cc_library(
    name = "skeleton_event_test_resources",
    testonly = True,
    srcs = ["skeleton_event_test_resources.cpp"],
    hdrs = ["skeleton_event_test_resources.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com/impl/bindings/lola:__subpackages__"],
    deps = [
        ":skeleton_test_resources",
        "//platform/aas/mw/com/impl:service_discovery_mock",
        "//platform/aas/mw/com/impl/bindings/lola:event",
        "//platform/aas/mw/com/impl/bindings/lola/messaging:mock",
        "//third_party/googletest:main",
        "@amp",
    ],
)

cc_library(
    name = "transaction_log_test_resources",
    testonly = True,
    srcs = ["transaction_log_test_resources.cpp"],
    hdrs = ["transaction_log_test_resources.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com/impl/bindings/lola:__subpackages__"],
    deps = [
        ":proxy_event_test_resources",
        "//platform/aas/mw/com/impl/bindings/lola:transaction_log",
        "//platform/aas/mw/com/impl/bindings/lola:transaction_log_id",
        "//platform/aas/mw/com/impl/bindings/lola:transaction_log_rollback_executor",
        "//platform/aas/mw/com/impl/bindings/lola:transaction_log_set",
        "//third_party/googletest:main",
    ],
)

cc_test(
    name = "skeleton_component_test",
    size = "small",
    timeout = "long",
    srcs = ["skeleton_component_test.cpp"],
    features = COMPILER_WARNING_FEATURES + [
        "aborts_upon_exception",
    ],
    tags = ["unit"],
    visibility = ["//platform/aas/mw/com/impl/bindings/lola:__subpackages__"],
    deps = [
        ":skeleton_test_resources",
        "//platform/aas/language/safecpp/coverage_termination_handler",
        "//platform/aas/lib/os/mocklib:acl_mock",
        "//platform/aas/lib/os/mocklib:fcntl_mock",
        "//platform/aas/lib/utils:coverage_termination_handler",
        "//platform/aas/mw/com/impl:runtime",
        "//platform/aas/mw/com/impl/bindings/mock_binding",
        "//third_party/googletest:main",
    ],
)

cc_test(
    name = "skeleton_event_component_test",
    size = "small",
    timeout = "long",
    srcs = ["skeleton_event_component_test.cpp"],
    features = COMPILER_WARNING_FEATURES + [
        "aborts_upon_exception",
    ],
    tags = ["unit"],
    visibility = ["//platform/aas/mw/com/impl/bindings/lola:__subpackages__"],
    deps = [
        ":skeleton_test_resources",
        "//platform/aas/language/safecpp/coverage_termination_handler",
        "//platform/aas/lib/filesystem",
        "//platform/aas/lib/os/mocklib:fcntl_mock",
        "//platform/aas/lib/utils:coverage_termination_handler",
        "//platform/aas/mw/com/impl:runtime_mock",
        "//platform/aas/mw/com/impl:service_discovery_mock",
        "//platform/aas/mw/com/impl/bindings/lola:runtime_mock",
        "//platform/aas/mw/com/impl/bindings/lola:skeleton",
        "//platform/aas/mw/com/impl/bindings/lola/messaging:mock",
        "//third_party/googletest:main",
    ],
)

cc_gtest_unit_test(
    name = "proxy_component_test",
    srcs = ["proxy_component_test.cpp"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com/impl/bindings/lola:__pkg__"],
    deps = [
        ":proxy_event_test_resources",
        "//platform/aas/lib/filesystem",
        "//platform/aas/mw/com/impl/bindings/lola:proxy",
        "//platform/aas/mw/com/impl/bindings/lola:runtime_mock",
        "//platform/aas/mw/com/impl/bindings/lola/test_doubles",
    ],
)
