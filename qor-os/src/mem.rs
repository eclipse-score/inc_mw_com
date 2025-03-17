// Copyright (c) 2025 Qorix GmbH
//
// This program and the accompanying materials are made available under the
// terms of the Apache License, Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0.
//
// SPDX-License-Identifier: Apache-2.0
use super::os_errors;
use super::{Error, Result};

use std::alloc::{GlobalAlloc, Layout, System};
use std::fmt::Display;

/// General pointer type
pub type Ptr = *mut u8;

pub trait OsAllocator {
    /// Allocate memory
    fn allocate(&mut self, size: usize, align: usize) -> Result<Ptr>;

    /// Deallocate memory
    ///
    /// # Safety
    ///
    /// A valid pointer, size and alignment must be provided.
    unsafe fn deallocate(&mut self, ptr: Ptr, size: usize, align: usize) -> Result<()>;
}

#[derive(Debug)]
pub enum MemAction {
    Alloc(usize, usize, Ptr),
    Dealloc(usize, usize, Ptr),
}

impl Display for MemAction {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match *self {
            MemAction::Alloc(size, align, ptr) => {
                write!(f, "alloc: 0x{size:016x} %0x{align:016x} -> 0x{ptr:#?}",)
            }
            MemAction::Dealloc(size, align, ptr) => {
                write!(f, "dealloc: 0x{ptr:#?} 0x{size:016x} %0x{align:016x}",)
            }
        }
    }
}

pub struct ProcessMem {
    history: Vec<MemAction>,
}

impl ProcessMem {
    pub const fn new() -> Self {
        Self {
            history: Vec::new(),
        }
    }

    #[allow(dead_code)]
    fn dump(&self) {
        println!();
        for item in &self.history {
            println!("{item}")
        }
    }
}

impl Default for ProcessMem {
    fn default() -> Self {
        Self::new()
    }
}

impl OsAllocator for ProcessMem {
    fn allocate(&mut self, size: usize, align: usize) -> Result<Ptr> {
        let ptr = unsafe { System.alloc(Layout::from_size_align_unchecked(size, align)) };
        self.history.push(MemAction::Alloc(size, align, ptr));

        if ptr.is_null() {
            return Err(Error::const_new(
                os_errors::ALLOCATION_FAILURE,
                "OS memory allocation fault",
            ));
        }

        Ok(ptr)
    }

    unsafe fn deallocate(&mut self, ptr: Ptr, size: usize, align: usize) -> Result<()> {
        assert!(self.history.iter().any(|elem| match elem {
            MemAction::Alloc(_, _, p) => ptr == *p,
            _ => false,
        }));
        self.history.push(MemAction::Dealloc(size, align, ptr));

        if !ptr.is_null() {
            unsafe {
                System.dealloc(ptr, Layout::from_size_align_unchecked(size, align));
            }
        }

        Ok(())
    }
}
