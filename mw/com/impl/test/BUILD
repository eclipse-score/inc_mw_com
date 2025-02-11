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
    name = "binding_factory_resources",
    testonly = True,
    srcs = ["binding_factory_resources.cpp"],
    hdrs = ["binding_factory_resources.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com/impl:__subpackages__"],
    deps = [
        "//platform/aas/mw/com/impl",
        "//platform/aas/mw/com/impl/bindings/mock_binding",
        "//platform/aas/mw/com/impl/plumbing:proxy_binding_factory_mock",
        "//platform/aas/mw/com/impl/plumbing:proxy_event_binding_factory_mock",
        "//platform/aas/mw/com/impl/plumbing:proxy_field_binding_factory_mock",
        "//platform/aas/mw/com/impl/plumbing:skeleton_binding_factory_mock",
        "//platform/aas/mw/com/impl/plumbing:skeleton_event_binding_factory_mock",
        "//platform/aas/mw/com/impl/plumbing:skeleton_field_binding_factory_mock",
        "@amp",
    ],
)

cc_library(
    name = "proxy_resources",
    testonly = True,
    srcs = ["proxy_resources.cpp"],
    hdrs = ["proxy_resources.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com/impl:__subpackages__"],
    deps = [
        "//platform/aas/mw/com/impl",
        "//platform/aas/mw/com/impl/bindings/mock_binding",
        "@amp",
    ],
)
