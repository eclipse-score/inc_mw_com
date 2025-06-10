// Copyright (c) 2025 Contributors to the Eclipse Foundation
//
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
//
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// <https://www.apache.org/licenses/LICENSE-2.0>
//
// SPDX-License-Identifier: Apache-2.0

#![allow(dead_code)]

use std::cmp::Ordering;
use std::collections::VecDeque;
use std::marker::PhantomData;
use std::mem::MaybeUninit;
use std::ops::{Deref, DerefMut};
use std::path::Path;
use std::sync::atomic::AtomicUsize;

use com_api::{
    Builder, ConsumerBuilder, ConsumerDescriptor, InstanceSpecifier, Interface, Reloc, Runtime,
    SampleContainer, ServiceDiscovery, Subscriber, Subscription,
};

pub struct RuntimeImpl {}

impl Runtime for RuntimeImpl {
    type Sample<'a, T: Reloc + Send + 'a> = Sample<'a, T>;
}

impl RuntimeImpl {
    // TODO: Any chance that these can be moved to a trait so that this becomes more testable?
    // If yes, this trait is certainly located here since
    pub fn find_instance<I: Interface>(
        &self,
        instance_specifier: InstanceSpecifier,
    ) -> SampleConsumerDiscovery<I> {
        SampleConsumerDiscovery {
            _interface: PhantomData,
        }
    }

    pub fn create_provided_service<I: Interface>(
        &self,
        instance_specifier: InstanceSpecifier,
    ) -> SampleProducerBuilder<I> {
        SampleProducerBuilder::new(&self, instance_specifier)
    }
}

struct LolaEvent<T> {
    event: PhantomData<T>,
}

struct LolaBinding<'a, T>
where
    T: Send,
{
    data: *mut T,
    event: &'a LolaEvent<T>,
}

unsafe impl<'a, T> Send for LolaBinding<'a, T> where T: Send {}

enum SampleBinding<'a, T>
where
    T: Send,
{
    Lola(LolaBinding<'a, T>),
    Test(Box<T>),
}

pub struct Sample<'a, T>
where
    T: Reloc + Send,
{
    id: usize,
    inner: SampleBinding<'a, T>,
}

static ID_COUNTER: AtomicUsize = AtomicUsize::new(0);

impl<'a, T> From<T> for Sample<'a, T>
where
    T: Reloc + Send,
{
    fn from(value: T) -> Self {
        Self {
            id: ID_COUNTER.fetch_add(1, std::sync::atomic::Ordering::Relaxed),
            inner: SampleBinding::Test(Box::new(value)),
        }
    }
}

impl<'a, T> Deref for Sample<'a, T>
where
    T: Reloc + Send,
{
    type Target = T;

    fn deref(&self) -> &Self::Target {
        match &self.inner {
            SampleBinding::Lola(_lola) => unimplemented!(),
            SampleBinding::Test(test) => test.as_ref(),
        }
    }
}

impl<'a, T> com_api::Sample<T> for Sample<'a, T> where T: Send + Reloc {}

impl<'a, T> PartialEq for Sample<'a, T>
where
    T: Send + Reloc,
{
    fn eq(&self, other: &Self) -> bool {
        self.id == other.id
    }
}

impl<'a, T> Eq for Sample<'a, T> where T: Send + Reloc {}

impl<'a, T> PartialOrd for Sample<'a, T>
where
    T: Send + Reloc,
{
    fn partial_cmp(&self, other: &Self) -> Option<std::cmp::Ordering> {
        self.id.partial_cmp(&other.id)
    }
}

impl<'a, T> Ord for Sample<'a, T>
where
    T: Send + Reloc,
{
    fn cmp(&self, other: &Self) -> Ordering {
        self.id.cmp(&other.id)
    }
}

pub struct SampleMut<'a, T>
where
    T: Reloc,
{
    data: T,
    _lifetime: PhantomData<&'a T>,
}

impl<'a, T> com_api::SampleMut<T> for SampleMut<'a, T>
where
    T: Reloc + Send,
{
    type Sample = Sample<'a, T>;

    fn into_sample(self) -> Self::Sample {
        todo!()
    }

    fn send(self) -> com_api::Result<()> {
        todo!()
    }
}

impl<'a, T> Deref for SampleMut<'a, T>
where
    T: Reloc,
{
    type Target = T;

    fn deref(&self) -> &Self::Target {
        &self.data
    }
}

impl<'a, T> DerefMut for SampleMut<'a, T>
where
    T: Reloc,
{
    fn deref_mut(&mut self) -> &mut Self::Target {
        &mut self.data
    }
}

pub struct SampleMaybeUninit<'a, T>
where
    T: Reloc + Send,
{
    data: MaybeUninit<T>,
    _lifetime: PhantomData<&'a T>,
}

impl<'a, T> com_api::SampleMaybeUninit<T> for SampleMaybeUninit<'a, T>
where
    T: Reloc + Send,
{
    type SampleMut = SampleMut<'a, T>;

    fn write(self, val: T) -> SampleMut<'a, T> {
        SampleMut {
            data: val,
            _lifetime: PhantomData,
        }
    }

    unsafe fn assume_init(self) -> SampleMut<'a, T> {
        SampleMut {
            data: unsafe { self.data.assume_init() },
            _lifetime: PhantomData,
        }
    }
}

