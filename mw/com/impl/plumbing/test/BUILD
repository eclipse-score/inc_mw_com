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
    name = "proxy_event_binding_resources",
    srcs = ["proxy_event_binding_resources.cpp"],
    hdrs = ["proxy_event_binding_resources.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com/impl/plumbing:__subpackages__"],
    deps = [
        "//platform/aas/mw/com/impl:proxy_event_binding",
        "//platform/aas/mw/com/impl/bindings/lola:element_fq_id",
        "//platform/aas/mw/com/impl/configuration",
        "@amp",
    ],
)

cc_library(
    name = "skeleton_event_binding_resources",
    srcs = ["skeleton_event_binding_resources.cpp"],
    hdrs = ["skeleton_event_binding_resources.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com/impl/plumbing:__subpackages__"],
    deps = [],
)
