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
    name = "tracing_runtime",
    testonly = True,
    srcs = ["tracing_runtime.cpp"],
    hdrs = ["tracing_runtime.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = [
        "//platform/aas/mw/com/impl:__pkg__",
        "//platform/aas/mw/com/impl/bindings/lola:__subpackages__",
        "//platform/aas/mw/com/impl/plumbing:__pkg__",
        "//platform/aas/mw/com/impl/test:__pkg__",
        "//platform/aas/mw/com/impl/tracing:__pkg__",
    ],
    deps = [
        "//platform/aas/mw/com/impl/tracing:i_tracing_runtime_binding",
        "//third_party/googletest",
    ],
)
