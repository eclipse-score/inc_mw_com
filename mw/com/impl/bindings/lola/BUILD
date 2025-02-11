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
load("//platform/aas/bazel/generators:unit_tests.bzl", "cc_unit_test_suites_for_host_and_qnx")
load("//platform/aas/mw:common_features.bzl", "COMPILER_WARNING_FEATURES")

cc_library(
    name = "lola",
    srcs = [
    ],
    hdrs = [
    ],
    features = COMPILER_WARNING_FEATURES,
    visibility = [
        "//platform/aas/mw/com/impl/bindings/lola/test:__pkg__",
        "//platform/aas/mw/com/impl/plumbing:__pkg__",
    ],
    deps = [
        ":event",
        ":proxy",
        ":runtime",
        ":skeleton",
        "//platform/aas/lib/concurrency",
        "//platform/aas/mw/com/impl/configuration",
        "//platform/aas/mw/com/message_passing",
        "//platform/aas/mw/log",
        "@amp",
    ],
)

cc_library(
    name = "path_builder",
    srcs = ["path_builder.cpp"],
    hdrs = ["path_builder.h"],
    features = COMPILER_WARNING_FEATURES,
    deps = [
        ":i_shm_path_builder",
        "//platform/aas/mw/com/impl/configuration",
    ],
)

cc_library(
    name = "shm_path_builder",
    srcs = ["shm_path_builder.cpp"],
    hdrs = ["shm_path_builder.h"],
    features = COMPILER_WARNING_FEATURES,
    implementation_deps = [
        ":path_builder",
    ],
    visibility = ["//platform/aas/mw/com/impl/plumbing:__pkg__"],
    deps = [
        ":i_shm_path_builder",
        "//platform/aas/mw/com/impl/configuration",
    ],
)

cc_library(
    name = "partial_restart_path_builder",
    srcs = ["partial_restart_path_builder.cpp"],
    hdrs = ["partial_restart_path_builder.h"],
    features = COMPILER_WARNING_FEATURES,
    implementation_deps = [
        ":path_builder",
    ],
    visibility = ["//platform/aas/mw/com/impl/plumbing:__pkg__"],
    deps = [
        ":i_partial_restart_path_builder",
        ":path_builder",
        "//platform/aas/mw/com/impl/configuration",
    ],
)

cc_library(
    name = "i_runtime",
    srcs = ["i_runtime.cpp"],
    hdrs = ["i_runtime.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com/impl/plumbing:__pkg__"],
    deps = [
        ":rollback_data",
        "//platform/aas/mw/com/impl:runtime_interfaces",
        "//platform/aas/mw/com/impl/bindings/lola/messaging",
        "//platform/aas/mw/com/impl/configuration",
    ],
)

cc_library(
    name = "i_shm_path_builder",
    srcs = ["i_shm_path_builder.cpp"],
    hdrs = ["i_shm_path_builder.h"],
    features = COMPILER_WARNING_FEATURES,
    deps = [
        "//platform/aas/mw/com/impl/configuration",
        "@amp",
    ],
)

cc_library(
    name = "i_partial_restart_path_builder",
    srcs = ["i_partial_restart_path_builder.cpp"],
    hdrs = ["i_partial_restart_path_builder.h"],
    features = COMPILER_WARNING_FEATURES,
    deps = [
        "//platform/aas/mw/com/impl/configuration",
        "@amp",
    ],
)

cc_library(
    name = "service_discovery_client",
    srcs = ["service_discovery_client.cpp"],
    hdrs = ["service_discovery_client.h"],
    features = COMPILER_WARNING_FEATURES,
    deps = [
        "//platform/aas/lib/concurrency:executor",
        "//platform/aas/lib/filesystem",
        "//platform/aas/lib/os/utils/inotify:inotify_instance",
        "//platform/aas/lib/os/utils/inotify:inotify_instance_impl",
        "//platform/aas/mw/com/impl:error",
        "//platform/aas/mw/com/impl:i_service_discovery_client",
        "//platform/aas/mw/com/impl/bindings/lola/service_discovery:flag_file",
        "//platform/aas/mw/com/impl/bindings/lola/service_discovery:flag_file_crawler",
        "//platform/aas/mw/com/impl/bindings/lola/service_discovery:known_instances_container",
        "//platform/aas/mw/com/impl/bindings/lola/service_discovery:lola_service_instance_identifier",
        "//platform/aas/mw/com/impl/bindings/lola/service_discovery:quality_aware_container",
        "@amp",
    ],
)

