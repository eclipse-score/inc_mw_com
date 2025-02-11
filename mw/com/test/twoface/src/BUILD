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
load("//ecu/xpad/ara/vector/tools/amsr_generators:amsrgen_tree.bzl", "ara_vector_artifacts", "cc_ara_vector_library", "filegroup_ara_vector_configs")
load("//platform/aas/bazel/packaging:adaptive_application.bzl", "pkg_adaptive_application")
load("//platform/aas/mw:common_features.bzl", "COMPILER_WARNING_FEATURES")

ara_vector_artifacts(
    name = "ara_com_amsrgen_ecu2_service",
    arxmls = [
        "//platform/aas/mw/com/test/twoface/franca/deployment/ecu2/service:arxmls_20-11",
        "//platform/aas/meta/datatypes:stdtypes",
    ],
)

ara_vector_artifacts(
    name = "ara_com_amsrgen_ecu2_client",
    arxmls = [
        "//platform/aas/mw/com/test/twoface/franca/deployment/ecu2/client:arxmls_20-11",
        "//platform/aas/meta/datatypes:stdtypes",
    ],
)

cc_ara_vector_library(
    name = "service_ipc_binding",
    application_name = "ServiceApp",
    ara_vector_artifacts = ":ara_com_amsrgen_ecu2_service",
    use_2011 = True,
)

cc_ara_vector_library(
    name = "client_ipc_binding",
    application_name = "ClientApp",
    ara_vector_artifacts = ":ara_com_amsrgen_ecu2_client",
    use_2011 = True,
)

cc_library(
    name = "lola_interface",
    hdrs = ["twoface_lola.h"],
    features = COMPILER_WARNING_FEATURES,
    deps = [
        "//platform/aas/mw/com",
    ],
)

filegroup_ara_vector_configs(
    name = "service_artifacts",
    ara_vector_artifacts = ":ara_com_amsrgen_ecu2_service",
    component_name = "ServiceApp",
    no_exec_config = True,
)

filegroup_ara_vector_configs(
    name = "client_artifacts",
    ara_vector_artifacts = ":ara_com_amsrgen_ecu2_client",
    component_name = "ClientApp",
    no_exec_config = True,
)

cc_binary(
    name = "service",
    srcs = [
        "twoface_service.cpp",
        "twoface_service.h",
    ],
    data = ["etc/mw_com_config.json"],
    features = COMPILER_WARNING_FEATURES + [
        "aborts_upon_exception",
    ],
    visibility = ["//platform/aas/mw/com/test/twoface:__pkg__"],
    deps = [
        ":lola_interface",
        ":service_ipc_binding",
        "//platform/aas/lib/aracoreinitwrapper",
        "//platform/aas/mw/com",
        "//platform/aas/mw/com/test/common_test_resources:stop_token_sig_term_handler",
        "//platform/aas/mw/log",
    ],
)

cc_binary(
    name = "client",
    srcs = [
        "twoface_client.cpp",
        "twoface_client.h",
    ],
    data = ["etc/mw_com_config.json"],
    features = COMPILER_WARNING_FEATURES + [
        "aborts_upon_exception",
    ],
    visibility = ["//platform/aas/mw/com/test/twoface:__pkg__"],
    deps = [
        ":client_ipc_binding",
        ":lola_interface",
        "//platform/aas/lib/aracoreinitwrapper",
        "//platform/aas/mw/com",
        "//platform/aas/mw/log",
    ],
)

pkg_adaptive_application(
    name = "service-pkg",
    application_name = "ServiceApp",
    bins = [":service"],
    etcs = [
        "etc/mw_com_config.json",
        ":service_artifacts",
        "etc/logging.json",
    ],
    visibility = ["//platform/aas/mw/com/test/twoface:__pkg__"],
)

pkg_adaptive_application(
    name = "client-pkg",
    application_name = "ClientApp",
    bins = [":client"],
    etcs = [
        "etc/mw_com_config.json",
        ":client_artifacts",
        "etc/logging.json",
    ],
    visibility = ["//platform/aas/mw/com/test/twoface:__pkg__"],
)
