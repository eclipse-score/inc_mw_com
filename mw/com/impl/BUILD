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
load("//platform/aas/bazel/generators:unit_tests.bzl", "cc_gtest_unit_test", "cc_unit_test_suites_for_host_and_qnx")
load("//platform/aas/mw:common_features.bzl", "COMPILER_WARNING_FEATURES")

cc_library(
    name = "impl",
    srcs = [
        "generic_proxy.cpp",
        "generic_proxy_event.cpp",
        "proxy_event.cpp",
        "proxy_field.cpp",
        "skeleton_event.cpp",
        "skeleton_field.cpp",
        "traits.cpp",
    ],
    hdrs = [
        "generic_proxy.h",
        "generic_proxy_event.h",
        "proxy_event.h",
        "proxy_field.h",
        "skeleton_event.h",
        "skeleton_field.h",
        "traits.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    visibility = [
        "//platform/aas/mw/com:__pkg__",
        "//platform/aas/mw/com/impl/test:__pkg__",
        "//platform/aas/mw/com/impl/test/uat:__pkg__",
        "//platform/aas/mw/com/impl/tracing:__pkg__",
    ],
    deps = [
        ":error",
        ":generic_proxy_event_binding",
        ":instance_identifier",
        ":proxy_base",
        ":proxy_event_base",
        ":proxy_event_binding",
        ":runtime",
        ":service_element_map",
        ":skeleton_base",
        ":skeleton_binding",
        ":skeleton_event_base",
        ":skeleton_event_binding",
        ":skeleton_field_base",
        "//platform/aas/mw/com/impl/configuration",
        "//platform/aas/mw/com/impl/plumbing:event",
        "//platform/aas/mw/com/impl/plumbing:field",
        "//platform/aas/mw/com/impl/tracing:proxy_event_tracing",
        "//platform/aas/mw/com/impl/tracing:skeleton_event_tracing",
        "//platform/aas/mw/log",
        "@amp",
    ],
)

cc_library(
    name = "skeleton_event_base",
    srcs = ["skeleton_event_base.cpp"],
    hdrs = ["skeleton_event_base.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = [
        "//platform/aas/mw/com/impl/tracing:__subpackages__",
    ],
    deps = [
        ":flag_owner",
        ":skeleton_event_binding",
        "//platform/aas/lib/result",
        "//platform/aas/mw/com/impl/tracing:skeleton_event_tracing",
        "//platform/aas/mw/com/impl/tracing:skeleton_event_tracing_data",
        "@amp",
    ],
)

cc_library(
    name = "skeleton_field_base",
    srcs = ["skeleton_field_base.cpp"],
    hdrs = ["skeleton_field_base.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = [
        "//platform/aas/mw/com/impl/tracing:__subpackages__",
    ],
    deps = [
        ":error",
        ":skeleton_event_base",
        "//platform/aas/lib/result",
        "//platform/aas/mw/log",
        "@amp",
    ],
)

cc_library(
    name = "skeleton_base",
    srcs = ["skeleton_base.cpp"],
    hdrs = ["skeleton_base.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = [
        "//platform/aas/mw/com/impl/plumbing:__pkg__",
    ],
    deps = [
        ":flag_owner",
        ":instance_identifier",
        ":runtime",
        ":skeleton_binding",
        ":skeleton_event_base",
        ":skeleton_field_base",
        "//platform/aas/mw/com/impl/plumbing",
        "//platform/aas/mw/com/impl/tracing:skeleton_tracing",
        "@amp",
    ],
)

cc_library(
    name = "find_service_handle",
    srcs = ["find_service_handle.cpp"],
    hdrs = ["find_service_handle.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com:__pkg__"],
)

cc_library(
    name = "find_service_handler",
    srcs = ["find_service_handler.cpp"],
    hdrs = ["find_service_handler.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com:__pkg__"],
    deps = [
        ":find_service_handle",
        "@amp",
    ],
)

cc_library(
    name = "proxy_base",
    srcs = ["proxy_base.cpp"],
    hdrs = ["proxy_base.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = [
        "//platform/aas/mw/com/impl/plumbing:__pkg__",
    ],
    deps = [
        ":find_service_handle",
        ":find_service_handler",
        ":runtime",
        "//platform/aas/lib/result",
        "//platform/aas/mw/com/impl/plumbing",
        "@amp",
    ],
)

