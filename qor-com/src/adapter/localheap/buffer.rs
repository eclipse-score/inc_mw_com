// Copyright (c) 2025 Qorix GmbH
//
// This program and the accompanying materials are made available under the
// terms of the Apache License, Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0.
//
// SPDX-License-Identifier: Apache-2.0
use super::HeapAdapter;

use crate::base::*;
use qor_core::prelude::*;

use std::{
    fmt::Debug,
    mem::MaybeUninit,
    ops::{Deref, DerefMut},
};

/// A single value assumed to reside on the heap
#[derive(Debug)]
#[repr(transparent)]
pub struct HeapBuffer<'s, 't, T>
where
    Self: 's,
    T: Debug + Send + TypeTag + Coherent + Reloc + 's,
    't: 's,
{
    _phantom_s: std::marker::PhantomData<&'s ()>,
    _phantom_t: std::marker::PhantomData<&'t ()>,
    value: Box<T>,
}

impl<'s, 't, T> Deref for HeapBuffer<'s, 't, T>
where
    Self: 's,
    T: Debug + Send + TypeTag + Coherent + Reloc + 's,
    't: 's,
{
    type Target = T;

    #[inline(always)]
    fn deref(&self) -> &Self::Target {
        &self.value
    }
}

impl<'s, 't, T> DerefMut for HeapBuffer<'s, 't, T>
where
    Self: 's,
    T: Debug + Send + TypeTag + Coherent + Reloc + 's,
    't: 's,
{
    #[inline(always)]
    fn deref_mut(&mut self) -> &mut Self::Target {
        &mut self.value
    }
}

impl<'s, 't, T> HeapBuffer<'s, 't, T>
where
    Self: 's,
    T: Debug + Send + TypeTag + Coherent + Reloc + 's,
    't: 's,
{
    #[inline(always)]
    pub(crate) fn new(value: T) -> Self {
        Self {
            _phantom_s: std::marker::PhantomData,
            _phantom_t: std::marker::PhantomData,
            value: Box::new(value),
        }
    }
}

impl<'s, 't, T> Buffer<'s, 't, HeapAdapter<'t>, T> for HeapBuffer<'s, 't, T>
where
    Self: 's,
    T: Debug + Send + TypeTag + Coherent + Reloc + 's,
    't: 's,
{
}

impl<'s, 't, T> BufferMut<'s, 't, HeapAdapter<'t>, T> for HeapBuffer<'s, 't, T>
where
    Self: 's,
    T: Debug + Send + TypeTag + Coherent + Reloc + 's,
    't: 's,
{
    fn into_buffer(self) -> impl Buffer<'s, 't, HeapAdapter<'t>, T> {
        self
    }
}

/// A maybe uninitialized value assumed to reside on the heap
#[derive(Debug)]
#[repr(transparent)]
pub struct HeapBufferMaybeUninit<'s, 't, T>
where
    Self: 's,
    T: Debug + Send + TypeTag + Coherent + Reloc + 's,
    't: 's,
{
    _phantom_s: std::marker::PhantomData<&'s ()>,
    _phantom_t: std::marker::PhantomData<&'t ()>,
    value: Box<MaybeUninit<T>>,
}

impl<'s, 't, T> Deref for HeapBufferMaybeUninit<'s, 't, T>
where
    Self: 's,
    T: Debug + Send + TypeTag + Coherent + Reloc + 's,
    't: 's,
{
    type Target = MaybeUninit<T>;

    fn deref(&self) -> &Self::Target {
        &self.value
    }
}

impl<'s, 't, T> DerefMut for HeapBufferMaybeUninit<'s, 't, T>
where
    Self: 's,
    T: Debug + Send + TypeTag + Coherent + Reloc + 's,
    't: 's,
{
    fn deref_mut(&mut self) -> &mut Self::Target {
        &mut self.value
    }
}

impl<'s, 't, T> HeapBufferMaybeUninit<'s, 't, T>
where
    Self: 's,
    T: Debug + Send + TypeTag + Coherent + Reloc + 's,
    't: 's,
{
    pub(crate) fn new() -> Self {
        Self {
            _phantom_s: std::marker::PhantomData,
            _phantom_t: std::marker::PhantomData,
            value: Box::new_uninit(),
        }
    }
}

impl<'s, 't, T> BufferMaybeUninit<'s, 't, HeapAdapter<'t>, T> for HeapBufferMaybeUninit<'s, 't, T>
where
    Self: 's,
    T: Debug + Send + TypeTag + Coherent + Reloc + 's,
    't: 's,
{
    /// The immutable data buffer is our heap buffer
    type Buffer = HeapBuffer<'s, 't, T>;

    /// The mutable data buffer is our heap buffer
    type BufferMut = HeapBuffer<'s, 't, T>;

    fn write(mut self, value: T) -> Self::BufferMut {
        *self.value = MaybeUninit::new(value);
        unsafe { self.assume_init() }
    }

    fn as_mut_ptr(&mut self) -> *mut T {
        self.value.as_mut_ptr() as *mut T
    }

    unsafe fn assume_init(self) -> Self::BufferMut {
        HeapBuffer {
            _phantom_s: std::marker::PhantomData,
            _phantom_t: std::marker::PhantomData,
            value: Box::from_raw(Box::into_raw(self.value.assume_init())),
        }
    }

    unsafe fn assume_init_read(self) -> Self::Buffer {
        HeapBuffer {
            _phantom_s: std::marker::PhantomData,
            _phantom_t: std::marker::PhantomData,
            value: Box::from_raw(Box::into_raw(self.value.assume_init())),
        }
    }
}
