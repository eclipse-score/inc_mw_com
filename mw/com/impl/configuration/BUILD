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

load("//bazel/tools:json_schema_validator.bzl", "validate_json_schema_test")
load("//platform/aas/bazel/generators:unit_tests.bzl", "cc_gtest_unit_test", "cc_unit_test_suites_for_host_and_qnx")
load("//platform/aas/mw:common_features.bzl", "COMPILER_WARNING_FEATURES")

validate_json_schema_test(
    name = "validate_ara_com_config_schema",
    json = "example/ara_com_config.json",
    schema = ":ara_com_config_schema",
    tags = ["lint"],
)

filegroup(
    name = "ara_com_config_schema",
    srcs = ["ara_com_config_schema.json"],
    visibility = [
        "//platform/aas/mw/com:__subpackages__",
        "//platform/aas/mw/com/impl/configuration/test:__subpackages__",
    ],
)

cc_library(
    name = "quality_type",
    srcs = ["quality_type.cpp"],
    hdrs = ["quality_type.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com/impl/bindings/lola/messages:__pkg__"],
)

cc_library(
    name = "config_parser",
    srcs = ["config_parser.cpp"],
    hdrs = ["config_parser.h"],
    features = COMPILER_WARNING_FEATURES,
    implementation_deps = [
        ":lola_service_instance_deployment",
        ":quality_type",
        ":service_type_deployment",
        ":someip_service_instance_deployment",
        "//platform/aas/lib/memory:string_literal",
        "//platform/aas/mw/log",
    ],
    deps = [
        ":configuration_local",
        "//platform/aas/lib/json:json_parser",
        "@amp",
    ],
)

cc_library(
    name = "configuration_local",
    srcs = ["configuration.cpp"],
    hdrs = ["configuration.h"],
    features = COMPILER_WARNING_FEATURES,
    deps = [
        ":global_configuration",
        ":service_identifier_type",
        ":service_instance_deployment",
        ":service_type_deployment",
        ":tracing_configuration",
        "//platform/aas/mw/com/impl:instance_specifier",
        "//platform/aas/mw/com/impl/tracing/configuration:service_element_identifier",
        "//platform/aas/mw/com/impl/tracing/configuration:tracing_config",
        "//platform/aas/mw/log",
    ],
)

cc_library(
    name = "global_configuration",
    srcs = ["global_configuration.cpp"],
    hdrs = ["global_configuration.h"],
    features = COMPILER_WARNING_FEATURES,
    implementation_deps = ["//platform/aas/mw/log"],
    deps = [
        ":quality_type",
        ":shm_size_calc_mode",
    ],
)

cc_library(
    name = "tracing_configuration",
    srcs = ["tracing_configuration.cpp"],
    hdrs = ["tracing_configuration.h"],
    features = COMPILER_WARNING_FEATURES,
    implementation_deps = ["//platform/aas/mw/log"],
    deps = [
        "//platform/aas/mw/com/impl:instance_specifier",
        "//platform/aas/mw/com/impl/tracing/configuration:service_element_identifier",
        "//platform/aas/mw/com/impl/tracing/configuration:service_element_identifier_view",
        "//platform/aas/mw/com/impl/tracing/configuration:tracing_config",
        "@amp",
    ],
)

cc_library(
    name = "configuration_error",
    srcs = ["configuration_error.cpp"],
    hdrs = ["configuration_error.h"],
    features = COMPILER_WARNING_FEATURES,
    deps = ["//platform/aas/lib/result"],
)

cc_library(
    name = "lola_event_instance_deployment",
    srcs = ["lola_event_instance_deployment.cpp"],
    hdrs = ["lola_event_instance_deployment.h"],
    features = COMPILER_WARNING_FEATURES,
    deps = [
        ":configuration_common_resources",
        "//platform/aas/lib/json:json_parser",
        "@amp",
    ],
)

cc_library(
    name = "someip_event_instance_deployment",
    srcs = ["someip_event_instance_deployment.cpp"],
    hdrs = ["someip_event_instance_deployment.h"],
    features = COMPILER_WARNING_FEATURES,
    deps = ["//platform/aas/lib/json:json_parser"],
)

cc_library(
    name = "lola_event_id",
    srcs = ["lola_event_id.cpp"],
    hdrs = ["lola_event_id.h"],
    features = COMPILER_WARNING_FEATURES,
    deps = [
        ":configuration_common_resources",
        "//platform/aas/lib/json:json_parser",
        "@amp",
    ],
)

