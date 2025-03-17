// Copyright (c) 2025 Qorix GmbH
//
// This program and the accompanying materials are made available under the
// terms of the Apache License, Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0.
//
// SPDX-License-Identifier: Apache-2.0
use super::buffer::*;
use super::HeapAdapter;

use crate::base::*;
use qor_core::prelude::*;

use std::{
    collections::VecDeque,
    fmt::Debug,
    ops::{Deref, DerefMut},
    sync::{Arc, Condvar, Mutex},
    time::Duration,
};

//
// Topic
//

/// The queue of a topic, storing the unprocessed values
///
type TopicQueue<'s, 't, T>
where
    T: 's,
    't: 's,
= Arc<(Mutex<VecDeque<HeapBuffer<'s, 't, T>>>, Condvar)>;

/// The publish action for a local topic
#[derive(Debug)]
pub struct HeapPublishBuffer<'s, 't, T>
where
    Self: 's,
    T: Debug + Send + TypeTag + Coherent + Reloc + 's,
    't: 's,
{
    /// The queue of the related topic
    queue: TopicQueue<'s, 't, T>,

    /// The value of the current publish/subscribe action
    value: HeapBuffer<'s, 't, T>,
}

impl<'s, 't, T> Deref for HeapPublishBuffer<'s, 't, T>
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

impl<'s, 't, T> DerefMut for HeapPublishBuffer<'s, 't, T>
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

impl<'s, 't, T> PublishBuffer<'s, 't, HeapAdapter<'t>, T> for HeapPublishBuffer<'s, 't, T>
where
    Self: 's,
    T: Debug + Send + TypeTag + Coherent + Reloc + 's,
    't: 's,
{
    fn publish(self) -> ComResult<()> {
        let mut queue = self.queue.0.lock().unwrap();

        queue.push_back(self.value);
        self.queue.1.notify_all();
        Ok(())
    }
}

/// The publish action for a local topic with a maybe uninitialized value
#[derive(Debug)]
pub struct HeapPublishBufferMaybeUninit<'s, 't, T>
where
    Self: 's,
    T: Debug + Send + TypeTag + Coherent + Reloc + 's,
    't: 's,
{
    queue: TopicQueue<'s, 't, T>,
    value: HeapBufferMaybeUninit<'s, 't, T>,
}

impl<'s, 't, T> PublishBufferMaybeUninit<'s, 't, HeapAdapter<'t>, T>
    for HeapPublishBufferMaybeUninit<'s, 't, T>
where
    T: Debug + Send + TypeTag + Coherent + Reloc + 's,
    't: 's,
{
    fn write(self, value: T) -> impl PublishBuffer<'s, 't, HeapAdapter<'t>, T> {
        HeapPublishBuffer {
            queue: self.queue.clone(),
            value: HeapBuffer::new(value),
        }
    }

    fn as_mut_ptr(&mut self) -> *mut T {
        self.value.as_mut_ptr()
    }

    unsafe fn assume_init(self) -> impl PublishBuffer<'s, 't, HeapAdapter<'t>, T> {
        HeapPublishBuffer {
            queue: self.queue.clone(),
            value: self.value.assume_init(),
        }
    }
}

/// The local topic
#[derive(Debug)]
pub struct HeapTopic<'s, 't, T>
where
    Self: 's,
    T: Debug + Send + TypeTag + Coherent + Reloc + 's,
    't: 's,
{
    queue: TopicQueue<'s, 't, T>,

    queue_policy: QueuePolicy,
    max_fan_in: usize,
    max_fan_out: usize,
}

impl<'s, 't, T> Clone for HeapTopic<'s, 't, T>
where
    Self: 's,
    T: Debug + Send + TypeTag + Coherent + Reloc + 's,
    't: 's,
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

impl<'s, 't, T> Publisher<'s, 't, HeapAdapter<'t>, T> for HeapTopic<'s, 't, T>
where
    Self: 's,
    T: Debug + Send + TypeTag + Coherent + Reloc + 's,
    't: 's,
{
    type ReuseBuffer = HeapBuffer<'s, 't, T>;

    fn loan_with(&self, value: T) -> ComResult<impl PublishBuffer<'s, 't, HeapAdapter<'t>, T>> {
        Ok(HeapPublishBuffer {
            queue: self.queue.clone(),
            value: HeapBuffer::new(value),
        })
    }

    fn loan_uninit(&self) -> ComResult<impl PublishBufferMaybeUninit<'s, 't, HeapAdapter<'t>, T>> {
        Ok(HeapPublishBufferMaybeUninit {
            queue: self.queue.clone(),
            value: HeapBufferMaybeUninit::new(),
        })
    }

    fn reuse(
        &self,
        buffer: Self::ReuseBuffer,
    ) -> ComResult<impl PublishBuffer<'s, 't, HeapAdapter<'t>, T>> {
        Ok(HeapPublishBuffer {
            queue: self.queue.clone(),
            value: buffer,
        })
    }
}

