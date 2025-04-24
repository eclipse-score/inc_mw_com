// Copyright (c) 2025 Qorix GmbH
//
// This program and the accompanying materials are made available under the
// terms of the Apache License, Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0.
//
// SPDX-License-Identifier: Apache-2.0
use super::Local;

use qor_core::prelude::*;

use crate::base::*;
use crate::concepts::*;

use std::{
    collections::VecDeque,
    fmt::Debug,
    mem::MaybeUninit,
    ops::{Deref, DerefMut},
    sync::{Arc, Condvar, Mutex},
    time::Duration,
};

//
// Topic
//

/// The queue of a event, storing the unprocessed values
///
type ValueQueue<T> = Arc<(Mutex<VecDeque<Box<T>>>, Condvar)>;

/// The publish action for a local event
#[derive(Debug)]
pub struct LocalSample<T>
where
    T: Debug + Send + TypeTag + Coherent + Reloc,
{
    /// The queue of the related event
    queue: ValueQueue<T>,

    /// The value of the current publish/subscribe action
    value: Box<T>,
}

impl<T> Deref for LocalSample<T>
where
    T: Debug + Send + TypeTag + Coherent + Reloc,
{
    type Target = T;

    #[inline(always)]
    fn deref(&self) -> &Self::Target {
        &self.value
    }
}

impl<T> DerefMut for LocalSample<T>
where
    T: Debug + Send + TypeTag + Coherent + Reloc,
{
    #[inline(always)]
    fn deref_mut(&mut self) -> &mut Self::Target {
        &mut self.value
    }
}

impl<T> LocalSample<T>
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

impl<T> SampleConcept<Local, T> for LocalSample<T> where T: Debug + Send + TypeTag + Coherent + Reloc {}