cc_library(
    name = "lola_field_id",
    srcs = ["lola_field_id.cpp"],
    hdrs = ["lola_field_id.h"],
    features = COMPILER_WARNING_FEATURES,
    deps = [":lola_event_id"],
)

cc_library(
    name = "lola_service_id",
    srcs = ["lola_service_id.cpp"],
    hdrs = ["lola_service_id.h"],
    features = COMPILER_WARNING_FEATURES,
    deps = [
        ":configuration_common_resources",
        "//platform/aas/lib/json:json_parser",
        "@amp",
    ],
)

cc_library(
    name = "lola_field_instance_deployment",
    srcs = ["lola_field_instance_deployment.cpp"],
    hdrs = ["lola_field_instance_deployment.h"],
    features = COMPILER_WARNING_FEATURES,
    deps = [
        ":lola_event_instance_deployment",
        "@amp",
    ],
)

cc_library(
    name = "someip_field_instance_deployment",
    srcs = ["someip_field_instance_deployment.cpp"],
    hdrs = ["someip_field_instance_deployment.h"],
    features = COMPILER_WARNING_FEATURES,
    deps = ["//platform/aas/lib/json:json_parser"],
)

cc_library(
    name = "lola_service_instance_deployment",
    srcs = ["lola_service_instance_deployment.cpp"],
    hdrs = ["lola_service_instance_deployment.h"],
    features = COMPILER_WARNING_FEATURES,
    deps = [
        ":configuration_common_resources",
        ":lola_event_instance_deployment",
        ":lola_field_instance_deployment",
        ":lola_service_instance_id",
        ":quality_type",
        "//platform/aas/lib/json:json_parser",
        "//platform/aas/mw/com/impl:error",
        "@amp",
    ],
)

cc_library(
    name = "lola_service_type_deployment",
    srcs = ["lola_service_type_deployment.cpp"],
    hdrs = ["lola_service_type_deployment.h"],
    features = COMPILER_WARNING_FEATURES,
    deps = [
        ":binding_service_type_deployment",
        ":lola_event_id",
        ":lola_field_id",
        ":lola_service_id",
    ],
)

cc_library(
    name = "lola_service_instance_id",
    srcs = ["lola_service_instance_id.cpp"],
    hdrs = ["lola_service_instance_id.h"],
    features = COMPILER_WARNING_FEATURES,
    deps = [
        ":configuration_common_resources",
        "//platform/aas/lib/json:json_parser",
    ],
)

cc_library(
    name = "someip_service_instance_id",
    srcs = ["someip_service_instance_id.cpp"],
    hdrs = ["someip_service_instance_id.h"],
    features = COMPILER_WARNING_FEATURES,
    deps = [
        ":configuration_common_resources",
        "//platform/aas/lib/json:json_parser",
    ],
)

cc_library(
    name = "service_identifier_type",
    srcs = ["service_identifier_type.cpp"],
    hdrs = ["service_identifier_type.h"],
    features = COMPILER_WARNING_FEATURES,
    deps = [
        ":configuration_common_resources",
        ":service_version_type",
        "//platform/aas/lib/json:json_parser",
        "//platform/aas/lib/memory:string_literal",
        "@amp",
    ],
)

cc_library(
    name = "service_instance_deployment",
    srcs = ["service_instance_deployment.cpp"],
    hdrs = ["service_instance_deployment.h"],
    features = COMPILER_WARNING_FEATURES,
    deps = [
        ":configuration_common_resources",
        ":lola_service_instance_deployment",
        ":quality_type",
        ":service_identifier_type",
        ":someip_service_instance_deployment",
        "//platform/aas/mw/com/impl:binding_type",
        "//platform/aas/mw/com/impl:error",
        "//platform/aas/mw/com/impl:instance_specifier",
        "@amp",
    ],
)

cc_library(
    name = "service_instance_id",
    srcs = ["service_instance_id.cpp"],
    hdrs = ["service_instance_id.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com/impl:__subpackages__"],
    deps = [
        ":configuration_common_resources",
        ":lola_service_instance_id",
        ":someip_service_instance_id",
        "//platform/aas/lib/json:json_parser",
        "@amp",
    ],
)

cc_library(
    name = "service_type_deployment",
    srcs = ["service_type_deployment.cpp"],
    hdrs = ["service_type_deployment.h"],
    features = COMPILER_WARNING_FEATURES,
    deps = [
        ":configuration_common_resources",
        ":lola_service_type_deployment",
        "//platform/aas/lib/json:json_parser",
        "@amp",
    ],
)

