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

load("//platform/aas/tools/sctf/bazel_gen:sctf_gen.bzl", "py_sctf_test")

py_sctf_test(
    name = "test_basic_message_passing",
    srcs = [
        "__init__.py",
        "test_basic_message_passing.py",
    ],
    data = ["//platform/aas/mw/com/message_passing/test:messaging_app_mqueue-pkg.tar"],
    main = "test_basic_message_passing.py",
    module_root = "platform/aas/mw/com/message_passing/test/sct",
)
