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
load("//platform/aas/bazel/packaging:adaptive_application.bzl", "pkg_adaptive_application")
load("//platform/aas/mw:common_features.bzl", "COMPILER_WARNING_FEATURES")

cc_binary(
    name = "bigdata",
    srcs = [
        "big_datatype_application.cpp",
        "big_datatype_application.h",
    ],
    data = ["mw_com_config.json"],
    features = COMPILER_WARNING_FEATURES + [
        "aborts_upon_exception",
    ],
    deps = [
        "//platform/aas/mw/com",
        "//platform/aas/mw/com/test/common_test_resources:assert_handler",
        "//platform/aas/mw/com/test/common_test_resources:sample_sender_receiver",
        "//platform/aas/mw/com/test/common_test_resources:sctf_test_runner",
    ],
)

pkg_adaptive_application(
    name = "bigdata-pkg",
    application_name = "bigdata",
    bins = [":bigdata"],
    etcs = [
        "logging.json",
        "mw_com_config.json",
        "//platform/aas/mw/com/test/bigdata/franca:config_files",
    ],
    visibility = [
        "//platform/aas/mw/com/test/bigdata/sct:__pkg__",
        "//platform/aas/test/mw/com:__pkg__",
    ],
)
