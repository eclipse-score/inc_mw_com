// Copyright (c) 2025 Qorix GmbH
//
// This program and the accompanying materials are made available under the
// terms of the Apache License, Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0.
//
// SPDX-License-Identifier: Apache-2.0
use qor_core::Error;

use std::sync::OnceLock;

/// OS error codes
pub mod os_errors;

/// OS memory access functions
pub mod mem;

/// OS thread access functions
pub mod thread;

/// OS file system access functions
pub mod filesystem;

pub mod prelude {}

/// Operating system error class
type Result<T> = std::result::Result<T, Error>;

/// Operating System Abstraction functions
/// The OS cannot be instantiated and is only accesses through static methods
pub struct Os {}

static mut PROCESS_MEM: OnceLock<mem::ProcessMem> = OnceLock::new();

impl Os {
    pub fn name() -> String {
        "unknown".to_string()
    }

    pub fn memory() -> &'static mut mem::ProcessMem {
        // TODO: Make clippy accept references to static mut which is now denied by default. See
        // https://doc.rust-lang.org/nightly/edition-guide/rust-2024/static-mut-references.html#disallow-references-to-static-mut
        // for details.
        #[allow(static_mut_refs)]
        unsafe {
            PROCESS_MEM.get_or_init(mem::ProcessMem::new);
            PROCESS_MEM.get_mut().unwrap()
        }
    }
}