impl<'s, 't, T> Subscriber<'s, 't, HeapAdapter<'t>, T> for HeapTopic<'s, 't, T>
where
    T: Debug + Send + TypeTag + Coherent + Reloc + 's,
    't: 's,
{
    fn try_receive(&self) -> ComResult<Option<impl BufferMut<'s, 't, HeapAdapter<'t>, T>>> {
        Ok(self.queue.0.lock().unwrap().pop_front())
    }

    fn receive(&self) -> ComResult<impl BufferMut<'s, 't, HeapAdapter<'t>, T>> {
        let mut queue = self.queue.0.lock().unwrap();
        loop {
            if let Some(value) = queue.pop_front() {
                return Ok(value);
            } else {
                queue = self.queue.1.wait(queue).unwrap();
            }
        }
    }

    fn receive_timeout(
        &self,
        timeout: Duration,
    ) -> ComResult<impl BufferMut<'s, 't, HeapAdapter<'t>, T>> {
        let mut queue = self.queue.0.lock().unwrap();

        loop {
            if let Some(value) = queue.pop_front() {
                return Ok(value);
            } else {
                let (q, r) = self.queue.1.wait_timeout(queue, timeout).unwrap();
                queue = q;
                if r.timed_out() {
                    return Err(Error::from_code(qor_core::core_errors::TIMEOUT));
                }
            }
        }
    }

    async fn receive_async(&self) -> ComResult<impl BufferMut<'s, 't, HeapAdapter<'t>, T>> {
        Err::<HeapBuffer<T>, Error>(Error::FEATURE_NOT_SUPPORTED)
    }
}

impl<'s, 't, T> Topic<'s, 't, HeapAdapter<'t>, T> for HeapTopic<'s, 't, T>
where
    T: Debug + Send + TypeTag + Coherent + Reloc + 's,
    't: 's,
{
    #[inline(always)]
    fn publisher(&self) -> ComResult<impl Publisher<'s, 't, HeapAdapter<'t>, T>> {
        Ok(self.clone())
    }

    #[inline(always)]
    fn subscriber(&self) -> ComResult<impl Subscriber<'s, 't, HeapAdapter<'t>, T>> {
        Ok(self.clone())
    }
}

impl<'s, 't, T> HeapTopic<'s, 't, T>
where
    T: Debug + Send + TypeTag + Coherent + Reloc + 's,
    't: 's,
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
pub struct HeapTopicBuilder<T>
where
    T: Debug + Send + TypeTag + Coherent + Reloc,
{
    _phantom_u: std::marker::PhantomData<T>,

    max_queue_depth: usize,
    queue_policy: QueuePolicy,
    max_fan_in: usize,
    max_fan_out: usize,
}

impl<'t, T> TopicBuilder<'t, HeapAdapter<'t>, T> for HeapTopicBuilder<T>
where
    T: Debug + Send + TypeTag + Coherent + Reloc,
{
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

    fn build<'a>(self) -> ComResult<impl Topic<'a, 't, HeapAdapter<'t>, T>>
    where
        T: 'a,
        't: 'a,
    {
        if self.max_fan_in > 1 {
            Err(Error::FEATURE_NOT_SUPPORTED)
        } else if self.max_fan_out > 1 {
            Err(Error::FEATURE_NOT_SUPPORTED)
        } else {
            Ok(HeapTopic::new(
                self.max_queue_depth,
                self.queue_policy,
                self.max_fan_in,
                self.max_fan_out,
            ))
        }
    }
}

impl<T> HeapTopicBuilder<T>
where
    T: Debug + Send + TypeTag + Coherent + Reloc,
{
    #[inline(always)]
    pub(crate) fn new() -> Self {
        Self {
            _phantom_u: std::marker::PhantomData,

            max_queue_depth: 1,
            queue_policy: QueuePolicy::Error,
            max_fan_in: 1,
            max_fan_out: 1,
        }
    }
}