cc_library(
    name = "proxy_event_base",
    srcs = ["proxy_event_base.cpp"],
    hdrs = ["proxy_event_base.h"],
    features = COMPILER_WARNING_FEATURES,
    implementation_deps = [
        "//platform/aas/mw/com/impl/tracing:proxy_event_tracing",
    ],
    visibility = ["//platform/aas/mw/com/impl:__pkg__"],
    deps = [
        ":error",
        ":proxy_base",
        ":proxy_event_binding",
        ":sample_reference_tracker",
        "//platform/aas/language/safecpp/scoped_function:scope",
        "//platform/aas/mw/com/impl/tracing:proxy_event_tracing_data",
        "@amp",
    ],
)

cc_library(
    name = "subscription_state",
    srcs = ["subscription_state.cpp"],
    hdrs = ["subscription_state.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com:__pkg__"],
)

cc_library(
    name = "skeleton_binding",
    srcs = [
        "skeleton_binding.cpp",
    ],
    hdrs = [
        "skeleton_binding.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    visibility = [
        "//platform/aas/mw/com/impl/bindings/lola:__pkg__",
        "//platform/aas/mw/com/impl/bindings/mock_binding:__pkg__",
        "//platform/aas/mw/com/impl/plumbing:__subpackages__",
        "//platform/aas/mw/com/impl/tracing:__subpackages__",
    ],
    deps = [
        ":binding_type",
        "//platform/aas/lib/memory/shared:i_shared_memory_resource",
        "//platform/aas/lib/result",
        "//platform/aas/mw/com/impl/configuration",
        "@amp",
    ],
)

cc_library(
    name = "skeleton_event_binding",
    srcs = [
        "skeleton_event_binding.cpp",
    ],
    hdrs = [
        "skeleton_event_binding.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    visibility = [
        "//platform/aas/mw/com/impl/bindings/lola:__pkg__",
        "//platform/aas/mw/com/impl/bindings/mock_binding:__pkg__",
        "//platform/aas/mw/com/impl/plumbing:__subpackages__",
        "//platform/aas/mw/com/impl/tracing:__subpackages__",
    ],
    deps = [
        ":binding_type",
        "//platform/aas/lib/result",
        "//platform/aas/mw/com/impl/plumbing:sample_allocatee_ptr",
        "//platform/aas/mw/com/impl/tracing:skeleton_event_tracing_data",
        "@amp",
    ],
)

cc_library(
    name = "proxy_binding",
    srcs = [
        "proxy_binding.cpp",
    ],
    hdrs = [
        "proxy_binding.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    visibility = [
        "//platform/aas/mw/com/impl/bindings/lola:__pkg__",
        "//platform/aas/mw/com/impl/bindings/mock_binding:__pkg__",
        "//platform/aas/mw/com/impl/plumbing:__subpackages__",
        "//platform/aas/mw/com/impl/tracing:__subpackages__",
    ],
    deps = [
        "proxy_event_binding",
        "@amp",
    ],
)

cc_library(
    name = "proxy_event_binding",
    srcs = [
        "proxy_event_binding.cpp",
        "proxy_event_binding_base.cpp",
    ],
    hdrs = [
        "proxy_event_binding.h",
        "proxy_event_binding_base.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    visibility = [
        "//platform/aas/mw/com/impl/bindings/lola:__pkg__",
        "//platform/aas/mw/com/impl/bindings/mock_binding:__pkg__",
        "//platform/aas/mw/com/impl/plumbing:__subpackages__",
        "//platform/aas/mw/com/impl/tracing:__subpackages__",
    ],
    deps = [
        ":binding_event_receive_handler",
        ":binding_type",
        ":sample_reference_tracker",
        ":subscription_state",
        "//platform/aas/lib/result",
        "//platform/aas/mw/com/impl/configuration",
        "//platform/aas/mw/com/impl/plumbing:sample_ptr",
        "//platform/aas/mw/com/impl/tracing:i_tracing_runtime",
        "@amp",
    ],
)

