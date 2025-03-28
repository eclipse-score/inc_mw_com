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
    collections::VecDeque,
    fmt::Debug,
    mem::MaybeUninit,
    ops::{Deref, DerefMut},
    sync::{Arc, Condvar, Mutex},
};

/// The queue of a event, storing the unprocessed values
///
pub type ValueQueue<T>
where
    T: Debug + Send + TypeTag + Coherent + Reloc,
= Arc<(Mutex<VecDeque<Box<T>>>, Condvar)>;

/// The publish action for a local event
#[derive(Debug)]
pub struct HeapSample<T>
where
    T: Debug + Send + TypeTag + Coherent + Reloc,
{
    /// The queue of the related event
    queue: ValueQueue<T>,

    /// The value of the current publish/subscribe action
    value: Box<T>,
}

impl<T> Deref for HeapSample<T>
where
    T: Debug + Send + TypeTag + Coherent + Reloc,
{
    type Target = T;

    #[inline(always)]
    fn deref(&self) -> &Self::Target {
        &self.value
    }
}

impl<T> DerefMut for HeapSample<T>
where
    T: Debug + Send + TypeTag + Coherent + Reloc,
{
    #[inline(always)]
    fn deref_mut(&mut self) -> &mut Self::Target {
        &mut self.value
    }
}

impl<T> HeapSample<T>
where
    T: Debug + Send + TypeTag + Coherent + Reloc,
{
    pub(crate) fn new(queue: ValueQueue<T>, value: T) -> Self {
        Self {
            queue,
            value: Box::new(value),
        }
    }

    pub(crate) fn new_boxed(queue: ValueQueue<T>, value: Box<T>) -> Self {
        Self { queue, value }
    }
}

impl<T> Sample<HeapAdapter, T> for HeapSample<T> where T: Debug + Send + TypeTag + Coherent + Reloc {}

impl<T> SampleMut<HeapAdapter, T> for HeapSample<T>
where
    T: Debug + Send + TypeTag + Coherent + Reloc,
{
    type Sample = HeapSample<T>;

    fn into_sample(self) -> Self::Sample {
        self
    }

    fn publish(self) -> ComResult<()> {
        let mut queue = self.queue.0.lock().unwrap();

        queue.push_back(self.value.into());
        self.queue.1.notify_all();
        Ok(())
    }
}

/// A maybe uninitialized value assumed to reside on the heap
#[derive(Debug)]
pub struct HeapSampleMaybeUninit<T>
where
    T: Debug + Send + TypeTag + Coherent + Reloc,
{
    /// The queue of the related event
    queue: ValueQueue<T>,
   
    /// The value of the current publish/subscribe action
    value: Box<MaybeUninit<T>>,
}

impl<T> HeapSampleMaybeUninit<T>
where
    T: Debug + Send + TypeTag + Coherent + Reloc,
{
    pub(crate) fn new(queue: ValueQueue<T>) -> Self {
        Self {
            queue,
            value: Box::new_uninit(),
        }
    }
}

impl<T> SampleMaybeUninit<HeapAdapter, T> for HeapSampleMaybeUninit<T>
where
    T: Debug + Send + TypeTag + Coherent + Reloc,
{
    /// The mutable data buffer is our heap buffer
    type SampleMut = HeapSample<T>;

    fn write(mut self, value: T) -> Self::SampleMut {
        *self.value = MaybeUninit::new(value);
        unsafe { self.assume_init() }
    }

    fn as_mut_ptr(&mut self) -> *mut T {
        self.value.as_mut_ptr() as *mut T
    }

    unsafe fn assume_init(self) -> Self::SampleMut {
        HeapSample {
            queue: self.queue.clone(),
            value: Box::from_raw(Box::into_raw(self.value.assume_init())),
        }
    }

    unsafe fn assume_init_read(
        self,
    ) -> <<Self as SampleMaybeUninit<HeapAdapter, T>>::SampleMut as SampleMut<HeapAdapter, T>>::Sample
    {
        HeapSample {
            queue: self.queue.clone(),
            value: Box::from_raw(Box::into_raw(self.value.assume_init())),
        }
    }
}