cc_library(
    name = "runtime",
    srcs = ["runtime.cpp"],
    hdrs = ["runtime.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com/impl/plumbing:__pkg__"],
    deps = [
        ":i_runtime",
        ":service_discovery_client",
        "//platform/aas/lib/concurrency:executor",
        "//platform/aas/lib/concurrency:long_running_threads_container",
        "//platform/aas/lib/memory/shared",
        "//platform/aas/mw/com/impl/bindings/lola/messaging",
        "//platform/aas/mw/com/impl/bindings/lola/tracing:tracing_runtime",
        "//platform/aas/mw/log",
        "@amp",
    ],
)

cc_library(
    name = "rollback_data",
    srcs = ["rollback_data.cpp"],
    hdrs = ["rollback_data.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com/impl:__subpackages__"],
)

cc_library(
    name = "runtime_mock",
    hdrs = ["runtime_mock.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com/impl:__subpackages__"],
    deps = [
        ":i_runtime",
        "//third_party/googletest",
    ],
)

cc_library(
    name = "shm_path_builder_mock",
    srcs = ["shm_path_builder_mock.cpp"],
    hdrs = ["shm_path_builder_mock.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com/impl:__subpackages__"],
    deps = [
        ":i_shm_path_builder",
        "//third_party/googletest",
    ],
)

cc_library(
    name = "partial_restart_path_builder_mock",
    srcs = ["partial_restart_path_builder_mock.cpp"],
    hdrs = ["partial_restart_path_builder_mock.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com/impl:__subpackages__"],
    deps = [
        ":i_partial_restart_path_builder",
        "//third_party/googletest",
    ],
)

cc_library(
    name = "event_meta_info",
    srcs = ["event_meta_info.cpp"],
    hdrs = ["event_meta_info.h"],
    features = COMPILER_WARNING_FEATURES,
    deps = [
        ":data_type_meta_info",
        "//platform/aas/lib/memory/shared",
    ],
)

cc_library(
    name = "data_type_meta_info",
    srcs = ["data_type_meta_info.cpp"],
    hdrs = ["data_type_meta_info.h"],
    features = COMPILER_WARNING_FEATURES,
)

cc_library(
    name = "shared_data_structures",
    srcs = [
        "service_data_control.cpp",
        "service_data_storage.cpp",
    ],
    hdrs = [
        "service_data_control.h",
        "service_data_storage.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    visibility = [
        "//platform/aas/mw/com/impl/bindings/lola/test_doubles:__pkg__",
    ],
    deps = [
        ":element_fq_id",
        ":event",
        ":event_meta_info",
        ":uid_pid_mapping",
        "//platform/aas/lib/memory/shared",
    ],
)

cc_library(
    name = "skeleton",
    srcs = [
        "skeleton.cpp",
        "skeleton_event.cpp",
        "skeleton_event_properties.cpp",
    ],
    hdrs = [
        "skeleton.h",
        "skeleton_event.h",
        "skeleton_event_properties.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    visibility = [
        "//platform/aas/mw/com/impl/bindings/lola/test:__pkg__",
        "//platform/aas/mw/com/impl/bindings/lola/test_doubles:__pkg__",
    ],
    deps = [
        ":event",
        ":i_partial_restart_path_builder",
        ":i_shm_path_builder",
        ":partial_restart_path_builder",
        ":shared_data_structures",
        ":shm_path_builder",
        "//platform/aas/lib/filesystem",
        "//platform/aas/lib/memory/shared",
        "//platform/aas/lib/memory/shared:lock_file",
        "//platform/aas/lib/memory/shared:new_delete_delegate_resource",
        "//platform/aas/lib/memory/shared/flock:exclusive_flock_mutex",
        "//platform/aas/lib/memory/shared/flock:flock_mutex_and_lock",
        "//platform/aas/mw/com/impl:instance_identifier",
        "//platform/aas/mw/com/impl:runtime",
        "//platform/aas/mw/com/impl:skeleton_binding",
        "//platform/aas/mw/com/impl:skeleton_event_binding",
        "//platform/aas/mw/com/impl/configuration",
        "//platform/aas/mw/com/impl/plumbing:sample_allocatee_ptr",
        "//platform/aas/mw/com/impl/tracing:skeleton_event_tracing",
    ],
)