cc_library(
    name = "generic_proxy_event_binding",
    srcs = [
        "generic_proxy_event_binding.cpp",
    ],
    hdrs = [
        "generic_proxy_event_binding.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    visibility = [
        "//platform/aas/mw/com/impl/bindings/lola:__pkg__",
        "//platform/aas/mw/com/impl/bindings/mock_binding:__pkg__",
        "//platform/aas/mw/com/impl/plumbing:__subpackages__",
        "//platform/aas/mw/com/impl/tracing:__subpackages__",
    ],
    deps = [
        ":binding_event_receive_handler",
        ":binding_type",
        ":proxy_event_binding",
        ":subscription_state",
        "//platform/aas/mw/com/impl/configuration",
        "//platform/aas/mw/com/impl/plumbing:sample_ptr",
        "//platform/aas/mw/com/impl/tracing:i_tracing_runtime",
    ],
)

cc_library(
    name = "handle_type",
    srcs = ["handle_type.cpp"],
    hdrs = ["handle_type.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com/impl:__subpackages__"],
    deps = [
        ":instance_identifier",
        "//platform/aas/mw/com/impl/configuration",
        "@amp",
    ],
)

cc_library(
    name = "instance_identifier",
    srcs = ["instance_identifier.cpp"],
    hdrs = ["instance_identifier.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com/impl:__subpackages__"],
    deps = [
        ":error",
        "//platform/aas/mw/com/impl/configuration",
    ],
)

cc_library(
    name = "enriched_instance_identifier",
    srcs = ["enriched_instance_identifier.cpp"],
    hdrs = ["enriched_instance_identifier.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com/impl:__subpackages__"],
    deps = [
        ":handle_type",
        ":instance_identifier",
        "//platform/aas/mw/com/impl/configuration:service_instance_id",
        "@amp",
    ],
)

cc_library(
    name = "error",
    srcs = ["com_error.cpp"],
    hdrs = ["com_error.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com:__subpackages__"],
    deps = ["//platform/aas/lib/result"],
)

cc_library(
    name = "instance_specifier",
    srcs = ["instance_specifier.cpp"],
    hdrs = ["instance_specifier.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com:__subpackages__"],
    deps = [
        ":error",
        "//platform/aas/lib/result",
        "//platform/aas/mw/log",
        "@amp",
    ],
)

cc_library(
    name = "i_service_discovery_client",
    srcs = ["i_service_discovery_client.cpp"],
    hdrs = ["i_service_discovery_client.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com:__subpackages__"],
    deps = [
        ":enriched_instance_identifier",
        ":find_service_handler",
        ":handle_type",
        ":i_service_discovery",
        ":instance_identifier",
        "//platform/aas/lib/result",
    ],
)

cc_library(
    name = "service_discovery_client_mock",
    testonly = True,
    srcs = ["service_discovery_client_mock.cpp"],
    hdrs = ["service_discovery_client_mock.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com/impl:__subpackages__"],
    deps = [
        ":i_service_discovery_client",
        "//third_party/googletest",
    ],
)

cc_library(
    name = "binding_type",
    srcs = ["binding_type.cpp"],
    hdrs = ["binding_type.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com/impl:__subpackages__"],
)

cc_library(
    name = "runtime_interfaces",
    srcs = [
        "i_runtime.cpp",
        "i_runtime_binding.cpp",
    ],
    hdrs = [
        "i_runtime.h",
        "i_runtime_binding.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com/impl:__subpackages__"],
    deps = [
        ":binding_type",
        ":i_service_discovery",
        ":i_service_discovery_client",
        ":instance_identifier",
        ":instance_specifier",
        "//platform/aas/mw/com/impl/tracing:i_tracing_runtime",
        "//platform/aas/mw/com/impl/tracing:i_tracing_runtime_binding",
        "//platform/aas/mw/com/impl/tracing/configuration:i_tracing_filter_config",
    ],
)

cc_library(
    name = "runtime_mock",
    hdrs = ["runtime_mock.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com:__subpackages__"],
    deps = [
        ":runtime_interfaces",
        "//third_party/googletest",
    ],
)

cc_library(
    name = "event_receive_handler",
    srcs = ["event_receive_handler.cpp"],
    hdrs = ["event_receive_handler.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com:__subpackages__"],
    deps = ["@amp"],
)

cc_library(
    name = "i_service_discovery",
    srcs = ["i_service_discovery.cpp"],
    hdrs = ["i_service_discovery.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com:__subpackages__"],
    deps = [
        ":enriched_instance_identifier",
        ":find_service_handle",
        ":find_service_handler",
        ":handle_type",
        ":instance_identifier",
        ":instance_specifier",
        "//platform/aas/lib/result",
    ],
)