pub struct SubscribableImpl<T> {
    _data: PhantomData<T>,
}

impl<T> Default for SubscribableImpl<T> {
    fn default() -> Self {
        Self { _data: PhantomData }
    }
}

impl<T: Reloc + Send> Subscriber<T> for SubscribableImpl<T> {
    type Subscription = SubscriberImpl<T>;

    fn subscribe(self, max_num_samples: usize) -> com_api::Result<Self::Subscription> {
        Ok(SubscriberImpl::new())
    }
}

#[derive(Default)]
pub struct SubscriberImpl<T>
where
    T: Reloc + Send,
{
    data: VecDeque<T>,
}

impl<T> SubscriberImpl<T>
where
    T: Reloc + Send,
{
    pub fn new() -> Self {
        Self {
            data: Default::default(),
        }
    }

    pub fn add_data(&mut self, data: T) {
        self.data.push_front(data);
    }
}

impl<T> Subscription<T> for SubscriberImpl<T>
where
    T: Reloc + Send,
{
    type Subscriber = SubscribableImpl<T>;
    type Sample<'a>
        = Sample<'a, T>
    where
        T: 'a;

    fn unsubscribe(self) -> Self::Subscriber {
        Default::default()
    }

    fn try_receive<'a, C>(&self, scratch: C, max_samples: usize) -> (C, com_api::Result<usize>)
    where
        Self: 'a,
        T: 'a,
        C: SampleContainer<Self::Sample<'a>> + 'a,
    {
        todo!()
    }

    fn receive<'a, C>(
        &self,
        scratch: C,
        new_samples: usize,
        max_samples: usize,
    ) -> impl Future<Output = (C, com_api::Result<usize>)> + Send
    where
        Self: 'a,
        T: 'a,
        C: SampleContainer<Self::Sample<'a>> + 'a,
    {
        async { todo!() }
    }
}

pub struct Publisher<T> {
    _data: PhantomData<T>,
}

impl<T> Publisher<T>
where
    T: Reloc + Send,
{
    pub fn new() -> Self {
        Self { _data: PhantomData }
    }

    pub fn allocate(&self) -> com_api::Result<SampleMaybeUninit<T>> {
        Ok(SampleMaybeUninit {
            data: MaybeUninit::uninit(),
            _lifetime: PhantomData,
        })
    }
}

pub struct SampleConsumerDiscovery<I> {
    _interface: PhantomData<I>,
}

