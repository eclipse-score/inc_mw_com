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
load("//platform/aas/bazel/generators:unit_tests.bzl", "cc_gtest_unit_test")
load("//platform/aas/mw:common_features.bzl", "COMPILER_WARNING_FEATURES")

cc_library(
    name = "flag_file",
    srcs = ["flag_file.cpp"],
    hdrs = ["flag_file.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com/impl/bindings/lola:__pkg__"],
    deps = [
        "//platform/aas/lib/filesystem",
        "//platform/aas/mw/com/impl:enriched_instance_identifier",
        "//platform/aas/mw/com/impl:instance_identifier",
        "@amp",
    ],
)

cc_gtest_unit_test(
    name = "flag_file_test",
    srcs = ["flag_file_test.cpp"],
    features = COMPILER_WARNING_FEATURES,
    deps = [
        ":flag_file",
        "//platform/aas/lib/filesystem:mock",
        "//platform/aas/lib/os:unistd",
    ],
)

cc_library(
    name = "flag_file_crawler",
    srcs = ["flag_file_crawler.cpp"],
    hdrs = ["flag_file_crawler.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com/impl/bindings/lola:__pkg__"],
    deps = [
        ":flag_file",
        ":known_instances_container",
        ":quality_aware_container",
        "//platform/aas/lib/filesystem",
        "//platform/aas/lib/os/utils/inotify:inotify_instance",
        "//platform/aas/lib/os/utils/inotify:inotify_watch_descriptor",
        "//platform/aas/mw/com/impl:instance_identifier",
        "@amp",
    ],
)

cc_gtest_unit_test(
    name = "flag_file_crawler_test",
    srcs = ["flag_file_crawler_test.cpp"],
    features = COMPILER_WARNING_FEATURES,
    deps = [
        ":flag_file_crawler",
        "//platform/aas/lib/filesystem:mock",
        "//platform/aas/lib/os/utils/inotify:inotify_instance_mock",
    ],
)

cc_library(
    name = "known_instances_container",
    srcs = ["known_instances_container.cpp"],
    hdrs = ["known_instances_container.h"],
    features = COMPILER_WARNING_FEATURES,
    implementation_deps = ["@amp"],
    visibility = ["//platform/aas/mw/com/impl/bindings/lola:__pkg__"],
    deps = [
        "//platform/aas/mw/com/impl:enriched_instance_identifier",
        "//platform/aas/mw/com/impl:handle_type",
    ],
)

cc_gtest_unit_test(
    name = "known_instances_container_test",
    srcs = ["known_instances_container_test.cpp"],
    features = COMPILER_WARNING_FEATURES,
    deps = [":known_instances_container"],
)

cc_library(
    name = "lola_service_instance_identifier",
    srcs = ["lola_service_instance_identifier.cpp"],
    hdrs = ["lola_service_instance_identifier.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com/impl/bindings/lola:__pkg__"],
    deps = [
        "//platform/aas/mw/com/impl:enriched_instance_identifier",
        "//platform/aas/mw/com/impl/configuration",
    ],
)

cc_gtest_unit_test(
    name = "lola_service_instance_identifier_test",
    srcs = ["lola_service_instance_identifier_test.cpp"],
    features = COMPILER_WARNING_FEATURES,
    deps = [":lola_service_instance_identifier"],
)

cc_library(
    name = "quality_aware_container",
    srcs = ["quality_aware_container.cpp"],
    hdrs = ["quality_aware_container.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com/impl/bindings/lola:__pkg__"],
)

test_suite(
    name = "unit_test",
    tests = [
        ":flag_file_crawler_test",
        ":flag_file_test",
        ":known_instances_container_test",
        ":lola_service_instance_identifier_test",
    ],
    visibility = ["//platform/aas/mw/com/impl/bindings/lola:__pkg__"],
)
