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

use std::marker::PhantomData;
use std::mem::MaybeUninit;
use std::ops::{Deref, DerefMut};
use std::path::Path;
use std::time::SystemTime;

use com_api::{Builder, Reloc, Runtime, Subscriber};

pub struct InstanceSpecifier {}

pub struct RuntimeImpl {}
impl Runtime for RuntimeImpl {
    type InstanceSpecifier = InstanceSpecifier;
}

impl RuntimeImpl {}

pub struct RuntimeBuilderImpl {}

/// Generic trait for all "factory-like" types
impl Builder for RuntimeBuilderImpl {
    type Output = RuntimeImpl;
    fn build(self) -> com_api::Result<Self::Output> {
        Ok(Self::Output {})
    }
}

/// Entry point for the default implementation for the com module of s-core
impl com_api::RuntimeBuilder for RuntimeBuilderImpl {
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
    inner: SampleBinding<'a, T>,
}

impl<'a, T> From<T> for Sample<'a, T>
where
    T: Reloc + Send,
{
    fn from(value: T) -> Self {
        Self {
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
    T: com_api::Reloc + Send,
{
    data: MaybeUninit<T>,
    _lifetime: PhantomData<&'a T>,
}

impl<T> Deref for SampleMaybeUninit<'_, T>
where
    T: Reloc + Send,
{
    type Target = MaybeUninit<T>;

    fn deref(&self) -> &Self::Target {
        &self.data
    }
}

impl<T> DerefMut for SampleMaybeUninit<'_, T>
where
    T: Reloc + Send,
{
    fn deref_mut(&mut self) -> &mut Self::Target {
        &mut self.data
    }
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

    unsafe fn assume_init_2(self) -> SampleMut<'a, T> {
        SampleMut {
            data: unsafe { self.data.assume_init() },
            _lifetime: PhantomData,
        }
    }
}

pub struct SubscriberImpl<T> {
    _data: PhantomData<T>,
}

impl<T> SubscriberImpl<T>
where
    T: Reloc + Send,
{
    pub fn new() -> Self {
        Self { _data: PhantomData }
    }
}

impl<T> Subscriber<T> for SubscriberImpl<T>
where
    T: Reloc + Send + Sync,
{
    fn receive_blocking(&self) -> com_api::Result<impl com_api::Sample<T>> {
        Err::<Sample<T>, _>(com_api::Error::Fail)
    }

    fn try_receive(&self) -> com_api::Result<Option<impl com_api::Sample<T>>> {
        Ok(None::<Sample<T>>)
    }

    fn receive_until(&self, _until: SystemTime) -> com_api::Result<impl com_api::Sample<T>> {
        Err::<Sample<T>, _>(com_api::Error::Timeout)
    }

    async fn receive<'a>(&'a self) -> com_api::Result<impl com_api::Sample<T> + 'a>
    where
        T: 'a,
    {
        Err::<Sample<T>, com_api::Error>(com_api::Error::Fail)
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