cc_library(
    name = "service_version_type",
    srcs = ["service_version_type.cpp"],
    hdrs = ["service_version_type.h"],
    features = COMPILER_WARNING_FEATURES,
    deps = [
        ":configuration_common_resources",
        "//platform/aas/lib/json:json_parser",
        "@amp",
    ],
)

cc_library(
    name = "someip_service_instance_deployment",
    srcs = ["someip_service_instance_deployment.cpp"],
    hdrs = ["someip_service_instance_deployment.h"],
    features = COMPILER_WARNING_FEATURES,
    deps = [
        ":configuration_common_resources",
        ":someip_event_instance_deployment",
        ":someip_field_instance_deployment",
        ":someip_service_instance_id",
        "//platform/aas/lib/json:json_parser",
        "//platform/aas/lib/result",
        "@amp",
    ],
)

cc_library(
    name = "binding_service_type_deployment",
    srcs = ["binding_service_type_deployment.cpp"],
    hdrs = ["binding_service_type_deployment.h"],
    features = COMPILER_WARNING_FEATURES,
    deps = [
        ":configuration_common_resources",
        "//platform/aas/lib/json:json_parser",
    ],
)

cc_library(
    name = "shm_size_calc_mode",
    srcs = ["shm_size_calc_mode.cpp"],
    hdrs = ["shm_size_calc_mode.h"],
    features = COMPILER_WARNING_FEATURES,
)

cc_library(
    name = "configuration_common_resources",
    srcs = ["configuration_common_resources.cpp"],
    hdrs = ["configuration_common_resources.h"],
    features = COMPILER_WARNING_FEATURES,
    deps = [
        "//platform/aas/lib/json:json_parser",
        "@amp",
    ],
)

cc_library(
    name = "configuration",
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com/impl:__subpackages__"],
    deps = [
        ":config_parser",
        ":configuration_error",
        ":configuration_local",
        ":lola_event_id",
        ":lola_event_instance_deployment",
        ":lola_service_instance_deployment",
        ":lola_service_type_deployment",
        ":service_identifier_type",
        ":service_instance_deployment",
        ":service_instance_id",
        ":service_type_deployment",
        ":service_version_type",
        ":shm_size_calc_mode",
        ":someip_service_instance_deployment",
    ],
)

cc_test(
    name = "config_parser_test",
    size = "small",
    timeout = "long",
    srcs = ["config_parser_test.cpp"],
    data = ["example/ara_com_config.json"],
    features = COMPILER_WARNING_FEATURES + [
        "aborts_upon_exception",
    ],
    tags = ["unit"],
    deps = [
        ":config_parser",
        "//platform/aas/language/safecpp/coverage_termination_handler",
        "//third_party/googletest:main",
    ],
)

cc_gtest_unit_test(
    name = "configuration_error_test",
    srcs = ["configuration_error_test.cpp"],
    features = COMPILER_WARNING_FEATURES,
    deps = [":configuration_error"],
)

cc_gtest_unit_test(
    name = "configuration_test",
    srcs = ["configuration_test.cpp"],
    data = ["example/ara_com_config.json"],
    features = COMPILER_WARNING_FEATURES,
    deps = [
        ":config_parser",
        ":configuration_local",
        "@amp",
    ],
)

cc_gtest_unit_test(
    name = "lola_service_instance_deployment_test",
    srcs = ["lola_service_instance_deployment_test.cpp"],
    features = COMPILER_WARNING_FEATURES,
    deps = [
        ":lola_service_instance_deployment",
        "//platform/aas/mw/com/impl/configuration/test:configuration_test_resources",
        "@amp",
    ],
)

cc_gtest_unit_test(
    name = "lola_event_instance_deployment_test",
    srcs = ["lola_event_instance_deployment_test.cpp"],
    features = COMPILER_WARNING_FEATURES,
    deps = [
        ":lola_event_instance_deployment",
        "//platform/aas/mw/com/impl/configuration/test:configuration_test_resources",
    ],
)

cc_gtest_unit_test(
    name = "lola_field_instance_deployment_test",
    srcs = ["lola_field_instance_deployment_test.cpp"],
    features = COMPILER_WARNING_FEATURES,
    deps = [
        ":lola_field_instance_deployment",
        "//platform/aas/mw/com/impl/configuration/test:configuration_test_resources",
    ],
)

cc_gtest_unit_test(
    name = "quality_type_test",
    srcs = ["quality_type_test.cpp"],
    features = COMPILER_WARNING_FEATURES,
    deps = [":quality_type"],
)

