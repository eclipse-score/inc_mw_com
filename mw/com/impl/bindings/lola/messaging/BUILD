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
load("//platform/aas/mw:common_features.bzl", "COMPILER_WARNING_FEATURES")

cc_library(
    name = "messaging",
    srcs = [
        "handler_base.cpp",
        "i_message_passing_control.cpp",
        "i_message_passing_service.cpp",
        "message_passing_control.cpp",
        "message_passing_facade.cpp",
        "notify_event_handler.cpp",
        "thread_abstraction.cpp",
    ],
    hdrs = [
        "handler_base.h",
        "i_message_passing_control.h",
        "i_message_passing_service.h",
        "message_passing_control.h",
        "message_passing_facade.h",
        "notify_event_handler.h",
        "thread_abstraction.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com/impl/bindings/lola:__pkg__"],
    deps = [
        "//platform/aas/lib/os:errno_logging",
        "//platform/aas/lib/os:unistd",
        "//platform/aas/mw/com/impl:binding_event_receive_handler",
        "//platform/aas/mw/com/impl/bindings/lola:element_fq_id",
        "//platform/aas/mw/com/impl/bindings/lola/messaging/messages",
        "//platform/aas/mw/com/impl/configuration",
        "//platform/aas/mw/com/message_passing",
        "//platform/aas/mw/log",
        "@amp",
    ],
)

cc_library(
    name = "mock",
    testonly = True,
    srcs = [
    ],
    hdrs = [
        "message_passing_control_mock.h",
        "message_passing_service_mock.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com/impl:__subpackages__"],
    deps = [
        ":messaging",
        "//third_party/googletest",
    ],
)

cc_test(
    name = "message_passing_unit_test",
    # TODO: Investigate why this unit tests takes more than 1 minute under memcheck ()
    timeout = "moderate",
    srcs = [
        "message_passing_control_test.cpp",
        "message_passing_facade_test.cpp",
    ],
    features = COMPILER_WARNING_FEATURES + [
        "aborts_upon_exception",
    ],
    tags = ["unit"],
    visibility = ["//platform/aas/mw/com/impl/bindings/lola:__pkg__"],
    deps = [
        ":messaging",
        ":mock",
        "//platform/aas/language/safecpp/coverage_termination_handler",
        "//platform/aas/lib/os/mocklib:unistd_mock",
        "//platform/aas/mw/com/message_passing",
        "//platform/aas/mw/com/message_passing:mock",
        "//platform/aas/mw/log",
        "//third_party/googletest:main",
    ],
)

cc_test(
    name = "other_unit_test",
    # TODO: Investigate why this unit tests takes more than 1 minute under memcheck ()
    timeout = "moderate",
    srcs = [
        "notify_event_handler_test.cpp",
    ],
    features = COMPILER_WARNING_FEATURES + [
        "aborts_upon_exception",
    ],
    tags = ["unit"],
    visibility = ["//platform/aas/mw/com/impl/bindings/lola:__pkg__"],
    deps = [
        ":messaging",
        ":mock",
        "//platform/aas/language/safecpp/coverage_termination_handler",
        "//platform/aas/mw/com/message_passing",
        "//platform/aas/mw/com/message_passing:mock",
        "//platform/aas/mw/log",
        "//third_party/googletest:main",
    ],
)
