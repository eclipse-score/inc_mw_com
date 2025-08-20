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

//! This crate provides a mock implementation of the COM API for testing purposes.
//! It is meant to be used in conjunction with the `com-api` crate.
//! The mock implementation does not perform any real IPC and is not meant to be used in production.
//! It is only meant to be used for testing and development.

#![allow(dead_code)]

use std::cmp::Ordering;
use std::collections::VecDeque;
use std::marker::PhantomData;
use std::mem::MaybeUninit;
use std::ops::{Deref, DerefMut};
use std::path::Path;
use std::sync::atomic::AtomicUsize;

use com_api_concept::{
    AdapterConcept, BuilderConcept, ConsumerBuilderConcept, ConsumerDescriptorConcept,
    InstanceSpecifier, InterfaceConcept, Reloc, SampleConcept, SampleContainer,
    SampleMaybeUninitConcept, SampleMutConcept, ServiceDiscoveryConcept, SubscriberConcept,
    SubscriptionConcept,
};

pub struct MockAdapter {}

impl AdapterConcept for MockAdapter {
    type Sample<'a, T: Reloc + Send + 'a + std::fmt::Debug> = MockSample<'a, T>;
}

impl MockAdapter {
    // TODO: Any chance that these can be moved to a trait so that this becomes more testable?
    // If yes, this trait is certainly located here since
    pub fn find_service<I: InterfaceConcept>(
        &self,
        _instance_specifier: InstanceSpecifier,
    ) -> MockConsumerDiscovery<I> {
        MockConsumerDiscovery {
            _interface: PhantomData,
        }
    }

    pub fn producer_builder<I: InterfaceConcept>(
        &self,
        instance_specifier: InstanceSpecifier,
    ) -> MockProducerBuilder<I> {
        MockProducerBuilder::new(self, instance_specifier)
    }
}

struct MockEvent<T> {
    event: PhantomData<T>,
}

struct MockBinding<'a, T>
where
    T: Send,
{
    data: *mut T,
    event: &'a MockEvent<T>,
}

unsafe impl<'a, T> Send for MockBinding<'a, T> where T: Send {}

enum SampleBinding<'a, T>
where
    T: Send,
{
    Mock(MockBinding<'a, T>),
    Test(Box<T>),
}

pub struct MockSample<'a, T>
where
    T: Reloc + Send,
{
    id: usize,
    inner: SampleBinding<'a, T>,
}

static ID_COUNTER: AtomicUsize = AtomicUsize::new(0);

impl<'a, T> From<T> for MockSample<'a, T>
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

impl<'a, T> Deref for MockSample<'a, T>
where
    T: Reloc + Send,
{
    type Target = T;

    fn deref(&self) -> &Self::Target {
        match &self.inner {
            SampleBinding::Mock(_mock) => unimplemented!(),
            SampleBinding::Test(test) => test.as_ref(),
        }
    }
}

impl<'a, T> SampleConcept<T> for MockSample<'a, T> where T: Send + Reloc {}

impl<'a, T> PartialEq for MockSample<'a, T>
where
    T: Send + Reloc,
{
    fn eq(&self, other: &Self) -> bool {
        self.id == other.id
    }
}

impl<'a, T> Eq for MockSample<'a, T> where T: Send + Reloc {}

impl<'a, T> PartialOrd for MockSample<'a, T>
where
    T: Send + Reloc,
{
    fn partial_cmp(&self, other: &Self) -> Option<Ordering> {
        Some(self.cmp(other))
    }
}

impl<'a, T> Ord for MockSample<'a, T>
where
    T: Send + Reloc,
{
    fn cmp(&self, other: &Self) -> Ordering {
        self.id.cmp(&other.id)
    }
}

pub struct MockSampleMut<'a, T>
where
    T: Reloc,
{
    data: T,
    _lifetime: PhantomData<&'a T>,
}

impl<'a, T> SampleMutConcept<T> for MockSampleMut<'a, T>
where
    T: Reloc + Send,
{
    type Sample = MockSample<'a, T>;

    fn into_sample(self) -> Self::Sample {
        todo!()
    }

    fn send(self) -> com_api_concept::Result<()> {
        todo!()
    }
}

impl<'a, T> Deref for MockSampleMut<'a, T>
where
    T: Reloc,
{
    type Target = T;

    fn deref(&self) -> &Self::Target {
        &self.data
    }
}

impl<'a, T> DerefMut for MockSampleMut<'a, T>
where
    T: Reloc,
{
    fn deref_mut(&mut self) -> &mut Self::Target {
        &mut self.data
    }
}

pub struct MockSampleMaybeUninit<'a, T>
where
    T: Reloc + Send,
{
    data: MaybeUninit<T>,
    _lifetime: PhantomData<&'a T>,
}

impl<'a, T> SampleMaybeUninitConcept<T> for MockSampleMaybeUninit<'a, T>
where
    T: Reloc + Send,
{
    type SampleMut = MockSampleMut<'a, T>;

    fn write(self, val: T) -> Self::SampleMut {
        Self::SampleMut {
            data: val,
            _lifetime: PhantomData,
        }
    }

    unsafe fn assume_init(self) -> Self::SampleMut {
        Self::SampleMut {
            data: unsafe { self.data.assume_init() },
            _lifetime: PhantomData,
        }
    }
}

pub struct MockSubscribable<T> {
    _data: PhantomData<T>,
}

impl<T> Default for MockSubscribable<T> {
    fn default() -> Self {
        Self { _data: PhantomData }
    }
}

impl<T: Reloc + Send> SubscriberConcept<T> for MockSubscribable<T> {
    type Subscription = MockSubscriber<T>;

    fn subscribe(self, _max_num_samples: usize) -> com_api_concept::Result<Self::Subscription> {
        Ok(MockSubscriber::new())
    }
}

#[derive(Default)]
pub struct MockSubscriber<T>
where
    T: Reloc + Send,
{
    data: VecDeque<T>,
}

impl<T> MockSubscriber<T>
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

impl<T> SubscriptionConcept<T> for MockSubscriber<T>
where
    T: Reloc + Send,
{
    type Subscriber = MockSubscribable<T>;
    type Sample<'a>
        = MockSample<'a, T>
    where
        T: 'a;

    fn unsubscribe(self) -> Self::Subscriber {
        Default::default()
    }

    fn try_receive<'a>(
        &'a self,
        _scratch: &'_ mut SampleContainer<Self::Sample<'a>>,
        _max_samples: usize,
    ) -> com_api_concept::Result<usize> {
        todo!()
    }

    #[allow(clippy::manual_async_fn)]
    fn receive<'a>(
        &'a self,
        _scratch: &'_ mut SampleContainer<Self::Sample<'a>>,
        _new_samples: usize,
        _max_samples: usize,
    ) -> impl Future<Output = com_api_concept::Result<usize>> + Send {
        async { todo!() }
    }
}

