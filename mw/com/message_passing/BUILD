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
load("//platform/aas/tools/itf/bazel_gen:itf_gen.bzl", "py_unittest_qnx_test")

alias(
    name = "message_passing",
    actual = select({
        "@platforms//os:qnx": ":message_passing_resmgr",
        "//conditions:default": ":message_passing_mqueue",
    }),
    tags = ["FFI"],
    visibility = [
        "//platform/aas/mw/com/impl:__subpackages__",
        "//platform/aas/mw/com/message_passing/test:__pkg__",
        "//platform/aas/mw/log:__subpackages__",
        "//platform/aas/pas/logging:__subpackages__",
    ],
)

cc_library(
    name = "message",
    srcs = ["message.cpp"],
    hdrs = ["message.h"],
    features = COMPILER_WARNING_FEATURES,
    tags = ["FFI"],
)

cc_library(
    name = "shared_properties",
    srcs = ["shared_properties.cpp"],
    hdrs = ["shared_properties.h"],
    features = COMPILER_WARNING_FEATURES,
    tags = ["FFI"],
    deps = [
        ":message",
        "@amp",
    ],
)

cc_library(
    name = "serializer",
    srcs = ["serializer.cpp"],
    hdrs = ["serializer.h"],
    features = COMPILER_WARNING_FEATURES,
    tags = ["FFI"],
    deps = [
        ":message",
        ":shared_properties",
        "@amp",
    ],
)

filegroup(
    name = "common_hdrs",
    srcs = [
        "non_blocking_sender.h",
        "receiver.h",
        "receiver_config.h",
        "receiver_factory.h",
        "receiver_factory_impl.h",
        "sender.h",
        "sender_config.h",
        "sender_factory.h",
        "sender_factory_impl.h",
    ],
)

filegroup(
    name = "common_srcs",
    srcs = [
        "non_blocking_sender.cpp",
        "receiver_factory.cpp",
        "sender_factory.cpp",
    ],
)

cc_library(
    name = "message_passing_mqueue",
    srcs = [
        "mqueue/mqueue_receiver_factory.cpp",
        "mqueue/mqueue_receiver_traits.cpp",
        "mqueue/mqueue_receiver_traits.h",
        "mqueue/mqueue_sender_factory.cpp",
        "mqueue/mqueue_sender_traits.cpp",
        "mqueue/mqueue_sender_traits.h",
        ":common_srcs",
    ],
    hdrs = [
        ":common_hdrs",
    ],
    features = COMPILER_WARNING_FEATURES,
    visibility = [
        "//platform/aas/mw/com/impl:__subpackages__",
        "//platform/aas/mw/com/message_passing/test:__pkg__",
    ],
    deps = [
        ":interface",
        ":message",
        ":serializer",
        ":shared_properties",
        "//platform/aas/lib/concurrency",
        "//platform/aas/lib/memory:pmr_ring_buffer",
        "//platform/aas/lib/os:mqueue",
        "//platform/aas/lib/os:stat",
        "//platform/aas/lib/os:unistd",
        "@amp",
    ],
)

cc_library(
    name = "message_passing_resmgr",
    srcs = [
        "qnx/resmgr_receiver_factory.cpp",
        "qnx/resmgr_receiver_traits.cpp",
        "qnx/resmgr_receiver_traits.h",
        "qnx/resmgr_sender_factory.cpp",
        "qnx/resmgr_sender_traits.cpp",
        "qnx/resmgr_sender_traits.h",
        "qnx/resmgr_traits_common.cpp",
        "qnx/resmgr_traits_common.h",
        ":common_srcs",
    ],
    hdrs = [
        ":common_hdrs",
    ],
    features = COMPILER_WARNING_FEATURES,
    tags = ["FFI"],
    visibility = [
        "//platform/aas/mw/com/impl:__subpackages__",
        "//platform/aas/mw/com/message_passing/test:__pkg__",
    ],
    deps = [
        ":interface",
        ":message",
        ":serializer",
        ":shared_properties",
        "//platform/aas/lib/concurrency",
        "//platform/aas/lib/memory:pmr_ring_buffer",
        "//platform/aas/lib/os:fcntl",
        "//platform/aas/lib/os:unistd",
        "//platform/aas/lib/os/qnx:channel",
        "//platform/aas/lib/os/qnx:dispatch",
        "//platform/aas/lib/os/qnx:iofunc",
        "@amp",
    ],
)