cc_gtest_unit_test(
    name = "service_identifier_type_test",
    srcs = ["service_identifier_type_test.cpp"],
    features = COMPILER_WARNING_FEATURES,
    deps = [
        ":service_identifier_type",
        "//platform/aas/mw/com/impl/configuration/test:configuration_test_resources",
    ],
)

cc_gtest_unit_test(
    name = "service_instance_deployment_test",
    srcs = ["service_instance_deployment_test.cpp"],
    features = COMPILER_WARNING_FEATURES,
    deps = [
        ":service_instance_deployment",
        "//platform/aas/mw/com/impl/configuration/test:configuration_test_resources",
    ],
)

cc_gtest_unit_test(
    name = "service_version_type_test",
    srcs = ["service_version_type_test.cpp"],
    features = COMPILER_WARNING_FEATURES,
    deps = [
        ":service_version_type",
        "//platform/aas/mw/com/impl/configuration/test:configuration_test_resources",
    ],
)

cc_gtest_unit_test(
    name = "service_type_deployment_test",
    srcs = ["service_type_deployment_test.cpp"],
    features = COMPILER_WARNING_FEATURES,
    deps = [
        ":service_type_deployment",
        "//platform/aas/mw/com/impl/configuration/test:configuration_test_resources",
    ],
)

cc_gtest_unit_test(
    name = "someip_service_instance_deployment_test",
    srcs = ["someip_service_instance_deployment_test.cpp"],
    features = COMPILER_WARNING_FEATURES,
    deps = [
        ":someip_service_instance_deployment",
        "//platform/aas/mw/com/impl/configuration/test:configuration_test_resources",
    ],
)

cc_gtest_unit_test(
    name = "lola_service_type_deployment_test",
    srcs = ["lola_service_type_deployment_test.cpp"],
    features = COMPILER_WARNING_FEATURES,
    deps = [
        ":lola_service_type_deployment",
        "//platform/aas/mw/com/impl/configuration/test:configuration_test_resources",
    ],
)

cc_gtest_unit_test(
    name = "service_instance_id_test",
    srcs = ["service_instance_id_test.cpp"],
    features = COMPILER_WARNING_FEATURES,
    deps = [
        ":service_instance_id",
        "//platform/aas/mw/com/impl/configuration/test:configuration_test_resources",
    ],
)

cc_gtest_unit_test(
    name = "lola_service_instance_id_test",
    srcs = ["lola_service_instance_id_test.cpp"],
    features = COMPILER_WARNING_FEATURES,
    deps = [
        ":lola_service_instance_id",
        "//platform/aas/mw/com/impl/configuration/test:configuration_test_resources",
    ],
)

cc_gtest_unit_test(
    name = "someip_service_instance_id_test",
    srcs = ["someip_service_instance_id_test.cpp"],
    features = COMPILER_WARNING_FEATURES,
    deps = [
        ":someip_service_instance_deployment",
        "//platform/aas/mw/com/impl/configuration/test:configuration_test_resources",
    ],
)

cc_gtest_unit_test(
    name = "global_configuration_test",
    srcs = ["global_configuration_test.cpp"],
    features = COMPILER_WARNING_FEATURES,
    deps = [
        ":global_configuration",
    ],
)

cc_gtest_unit_test(
    name = "tracing_configuration_test",
    srcs = ["tracing_configuration_test.cpp"],
    features = COMPILER_WARNING_FEATURES,
    deps = [
        ":tracing_configuration",
    ],
)

cc_unit_test_suites_for_host_and_qnx(
    name = "unit_test_suite",
    cc_unit_tests = [
        ":config_parser_test",
        ":configuration_error_test",
        ":configuration_test",
        ":global_configuration_test",
        ":lola_event_instance_deployment_test",
        ":lola_field_instance_deployment_test",
        ":lola_service_instance_deployment_test",
        ":lola_service_instance_id_test",
        ":lola_service_type_deployment_test",
        ":quality_type_test",
        ":service_identifier_type_test",
        ":service_instance_deployment_test",
        ":service_instance_id_test",
        ":service_type_deployment_test",
        ":service_version_type_test",
        ":someip_service_instance_deployment_test",
        ":someip_service_instance_id_test",
        ":tracing_configuration_test",
    ],
    visibility = ["//platform/aas/mw/com/impl:__pkg__"],
)

filegroup(
    name = "ara_com_config.json",
    srcs = ["example/ara_com_config.json"],
    visibility = ["//visibility:public"],
)

filegroup(
    name = "ara_com_config_other.json",
    srcs = ["example/ara_com_config_other.json"],
    visibility = ["//visibility:public"],
)
