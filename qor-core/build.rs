// Copyright (c) 2025 Qorix GmbH
//
// This program and the accompanying materials are made available under the
// terms of the Apache License, Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0.
//
// SPDX-License-Identifier: Apache-2.0

fn main() {
    // ok, we can build
    #[cfg(target_has_atomic = "64")]
    println!("cargo:rerun-if-changed=src/main.rs");
    // not yet supported
    #[cfg(not(target_has_atomic = "64"))]
    panic!("AtomicU64 operations are not lock-free on this platform which is required for current implementation.");
}