cc_library(
    name = "service_discovery",
    srcs = ["service_discovery.cpp"],
    hdrs = ["service_discovery.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com:__subpackages__"],
    deps = [
        ":enriched_instance_identifier",
        ":find_service_handle",
        ":find_service_handler",
        ":handle_type",
        ":i_service_discovery",
        ":instance_identifier",
        ":instance_specifier",
        ":runtime_interfaces",
        "//platform/aas/lib/result",
    ],
)

cc_library(
    name = "binding_event_receive_handler",
    srcs = ["binding_event_receive_handler.cpp"],
    hdrs = ["binding_event_receive_handler.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com:__subpackages__"],
    deps = ["//platform/aas/language/safecpp/scoped_function:move_only_scoped_function"],
)

cc_library(
    name = "service_discovery_mock",
    testonly = True,
    srcs = ["service_discovery_mock.cpp"],
    hdrs = ["service_discovery_mock.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com:__subpackages__"],
    deps = [
        ":find_service_handle",
        ":find_service_handler",
        ":handle_type",
        ":i_service_discovery",
        ":instance_identifier",
        ":instance_specifier",
        "//platform/aas/lib/result",
        "//third_party/googletest",
    ],
)

cc_library(
    name = "runtime",
    srcs = ["runtime.cpp"],
    hdrs = ["runtime.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com:__subpackages__"],
    deps = [
        ":instance_identifier",
        ":instance_specifier",
        ":runtime_interfaces",
        ":service_discovery",
        "//platform/aas/lib/concurrency:long_running_threads_container",
        "//platform/aas/lib/memory/shared:types",
        "//platform/aas/mw/com/impl:event_receive_handler",
        "//platform/aas/mw/com/impl/configuration",
        "//platform/aas/mw/com/impl/plumbing:runtime",
        "//platform/aas/mw/com/impl/tracing:tracing_runtime",
        "//platform/aas/mw/com/impl/tracing/configuration:tracing_filter_config",
        "//platform/aas/mw/com/impl/tracing/configuration:tracing_filter_config_parser",
        "//platform/aas/mw/log",
        "@amp",
    ],
)

cc_library(
    name = "sample_reference_tracker",
    srcs = ["sample_reference_tracker.cpp"],
    hdrs = ["sample_reference_tracker.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com/impl:__subpackages__"],
    deps = ["@amp"],
)

cc_library(
    name = "service_element_map",
    srcs = ["service_element_map.cpp"],
    hdrs = ["service_element_map.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com/impl:__subpackages__"],
    deps = ["@amp"],
)

cc_library(
    name = "flag_owner",
    srcs = ["flag_owner.cpp"],
    hdrs = ["flag_owner.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com/impl:__subpackages__"],
    deps = [],
)

genrule(
    name = "config",
    testonly = True,
    srcs = ["//platform/aas/mw/com/impl/configuration:ara_com_config.json"],
    outs = ["etc/mw_com_config.json"],
    cmd = """
        mkdir -p $(@D)/;
        cp $(location //platform/aas/mw/com/impl/configuration:ara_com_config.json) $(OUTS)
        """,
    output_to_bindir = True,
)

[
    cc_gtest_unit_test(
        name = "unit_test_runtime_double_config_{}".format(runtime_test_idx),
        srcs = ["runtime_single_exec_test_{}.cpp".format(runtime_test_idx)],
        data = [
            ":config",
            "//platform/aas/mw/com/impl/configuration:ara_com_config.json",
            "//platform/aas/mw/com/impl/configuration:ara_com_config_other.json",
        ],
        deps = [
            ":impl",
            "//platform/aas/mw/log",
        ],
    )
    for runtime_test_idx in [
        3,
        4,
    ]
]

[
    cc_gtest_unit_test(
        name = "unit_test_runtime_single_config_{}".format(runtime_test_idx),
        srcs = ["runtime_single_exec_test_{}.cpp".format(runtime_test_idx)],
        data = [
            ":config",
            "//platform/aas/mw/com/impl/configuration:ara_com_config.json",
        ],
        deps = [
            ":impl",
            "//platform/aas/mw/log",
        ],
    )
    for runtime_test_idx in [
        1,
        2,
        5,
        6,
        7,
        8,
        9,
        10,
    ]
]

