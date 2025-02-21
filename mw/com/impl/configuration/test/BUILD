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
load("//platform/aas/mw:common_features.bzl", "COMPILER_WARNING_FEATURES")

cc_library(
    name = "configuration_test_resources",
    testonly = True,
    srcs = ["configuration_test_resources.cpp"],
    hdrs = ["configuration_test_resources.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com/impl:__subpackages__"],
    deps = [
        "//platform/aas/mw/com/impl/configuration",
        "//third_party/googletest:main",
        "@amp",
    ],
)

validate_json_schema_test(
    name = "invalid_json_wrong_consumer_name",
    expected_failure = True,
    json = "invalid_json_consumer_name.json",
    schema = "//platform/aas/mw/com/impl/configuration:ara_com_config_schema",
    tags = ["lint"],
)

validate_json_schema_test(
    name = "invalid_json_wrong_provider_name",
    expected_failure = True,
    json = "invalid_json_provider_name.json",
    schema = "//platform/aas/mw/com/impl/configuration:ara_com_config_schema",
    tags = ["lint"],
)

validate_json_schema_test(
    name = "invalid_json_unknown_permission_check_value",
    expected_failure = True,
    json = "invalid_json_unknown_permission_check_value.json",
    schema = "//platform/aas/mw/com/impl/configuration:ara_com_config_schema",
    tags = ["lint"],
)

validate_json_schema_test(
    name = "invalid_json_incomplete_service_type",
    expected_failure = True,
    json = "invalid_json_incomplete_service_type.json",
    schema = "//platform/aas/mw/com/impl/configuration:ara_com_config_schema",
    tags = ["lint"],
)

validate_json_schema_test(
    name = "invalid_json_incomplete_binding",
    expected_failure = True,
    json = "invalid_json_incomplete_binding.json",
    schema = "//platform/aas/mw/com/impl/configuration:ara_com_config_schema",
    tags = ["lint"],
)

validate_json_schema_test(
    name = "invalid_json_incomplete_event",
    expected_failure = True,
    json = "invalid_json_incomplete_event.json",
    schema = "//platform/aas/mw/com/impl/configuration:ara_com_config_schema",
    tags = ["lint"],
)

validate_json_schema_test(
    name = "invalid_json_incomplete_field",
    expected_failure = True,
    json = "invalid_json_incomplete_field.json",
    schema = "//platform/aas/mw/com/impl/configuration:ara_com_config_schema",
    tags = ["lint"],
)

validate_json_schema_test(
    name = "invalid_json_incomplete_service_instance",
    expected_failure = True,
    json = "invalid_json_incomplete_service_instance.json",
    schema = "//platform/aas/mw/com/impl/configuration:ara_com_config_schema",
    tags = ["lint"],
)

validate_json_schema_test(
    name = "invalid_json_incomplete_instance",
    expected_failure = True,
    json = "invalid_json_incomplete_instance.json",
    schema = "//platform/aas/mw/com/impl/configuration:ara_com_config_schema",
    tags = ["lint"],
)

validate_json_schema_test(
    name = "invalid_json_incomplete_instance_event",
    expected_failure = True,
    json = "invalid_json_incomplete_instance_event.json",
    schema = "//platform/aas/mw/com/impl/configuration:ara_com_config_schema",
    tags = ["lint"],
)

validate_json_schema_test(
    name = "invalid_json_incomplete_instance_field",
    expected_failure = True,
    json = "invalid_json_incomplete_instance_field.json",
    schema = "//platform/aas/mw/com/impl/configuration:ara_com_config_schema",
    tags = ["lint"],
)