cc_library(
    name = "proxy",
    srcs = [
        "generic_proxy_event.cpp",
        "proxy.cpp",
        "proxy_event.cpp",
        "proxy_event_common.cpp",
        "slot_collector.cpp",
        "subscription_helpers.cpp",
        "subscription_not_subscribed_states.cpp",
        "subscription_state_base.cpp",
        "subscription_state_machine.cpp",
        "subscription_state_machine_states.cpp",
        "subscription_subscribed_states.cpp",
        "subscription_subscription_pending_states.cpp",
    ],
    hdrs = [
        "generic_proxy_event.h",
        "proxy.h",
        "proxy_event.h",
        "proxy_event_common.h",
        "slot_collector.h",
        "subscription_helpers.h",
        "subscription_not_subscribed_states.h",
        "subscription_state_base.h",
        "subscription_state_machine.h",
        "subscription_state_machine_states.h",
        "subscription_subscribed_states.h",
        "subscription_subscription_pending_states.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com/impl/bindings/lola/test:__pkg__"],
    deps = [
        ":event",
        ":event_control",
        ":event_subscription_control",
        ":shared_data_structures",
        ":shm_path_builder",
        ":transaction_log_id",
        ":transaction_log_registration_guard",
        ":transaction_log_rollback_executor",
        "//platform/aas/lib/filesystem",
        "//platform/aas/lib/memory/shared",
        "//platform/aas/lib/memory/shared:lock_file",
        "//platform/aas/lib/memory/shared/flock:flock_mutex_and_lock",
        "//platform/aas/lib/memory/shared/flock:shared_flock_mutex",
        "//platform/aas/lib/os:errno_logging",
        "//platform/aas/lib/os:fcntl",
        "//platform/aas/lib/os:glob",
        "//platform/aas/lib/os:unistd",
        "//platform/aas/lib/result",
        "//platform/aas/mw/com/impl:binding_event_receive_handler",
        "//platform/aas/mw/com/impl:generic_proxy_event_binding",
        "//platform/aas/mw/com/impl:instance_identifier",
        "//platform/aas/mw/com/impl:instance_specifier",
        "//platform/aas/mw/com/impl:proxy_binding",
        "//platform/aas/mw/com/impl:proxy_event_binding",
        "//platform/aas/mw/com/impl:runtime",
        "//platform/aas/mw/com/impl:runtime_interfaces",
        "//platform/aas/mw/com/impl/bindings/lola:partial_restart_path_builder",
        "//platform/aas/mw/com/impl/configuration",
        "//platform/aas/mw/com/impl/plumbing:sample_ptr",
        "//platform/aas/mw/com/impl/tracing:i_tracing_runtime",
        "//platform/aas/mw/log",
    ],
)

cc_library(
    name = "element_fq_id",
    srcs = ["element_fq_id.cpp"],
    hdrs = ["element_fq_id.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = [
        "//platform/aas/mw/com/impl/bindings/lola:__subpackages__",
        "//platform/aas/mw/com/impl/plumbing:__subpackages__",
    ],
    deps = [
        "//platform/aas/mw/log",
    ],
)

cc_library(
    name = "sample_ptr",
    srcs = ["sample_ptr.cpp"],
    hdrs = ["sample_ptr.h"],
    features = COMPILER_WARNING_FEATURES,
    deps = [
        ":event_data_control",
        ":slot_decrementer",
    ],
)