cc_library(
    name = "interface",
    hdrs = [
        "i_receiver.h",
        "i_sender.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    tags = ["FFI"],
    visibility = [
        "//platform/aas/mw/com/impl:__subpackages__",
        "//platform/aas/mw/com/message_passing:__subpackages__",
    ],
    deps = [
        "//platform/aas/lib/os:errno",
        "@amp",
    ],
)

cc_library(
    name = "mock",
    testonly = True,
    hdrs = [
        "receiver_mock.h",
        "receiver_traits_mock.h",
        "sender_mock.h",
        "sender_traits_mock.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    visibility = [
        "//platform/aas/mw/com/impl:__subpackages__",
        "//platform/aas/mw/log:__subpackages__",
        "//platform/aas/pas/logging:__subpackages__",
    ],
    deps = [
        ":interface",
        "//third_party/googletest",
    ],
)

cc_unit_test_suites_for_host_and_qnx(
    name = "unit_test_suite",
    cc_unit_tests = [
        ":unit_test",
    ],
    visibility = ["//platform/aas/mw/com:__pkg__"],
)

cc_gtest_unit_test(
    name = "unit_test",
    srcs = [
        "non_blocking_sender_test.cpp",
        "receiver_test.cpp",
        "sender_factory_test.cpp",
        "sender_test.cpp",
        "serializer_test.cpp",
    ],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com:__pkg__"],
    deps = [
        ":message_passing",
        ":mock",
        "//platform/aas/lib/concurrency:executor_mock",
        "//platform/aas/lib/os/mocklib:mqueue_mock",
        "//platform/aas/lib/os/mocklib:unistd_mock",
        "//platform/aas/mw/log/test/stream_capture",
    ],
)

py_unittest_qnx_test(
    name = "unit_tests_qnx",
    test_cases = [
        ":resmgr_factory_test",
        ":resmgr_traits_test",
    ],
    visibility = ["//platform/aas/mw:__subpackages__"],
)

cc_library(
    name = "resmgr_traits_for_testing",
    testonly = True,
    srcs = [
        "qnx/resmgr_receiver_traits.cpp",
        "qnx/resmgr_sender_traits.cpp",
        "qnx/resmgr_traits_common.cpp",
    ],
    hdrs = [
        "qnx/resmgr_receiver_traits.h",
        "qnx/resmgr_sender_traits.h",
        "qnx/resmgr_traits_common.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    target_compatible_with = ["@platforms//os:qnx"],
    deps = [
        ":message",
        "//platform/aas/lib/memory:pmr_ring_buffer",
        "//platform/aas/lib/os:fcntl",
        "//platform/aas/lib/os:unistd",
        "//platform/aas/lib/os/qnx:channel",
        "//platform/aas/lib/os/qnx:dispatch",
        "//platform/aas/lib/os/qnx:iofunc",
        "@amp",
    ],
)

cc_gtest_unit_test(
    name = "resmgr_traits_test",
    srcs = [
        "resmgr_receiver_traits_test.cpp",
        "resmgr_sender_traits_test.cpp",
    ],
    features = COMPILER_WARNING_FEATURES,
    target_compatible_with = ["@platforms//os:qnx"],
    deps = [
        ":resmgr_traits_for_testing",
        "//platform/aas/lib/os/mocklib:fcntl_mock",
        "//platform/aas/lib/os/mocklib:unistd_mock",
        "//platform/aas/lib/os/mocklib/qnx:channel_mock",
        "//platform/aas/lib/os/mocklib/qnx:dispatch_mock",
        "//platform/aas/lib/os/mocklib/qnx:iofunc_mock",
    ],
)

cc_gtest_unit_test(
    name = "resmgr_factory_test",
    srcs = [
        "qnx/resmgr_receiver_factory.cpp",
        "qnx/resmgr_sender_factory.cpp",
        "resmgr_factory_test.cpp",
        ":common_hdrs",
        ":common_srcs",
    ],
    features = COMPILER_WARNING_FEATURES,
    target_compatible_with = ["@platforms//os:qnx"],
    deps = [
        "serializer",
        ":interface",
        ":message",
        ":resmgr_traits_for_testing",
        ":shared_properties",
        "//platform/aas/lib/concurrency:interruptible_wait",
        "//platform/aas/lib/concurrency:thread_pool",
        "//platform/aas/lib/os/mocklib:fcntl_mock",
        "//platform/aas/lib/os/mocklib:unistd_mock",
        "//platform/aas/lib/os/mocklib/qnx:channel_mock",
        "//platform/aas/lib/os/mocklib/qnx:dispatch_mock",
        "//platform/aas/lib/os/mocklib/qnx:iofunc_mock",
    ],
)
