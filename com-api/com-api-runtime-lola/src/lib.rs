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

//! This crate provides a LoLa implementation of the COM API for testing purposes.
//! It is meant to be used in conjunction with the `com-api` crate.

#![allow(dead_code)]

use std::cmp::Ordering;
use std::collections::VecDeque;
use std::marker::PhantomData;
use std::mem::MaybeUninit;
use std::ops::{Deref, DerefMut};
use std::path::Path;
use std::sync::atomic::AtomicUsize;

use com_api_concept::{
    Builder, ConsumerBuilder, ConsumerDescriptor, InstanceSpecifier, Interface, Reloc, Runtime,
    SampleContainer, ServiceDiscovery, Subscriber, Subscription,
};

pub struct LolaRuntimeImpl {}

impl Runtime for LolaRuntimeImpl {
    type Sample<'a, T: Reloc + Send + 'a + std::fmt::Debug> = Sample<'a, T>;
}

impl LolaRuntimeImpl {
    // TODO: Any chance that these can be moved to a trait so that this becomes more testable?
    // If yes, this trait is certainly located here since
    pub fn find_service<I: Interface>(
        &self,
        _instance_specifier: InstanceSpecifier,
    ) -> SampleConsumerDiscovery<I> {
        SampleConsumerDiscovery {
            _interface: PhantomData,
        }
    }

    pub fn producer_builder<I: Interface>(
        &self,
        instance_specifier: InstanceSpecifier,
    ) -> SampleProducerBuilder<I> {
        SampleProducerBuilder::new(self, instance_specifier)
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

impl<'a, T> com_api_concept::Sample<T> for Sample<'a, T> where T: Send + Reloc {}

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
    fn partial_cmp(&self, other: &Self) -> Option<Ordering> {
        Some(self.cmp(other))
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

impl<'a, T> com_api_concept::SampleMut<T> for SampleMut<'a, T>
where
    T: Reloc + Send,
{
    type Sample = Sample<'a, T>;

    fn into_sample(self) -> Self::Sample {
        todo!()
    }

    fn send(self) -> com_api_concept::Result<()> {
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

impl<'a, T> com_api_concept::SampleMaybeUninit<T> for SampleMaybeUninit<'a, T>
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

    fn subscribe(self, _max_num_samples: usize) -> com_api_concept::Result<Self::Subscription> {
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

pub struct Publisher<T> {
    _data: PhantomData<T>,
}

impl<T> Default for Publisher<T>
where
    T: Reloc + Send,
{
    fn default() -> Self {
        Self::new()
    }
}

impl<T> Publisher<T>
where
    T: Reloc + Send,
{
    pub fn new() -> Self {
        Self { _data: PhantomData }
    }

    pub fn allocate<'a>(&'a self) -> com_api_concept::Result<SampleMaybeUninit<'a, T>> {
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
    fn new(_runtime: &LolaRuntimeImpl, _instance_specifier: InstanceSpecifier) -> Self {
        Self {
            _interface: PhantomData,
        }
    }
}

impl<I: Interface> ServiceDiscovery<I, LolaRuntimeImpl> for SampleConsumerDiscovery<I>
where
    SampleConsumerBuilder<I>: ConsumerBuilder<I, LolaRuntimeImpl>,
{
    type ConsumerBuilder = SampleConsumerBuilder<I>;
    type ServiceEnumerator = Vec<SampleConsumerBuilder<I>>;

    fn get_available_instances(&self) -> com_api_concept::Result<Self::ServiceEnumerator> {
        Ok(Vec::new())
    }
}

pub struct SampleProducerBuilder<I: Interface> {
    instance_specifier: InstanceSpecifier,
    _interface: PhantomData<I>,
}

impl<I: Interface> SampleProducerBuilder<I> {
    fn new(_runtime: &LolaRuntimeImpl, instance_specifier: InstanceSpecifier) -> Self {
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

pub struct SampleConsumerBuilder<I: Interface> {
    instance_specifier: InstanceSpecifier,
    _interface: PhantomData<I>,
}

impl<I: Interface> ConsumerDescriptor<LolaRuntimeImpl> for SampleConsumerBuilder<I> {
    fn get_instance_id(&self) -> usize {
        todo!()
    }
}

pub struct RuntimeBuilderImpl {}

impl Builder<LolaRuntimeImpl> for RuntimeBuilderImpl {
    fn build(self) -> com_api_concept::Result<LolaRuntimeImpl> {
        Ok(LolaRuntimeImpl {})
    }
}

/// Entry point for the default implementation for the com module of s-core
impl com_api_concept::RuntimeBuilder<LolaRuntimeImpl> for RuntimeBuilderImpl {
    fn load_config(&mut self, _config: &Path) -> &mut Self {
        self
    }
}

impl Default for RuntimeBuilderImpl {
    fn default() -> Self {
        Self::new()
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
    use com_api_concept::{SampleContainer, Subscription};

    #[test]
    fn receive_stuff() {
        let test_subscriber = super::SubscriberImpl::<u32>::new();
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
        let test_subscriber = super::SubscriberImpl::<u32>::new();
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
