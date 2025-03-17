// Copyright (c) 2025 Qorix GmbH
//
// This program and the accompanying materials are made available under the
// terms of the Apache License, Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0.
//
// SPDX-License-Identifier: Apache-2.0

use qor_core::core_errors::OS;
use qor_core::ErrorCode;

#[allow(dead_code)]
const MEM: ErrorCode = OS + 0x1000;
#[allow(dead_code)]
const THREAD: ErrorCode = OS + 0x2000;
#[allow(dead_code)]
const FILESYSTEM: ErrorCode = OS + 0x3000;

pub const ALLOCATION_FAILURE: ErrorCode = MEM + 1;