cc_library(
    name = "slot_decrementer",
    srcs = ["slot_decrementer.cpp"],
    hdrs = ["slot_decrementer.h"],
    features = COMPILER_WARNING_FEATURES,
    deps = [
        ":event_data_control",
        ":transaction_log_set",
    ],
)

cc_library(
    name = "uid_pid_mapping",
    srcs = ["uid_pid_mapping.cpp"],
    hdrs = ["uid_pid_mapping.h"],
    features = COMPILER_WARNING_FEATURES,
    implementation_deps = ["//platform/aas/mw/log"],
    visibility = ["//platform/aas/mw/com/impl/bindings/lola:__subpackages__"],
    deps = [
        "//platform/aas/lib/containers:dynamic_array",
        "//platform/aas/lib/memory/shared:atomic_indirector",
        "//platform/aas/lib/memory/shared:types",
        "@amp",
    ],
)

cc_library(
    name = "event_data_control",
    srcs = ["event_data_control.cpp"],
    hdrs = ["event_data_control.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com/impl/bindings/lola:__subpackages__"],
    deps = [
        ":event_slot_status",
        ":transaction_log",
        ":transaction_log_id",
        ":transaction_log_set",
        "//platform/aas/lib/containers:dynamic_array",
        "//platform/aas/lib/memory/shared:atomic_indirector",
        "//platform/aas/lib/memory/shared:types",
        "@amp",
    ],
)

cc_library(
    name = "event_control",
    srcs = ["event_control.cpp"],
    hdrs = ["event_control.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com/impl/bindings/lola:__subpackages__"],
    deps = [
        ":event_data_control",
        ":event_subscription_control",
    ],
)

cc_library(
    name = "event_subscription_control",
    srcs = ["event_subscription_control.cpp"],
    hdrs = ["event_subscription_control.h"],
    features = COMPILER_WARNING_FEATURES,
    implementation_deps = ["//platform/aas/mw/log"],
    visibility = ["//platform/aas/mw/com/impl/bindings/lola:__subpackages__"],
    deps = [
        "//platform/aas/lib/memory/shared:atomic_indirector",
        "//platform/aas/mw/com/impl/configuration",
    ],
)

cc_library(
    name = "event_slot_status",
    srcs = ["event_slot_status.cpp"],
    hdrs = ["event_slot_status.h"],
    features = COMPILER_WARNING_FEATURES,
)

cc_library(
    name = "event_data_control_composite",
    srcs = ["event_data_control_composite.cpp"],
    hdrs = ["event_data_control_composite.h"],
    features = COMPILER_WARNING_FEATURES,
    deps = [
        ":event_data_control",
        ":event_slot_status",
        "//platform/aas/lib/memory/shared:atomic_indirector",
        "@amp",
    ],
)

cc_library(
    name = "event_data_storage",
    srcs = ["event_data_storage.cpp"],
    hdrs = ["event_data_storage.h"],
    features = COMPILER_WARNING_FEATURES,
    deps = [
        "//platform/aas/lib/containers:dynamic_array",
        "//platform/aas/lib/memory/shared:types",
    ],
)

cc_library(
    name = "sample_allocatee_ptr",
    srcs = ["sample_allocatee_ptr.cpp"],
    hdrs = ["sample_allocatee_ptr.h"],
    features = COMPILER_WARNING_FEATURES,
    deps = [":event_data_control_composite"],
)

cc_library(
    name = "event",
    features = COMPILER_WARNING_FEATURES,
    visibility = [
        "//platform/aas/mw/com/impl/bindings/lola/messaging:__pkg__",
        "//platform/aas/mw/com/impl/bindings/lola/test:__pkg__",
        "//platform/aas/mw/com/impl/bindings/lola/test_doubles:__pkg__",
        "//platform/aas/mw/com/impl/plumbing:__pkg__",
    ],
    deps = [
        ":event_control",
        ":event_data_control",
        ":event_data_control_composite",
        ":event_data_storage",
        ":event_slot_status",
        ":event_subscription_control",
        ":sample_allocatee_ptr",
        ":sample_ptr",
    ],
)