cc_test(
    name = "unit_test",
    # TODO: Investigate why this unit tests takes more than 1 minute under memcheck ()
    timeout = "moderate",
    srcs = [
        "com_error_test.cpp",
        "enriched_instance_identifier_test.cpp",
        "find_service_handle_test.cpp",
        "generic_proxy_event_test.cpp",
        "generic_proxy_test.cpp",
        "handle_type_test.cpp",
        "instance_identifier_test.cpp",
        "instance_specifier_test.cpp",
        "proxy_base_test.cpp",
        "proxy_binding_test.cpp",
        "proxy_event_base_test.cpp",
        "proxy_event_binding_base_test.cpp",
        "proxy_event_test.cpp",
        "proxy_field_test.cpp",
        "runtime_test.cpp",
        "sample_reference_tracker_test.cpp",
        "service_discovery_test.cpp",
        "service_element_map_test.cpp",
        "skeleton_base_test.cpp",
        "skeleton_binding_test.cpp",
        "skeleton_event_base_test.cpp",
        "skeleton_event_binding_test.cpp",
        "skeleton_event_test.cpp",
        "skeleton_field_base_test.cpp",
        "skeleton_field_test.cpp",
        "traits_test.cpp",
    ],
    features = COMPILER_WARNING_FEATURES + [
        "aborts_upon_exception",
    ],
    tags = ["unit"],
    deps = [
        ":enriched_instance_identifier",
        ":impl",
        ":runtime_mock",
        ":service_discovery_client_mock",
        ":service_discovery_mock",
        ":service_element_map",
        "//platform/aas/analysis/tracing/library/generic_trace_api/mocks:trace_library_mock",
        "//platform/aas/language/safecpp/coverage_termination_handler",
        "//platform/aas/lib/os/mocklib:mman_mock",
        "//platform/aas/lib/os/mocklib:stat_mock",
        "//platform/aas/mw/com:types",
        "//platform/aas/mw/com/impl/bindings/lola:runtime_mock",
        "//platform/aas/mw/com/impl/bindings/lola/test_doubles",
        "//platform/aas/mw/com/impl/bindings/mock_binding",
        "//platform/aas/mw/com/impl/configuration/test:configuration_test_resources",
        "//platform/aas/mw/com/impl/plumbing:proxy_binding_factory_mock",
        "//platform/aas/mw/com/impl/plumbing:proxy_event_binding_factory_mock",
        "//platform/aas/mw/com/impl/plumbing:proxy_field_binding_factory_mock",
        "//platform/aas/mw/com/impl/plumbing:skeleton_binding_factory_mock",
        "//platform/aas/mw/com/impl/plumbing:skeleton_event_binding_factory_mock",
        "//platform/aas/mw/com/impl/plumbing:skeleton_field_binding_factory_mock",
        "//platform/aas/mw/com/impl/test:binding_factory_resources",
        "//platform/aas/mw/com/impl/test:proxy_resources",
        "//platform/aas/mw/log",
        "//third_party/googletest:main",
        "@amp",
    ],
)

cc_unit_test_suites_for_host_and_qnx(
    name = "unit_test_suite",
    cc_unit_tests = [
        ":unit_test",
        ":unit_test_runtime_single_config_1",
        ":unit_test_runtime_single_config_2",
        ":unit_test_runtime_double_config_3",
        ":unit_test_runtime_double_config_4",
        # TODO: Tests 5-9 are disabled as they're failing on QNX. Should be investigated and reenabled in: 
        # ":unit_test_runtime_single_config_5",
        # ":unit_test_runtime_single_config_6",
        # ":unit_test_runtime_single_config_7",
        # ":unit_test_runtime_single_config_8",
        # ":unit_test_runtime_single_config_9",
        ":unit_test_runtime_single_config_10",
    ],
    test_suites_from_sub_packages = [
        "//platform/aas/mw/com/impl/bindings/lola:unit_test_suite",
        "//platform/aas/mw/com/impl/configuration:unit_test_suite",
        "//platform/aas/mw/com/impl/plumbing:unit_test_suite",
        "//platform/aas/mw/com/impl/tracing:unit_test_suite",
    ],
    visibility = ["//platform/aas/mw/com:__pkg__"],
)