impl<I> SampleConsumerDiscovery<I> {
    fn new(_runtime: &RuntimeImpl, instance_specifier: InstanceSpecifier) -> Self {
        Self {
            _interface: PhantomData,
        }
    }
}

impl<I: Interface> ServiceDiscovery<I, RuntimeImpl> for SampleConsumerDiscovery<I>
where
    SampleConsumerDescriptor<I>: ConsumerDescriptor<I, RuntimeImpl>,
{
    type ConsumerDescriptor = SampleConsumerDescriptor<I>;
    type ServiceEnumerator = Vec<SampleConsumerDescriptor<I>>;

    fn get_available_instances(&self) -> com_api::Result<Self::ServiceEnumerator> {
        Ok(Vec::new())
    }
}

pub struct SampleProducerBuilder<I: Interface> {
    instance_specifier: InstanceSpecifier,
    _interface: PhantomData<I>,
}

impl<I: Interface> SampleProducerBuilder<I> {
    fn new(_runtime: &RuntimeImpl, instance_specifier: InstanceSpecifier) -> Self {
        Self {
            instance_specifier,
            _interface: PhantomData,
        }
    }
}

pub struct SampleConsumerDescriptor<I: Interface> {
    _interface: PhantomData<I>,
}

impl<I: Interface> Clone for SampleConsumerDescriptor<I> {
    fn clone(&self) -> Self {
        Self {
            _interface: PhantomData,
        }
    }
}

impl<I: Interface> ConsumerDescriptor<I, RuntimeImpl> for SampleConsumerDescriptor<I>
where
    SampleConsumerBuilder<I>: ConsumerBuilder<I, RuntimeImpl>,
{
    type ConsumerBuilder = SampleConsumerBuilder<I>;

    fn get_instance_id(&self) -> usize {
        todo!()
    }

    fn into_builder(self) -> Self::ConsumerBuilder {
        todo!()
    }
}

pub struct SampleConsumerBuilder<I: Interface> {
    instance_specifier: InstanceSpecifier,
    _interface: PhantomData<I>,
}

pub struct RuntimeBuilderImpl {}

impl Builder<RuntimeImpl> for RuntimeBuilderImpl {
    fn build(self) -> com_api::Result<RuntimeImpl> {
        Ok(RuntimeImpl {})
    }
}

/// Entry point for the default implementation for the com module of s-core
impl com_api::RuntimeBuilder<RuntimeImpl> for RuntimeBuilderImpl {
    fn load_config(&mut self, _config: &Path) -> &mut Self {
        self
    }
}

impl RuntimeBuilderImpl {
    /// Creates a new instance of the default implementation of the com layer
    pub fn new() -> Self {
        Self {}
    }
}

#[cfg(test)]
mod test {
    use com_api::Subscription;
    use std::collections::VecDeque;

    #[test]
    fn receive_stuff() {
        let test_subscriber = super::SubscriberImpl::<u32>::new();
        for _ in 0..10 {
            let sample_buf = VecDeque::new();
            match test_subscriber.try_receive(sample_buf, 1) {
                (_, Ok(0)) => panic!("No sample received"),
                (sample_buf, Ok(x)) => {
                    println!("{} samples received: sample[0] = {}", x, *sample_buf[0])
                }
                (_, Err(e)) => panic!("{:?}", e),
            }
        }
    }

    #[test]
    fn receive_async_stuff() {
        let test_subscriber = super::SubscriberImpl::<u32>::new();
        // block on an asynchronous reception of data from test_subscriber
        futures::executor::block_on(async {
            let sample_buf = VecDeque::new();
            match test_subscriber.receive(sample_buf, 1, 1).await {
                (_, Ok(0)) => panic!("No sample received"),
                (sample_buf, Ok(x)) => {
                    println!("{} samples received: sample[0] = {}", x, *sample_buf[0])
                }
                (_, Err(e)) => panic!("{:?}", e),
            }
        })
    }
}