cc_library(
    name = "transaction_log",
    srcs = ["transaction_log.cpp"],
    hdrs = ["transaction_log.h"],
    features = COMPILER_WARNING_FEATURES,
    implementation_deps = [
        "//platform/aas/mw/com/impl:error",
        "//platform/aas/mw/log",
    ],
    visibility = ["//platform/aas/mw/com/impl/bindings/lola:__subpackages__"],
    deps = [
        ":transaction_log_slot",
        "//platform/aas/lib/memory/shared",
        "@amp",
    ],
)

cc_library(
    name = "transaction_log_id",
    srcs = ["transaction_log_id.cpp"],
    hdrs = ["transaction_log_id.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com/impl/bindings/lola:__subpackages__"],
)

cc_library(
    name = "transaction_log_set",
    srcs = ["transaction_log_set.cpp"],
    hdrs = ["transaction_log_set.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com/impl/bindings/lola:__subpackages__"],
    deps = [
        ":transaction_log",
        ":transaction_log_id",
        "//platform/aas/lib/result",
        "//platform/aas/mw/com/impl/configuration",
        "//platform/aas/mw/com/impl/util:copyable_atomic",
        "@amp",
    ],
)

cc_library(
    name = "transaction_log_rollback_executor",
    srcs = ["transaction_log_rollback_executor.cpp"],
    hdrs = ["transaction_log_rollback_executor.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com/impl/bindings/lola:__subpackages__"],
    deps = [
        ":shared_data_structures",
        ":transaction_log_id",
        ":transaction_log_set",
        "//platform/aas/lib/result",
        "//platform/aas/mw/com/impl:runtime",
        "//platform/aas/mw/com/impl/bindings/lola:runtime",
        "@amp",
    ],
)

cc_library(
    name = "transaction_log_registration_guard",
    srcs = ["transaction_log_registration_guard.cpp"],
    hdrs = ["transaction_log_registration_guard.h"],
    features = COMPILER_WARNING_FEATURES,
    implementation_deps = [
        "//platform/aas/mw/com/impl:error",
    ],
    visibility = ["//platform/aas/mw/com/impl/bindings/lola:__subpackages__"],
    deps = [
        ":event_data_control",
        ":transaction_log_id",
        "//platform/aas/lib/result",
    ],
)

cc_library(
    name = "transaction_log_slot",
    srcs = ["transaction_log_slot.cpp"],
    hdrs = ["transaction_log_slot.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com/impl/bindings/lola:__subpackages__"],
)

cc_library(
    name = "event_data_control_test_resources",
    testonly = True,
    srcs = ["event_data_control_test_resources.cpp"],
    hdrs = ["event_data_control_test_resources.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//platform/aas/mw/com/impl:__subpackages__"],
    deps = [
        "//platform/aas/mw/com/impl/bindings/lola:event",
        "@amp",
    ],
)