pub struct MockPublisher<T> {
    _data: PhantomData<T>,
}

impl<T> Default for MockPublisher<T>
where
    T: Reloc + Send,
{
    fn default() -> Self {
        Self::new()
    }
}

impl<T> MockPublisher<T>
where
    T: Reloc + Send,
{
    pub fn new() -> Self {
        Self { _data: PhantomData }
    }

    pub fn allocate<'a>(&'a self) -> com_api_concept::Result<MockSampleMaybeUninit<'a, T>> {
        Ok(MockSampleMaybeUninit {
            data: MaybeUninit::uninit(),
            _lifetime: PhantomData,
        })
    }
}

pub struct MockConsumerDiscovery<I> {
    _interface: PhantomData<I>,
}

impl<I> MockConsumerDiscovery<I> {
    fn new(_runtime: &MockAdapter, _instance_specifier: InstanceSpecifier) -> Self {
        Self {
            _interface: PhantomData,
        }
    }
}

impl<I: InterfaceConcept> ServiceDiscoveryConcept<I, MockAdapter> for MockConsumerDiscovery<I>
where
    MockConsumerBuilder<I>: ConsumerBuilderConcept<I, MockAdapter>,
{
    type ConsumerBuilder = MockConsumerBuilder<I>;
    type ServiceEnumerator = Vec<MockConsumerBuilder<I>>;

    fn get_available_instances(&self) -> com_api_concept::Result<Self::ServiceEnumerator> {
        Ok(Vec::new())
    }
}

pub struct MockProducerBuilder<I: InterfaceConcept> {
    instance_specifier: InstanceSpecifier,
    _interface: PhantomData<I>,
}

impl<I: InterfaceConcept> MockProducerBuilder<I> {
    fn new(_runtime: &MockAdapter, instance_specifier: InstanceSpecifier) -> Self {
        Self {
            instance_specifier,
            _interface: PhantomData,
        }
    }
}

pub struct MockConsumerDescriptor<I: InterfaceConcept> {
    _interface: PhantomData<I>,
}

impl<I: InterfaceConcept> Clone for MockConsumerDescriptor<I> {
    fn clone(&self) -> Self {
        Self {
            _interface: PhantomData,
        }
    }
}

pub struct MockConsumerBuilder<I: InterfaceConcept> {
    instance_specifier: InstanceSpecifier,
    _interface: PhantomData<I>,
}

impl<I: InterfaceConcept> ConsumerDescriptorConcept<MockAdapter> for MockConsumerBuilder<I> {
    fn get_instance_id(&self) -> usize {
        todo!()
    }
}

pub struct MockAdapterBuilder {}

impl BuilderConcept<MockAdapter> for MockAdapterBuilder {
    fn build(self) -> com_api_concept::Result<MockAdapter> {
        Ok(MockAdapter {})
    }
}

/// Entry point for the default implementation for the com module of s-core
impl com_api_concept::AdapterBuilderConcept<MockAdapter> for MockAdapterBuilder {
    fn load_config(&mut self, _config: &Path) -> &mut Self {
        self
    }
}

impl Default for MockAdapterBuilder {
    fn default() -> Self {
        Self::new()
    }
}

impl MockAdapterBuilder {
    /// Creates a new instance of the default implementation of the com layer
    pub fn new() -> Self {
        Self {}
    }
}

#[cfg(test)]
mod test {
    use com_api_concept::{SampleContainer, SubscriptionConcept};

    #[test]
    fn receive_stuff() {
        let test_subscriber = super::MockSubscriber::<u32>::new();
        for _ in 0..10 {
            let mut sample_buf = SampleContainer::new();
            let receive_result = test_subscriber.try_receive(&mut sample_buf, 1);
            match receive_result {
                Ok(0) => panic!("No sample received"),
                Ok(x) => {
                    println!(
                        "{} samples received: sample[0] = {}",
                        x,
                        *sample_buf.front().unwrap()
                    )
                }
                Err(e) => panic!("{:?}", e),
            }
        }
    }

    #[test]
    fn receive_async_stuff() {
        let test_subscriber = super::MockSubscriber::<u32>::new();
        // block on an asynchronous reception of data from test_subscriber
        futures::executor::block_on(async {
            let mut sample_buf = SampleContainer::new();
            match test_subscriber.receive(&mut sample_buf, 1, 1).await {
                Ok(0) => panic!("No sample received"),
                Ok(x) => {
                    println!(
                        "{} samples received: sample[0] = {}",
                        x,
                        *sample_buf.front().unwrap()
                    )
                }
                Err(e) => panic!("{:?}", e),
            }
        })
    }
}