impl<T> SampleMutConcept<Local, T> for LocalSample<T>
where
    T: Debug + Send + TypeTag + Coherent + Reloc,
{
    type Sample = LocalSample<T>;

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
pub struct LocalSampleMaybeUninit<T>
where
    T: Debug + Send + TypeTag + Coherent + Reloc,
{
    /// The queue of the related event
    queue: ValueQueue<T>,

    /// The value of the current publish/subscribe action
    value: Box<MaybeUninit<T>>,
}

impl<T> LocalSampleMaybeUninit<T>
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

impl<T> SampleMaybeUninitConcept<Local, T> for LocalSampleMaybeUninit<T>
where
    T: Debug + Send + TypeTag + Coherent + Reloc,
{
    /// The mutable data buffer is our heap buffer
    type SampleMut = LocalSample<T>;

    fn write(mut self, value: T) -> Self::SampleMut {
        *self.value = MaybeUninit::new(value);
        unsafe { self.assume_init() }
    }

    fn as_mut_ptr(&mut self) -> *mut T {
        self.value.as_mut_ptr() as *mut T
    }

    unsafe fn assume_init(self) -> Self::SampleMut {
        LocalSample {
            queue: self.queue.clone(),
            value: Box::from_raw(Box::into_raw(self.value.assume_init())),
        }
    }
}

/// The local event
#[derive(Debug)]
pub struct LocalTopic<T>
where
    T: Debug + Send + TypeTag + Coherent + Reloc,
{
    queue: ValueQueue<T>,

    queue_policy: QueuePolicy,
    max_fan_in: usize,
    max_fan_out: usize,
}

impl<T> Clone for LocalTopic<T>
where
    T: Debug + Send + TypeTag + Coherent + Reloc,
{
    fn clone(&self) -> Self {
        Self {
            queue: self.queue.clone(),
            queue_policy: self.queue_policy,
            max_fan_in: self.max_fan_in,
            max_fan_out: self.max_fan_out,
        }
    }
}

impl<T> PublisherConcept<Local, T> for LocalTopic<T>
where
    T: Debug + Send + TypeTag + Coherent + Reloc,
{
    type SampleMaybeUninit = LocalSampleMaybeUninit<T>;

    fn loan_uninit(&self) -> ComResult<Self::SampleMaybeUninit> {
        Ok(LocalSampleMaybeUninit::new(self.queue.clone()))
    }

    fn loan(
        &self,
        value: T,
    ) -> ComResult<<Self::SampleMaybeUninit as SampleMaybeUninitConcept<Local, T>>::SampleMut> {
        Ok(LocalSample::new(self.queue.clone(), value))
    }
}

impl<T> SubscriberConcept<Local, T> for LocalTopic<T>
where
    T: Debug + Send + TypeTag + Coherent + Reloc,
{
    type Sample = LocalSample<T>;

    fn try_receive(&self) -> ComResult<Option<Self::Sample>> {
        match self.queue.0.lock().unwrap().pop_front() {
            Some(value) => Ok(Some(LocalSample::new_boxed(self.queue.clone(), value))),
            None => Ok(None),
        }
    }

    fn receive(&self) -> ComResult<Self::Sample> {
        let mut queue = self.queue.0.lock().unwrap();
        loop {
            if let Some(value) = queue.pop_front() {
                return Ok(LocalSample::new_boxed(self.queue.clone(), value));
            } else {
                queue = self.queue.1.wait(queue).unwrap();
            }
        }
    }

    fn receive_timeout(&self, timeout: Duration) -> ComResult<Self::Sample> {
        let mut queue = self.queue.0.lock().unwrap();

        loop {
            if let Some(value) = queue.pop_front() {
                return Ok(LocalSample::new_boxed(self.queue.clone(), value));
            } else {
                let (q, r) = self.queue.1.wait_timeout(queue, timeout).unwrap();
                queue = q;
                if r.timed_out() {
                    return Err(ComError::Timeout);
                }
            }
        }
    }

    async fn receive_async(&self) -> ComResult<Self::Sample> {
        unimplemented!()
    }

    async fn receive_timeout_async(&self, _timeout: Duration) -> ComResult<Self::Sample> {
        unimplemented!()
    }
}

impl<T> TopicConcept<Local, T> for LocalTopic<T>
where
    T: Debug + Send + TypeTag + Coherent + Reloc,
{
    type Publisher = Self;
    type Subscriber = Self;

    #[inline(always)]
    fn publisher(&self) -> ComResult<Self::Publisher> {
        Ok(self.clone())
    }

    #[inline(always)]
    fn subscriber(&self) -> ComResult<Self::Subscriber> {
        Ok(self.clone())
    }
}

impl<T> LocalTopic<T>
where
    T: Debug + Send + TypeTag + Coherent + Reloc,
{
    #[inline(always)]
    pub(crate) fn new(
        max_queue_depth: usize,
        queue_policy: QueuePolicy,
        max_fan_in: usize,
        max_fan_out: usize,
    ) -> Self {
        let mut queue = VecDeque::new();
        queue.reserve(max_queue_depth);

        Self {
            queue: Arc::new((Mutex::new(queue), Condvar::new())),
            queue_policy,
            max_fan_in,
            max_fan_out,
        }
    }
}

#[derive(Debug)]
pub struct LocalTopicBuilder<T>
where
    T: Debug + Send + TypeTag + Coherent + Reloc,
{
    _phantom: std::marker::PhantomData<T>,

    max_queue_depth: usize,
    queue_policy: QueuePolicy,
    max_fan_in: usize,
    max_fan_out: usize,
}

impl<T> TopicBuilderConcept<Local, T> for LocalTopicBuilder<T>
where
    T: Debug + Send + TypeTag + Coherent + Reloc,
{
    type Topic = LocalTopic<T>;

    #[inline(always)]
    fn with_queue_depth(mut self, queue_depth: usize) -> Self {
        self.max_queue_depth = queue_depth;
        self
    }

    #[inline(always)]
    fn with_queue_policy(mut self, queue_policy: QueuePolicy) -> Self {
        self.queue_policy = queue_policy;
        self
    }

    #[inline(always)]
    fn with_max_fan_in(mut self, fan_in: usize) -> Self {
        self.max_fan_in = fan_in;
        self
    }

    #[inline(always)]
    fn with_max_fan_out(mut self, fan_out: usize) -> Self {
        self.max_fan_out = fan_out;
        self
    }

    fn build(self) -> ComResult<Self::Topic> {
        if self.max_fan_in > 1 {
            Err(ComError::FanError)
        } else if self.max_fan_out < 1 {
            Err(ComError::FanError)
        } else {
            Ok(LocalTopic::new(
                self.max_queue_depth,
                self.queue_policy,
                self.max_fan_in,
                self.max_fan_out,
            ))
        }
    }
}

impl<T> LocalTopicBuilder<T>
where
    T: Debug + Send + TypeTag + Coherent + Reloc,
{
    #[inline(always)]
    pub(crate) fn new() -> Self {
        Self {
            _phantom: std::marker::PhantomData,

            max_queue_depth: 1,
            queue_policy: QueuePolicy::ErrorOnFull,
            max_fan_in: 1,
            max_fan_out: 1,
        }
    }
}