cc_test(
    name = "unit_test",
    size = "small",
    timeout = "long",
    srcs = [
        "element_fq_id_test.cpp",
        "event_data_control_composite_test.cpp",
        "event_data_control_test.cpp",
        "event_slot_status_test.cpp",
        "event_subscription_control_test.cpp",
        "partial_restart_path_builder_test.cpp",
        "proxy_event_common_test.cpp",
        "proxy_event_test.cpp",
        "proxy_test.cpp",
        "runtime_test.cpp",
        "sample_allocatee_ptr_test.cpp",
        "sample_ptr_test.cpp",
        "service_data_storage_test.cpp",
        "service_discovery_client_test.cpp",
        "shm_path_builder_test.cpp",
        "skeleton_event_test.cpp",
        "skeleton_event_tracing_test.cpp",
        "skeleton_test.cpp",
        "slot_collector_test.cpp",
        "subscription_state_machine_events_test.cpp",
        "subscription_state_machine_methods_test.cpp",
        "subscription_state_machine_states_test.cpp",
        "transaction_log_registration_guard_test.cpp",
        "transaction_log_rollback_executor_test.cpp",
        "transaction_log_set_test.cpp",
        "transaction_log_slot_test.cpp",
        "transaction_log_test.cpp",
        "uid_pid_mapping_test.cpp",
    ],
    features = COMPILER_WARNING_FEATURES + [
        "aborts_upon_exception",
    ],
    tags = ["unit"],
    deps = [
        ":event_data_control_test_resources",
        ":lola",
        ":shm_path_builder_mock",
        ":transaction_log",
        ":transaction_log_id",
        ":transaction_log_rollback_executor",
        ":transaction_log_set",
        "//platform/aas/language/safecpp/coverage_termination_handler",
        "//platform/aas/lib/concurrency:executor_mock",
        "//platform/aas/lib/concurrency:long_running_threads_container",
        "//platform/aas/lib/filesystem",
        "//platform/aas/lib/filesystem:mock",
        "//platform/aas/lib/memory/shared",
        "//platform/aas/lib/memory/shared:atomic_indirector_mock_binding",
        "//platform/aas/lib/memory/shared:shared_memory_factory_mock",
        "//platform/aas/lib/memory/shared:shared_memory_resource_heap_allocator_mock",
        "//platform/aas/lib/memory/shared:shared_memory_resource_mock",
        "//platform/aas/lib/os/mocklib:fcntl_mock",
        "//platform/aas/lib/os/mocklib:glob_mock",
        "//platform/aas/lib/os/mocklib:grp_mock",
        "//platform/aas/lib/os/mocklib:mqueue_mock",
        "//platform/aas/lib/os/mocklib:stat_mock",
        "//platform/aas/lib/os/mocklib:stdlib_mock",
        "//platform/aas/lib/os/mocklib:unistd_mock",
        "//platform/aas/lib/os/utils/inotify:inotify_instance_facade",
        "//platform/aas/lib/os/utils/inotify:inotify_instance_mock",
        "//platform/aas/lib/utils:coverage_termination_handler",
        "//platform/aas/mw/com/impl:instance_specifier",
        "//platform/aas/mw/com/impl:runtime",
        "//platform/aas/mw/com/impl:runtime_mock",
        "//platform/aas/mw/com/impl/bindings/lola:runtime_mock",
        "//platform/aas/mw/com/impl/bindings/lola/messaging:mock",
        "//platform/aas/mw/com/impl/bindings/lola/test:proxy_event_test_resources",
        "//platform/aas/mw/com/impl/bindings/lola/test:skeleton_event_test_resources",
        "//platform/aas/mw/com/impl/bindings/lola/test:skeleton_test_resources",
        "//platform/aas/mw/com/impl/bindings/lola/test:transaction_log_test_resources",
        "//platform/aas/mw/com/impl/bindings/lola/test_doubles",
        "//platform/aas/mw/com/impl/bindings/mock_binding",
        "//platform/aas/mw/com/impl/tracing:skeleton_event_tracing",
        "//platform/aas/mw/com/impl/tracing:skeleton_event_tracing_data",
        "//platform/aas/mw/com/impl/tracing:tracing_runtime_mock",
        "//platform/aas/mw/com/impl/tracing/configuration:tracing_filter_config_mock",
        "//platform/aas/mw/com/message_passing:mock",
        "//platform/aas/mw/log",
        "//third_party/googletest:main",
    ],
)

cc_unit_test_suites_for_host_and_qnx(
    name = "unit_test_suite",
    cc_unit_tests = [
        ":unit_test",
        "//platform/aas/mw/com/impl/bindings/lola/messaging:message_passing_unit_test",
        "//platform/aas/mw/com/impl/bindings/lola/messaging:other_unit_test",
        "//platform/aas/mw/com/impl/bindings/lola/service_discovery:unit_test",
        "//platform/aas/mw/com/impl/bindings/lola/test:skeleton_component_test",
        "//platform/aas/mw/com/impl/bindings/lola/test:skeleton_event_component_test",
        "//platform/aas/mw/com/impl/bindings/lola/test:proxy_component_test",
    ],
    test_suites_from_sub_packages = [
        "//platform/aas/mw/com/impl/bindings/lola/tracing:unit_test_suite",
    ],
    visibility = ["//platform/aas/mw/com/impl:__pkg__"],
)
