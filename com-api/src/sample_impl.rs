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

use crate::{Builder, Error, InstanceSpecifier, Interface, Reloc};
use std::marker::PhantomData;
use std::mem::MaybeUninit;
use std::ops::{Deref, DerefMut};
use std::path::Path;
use std::time::SystemTime;

pub struct SampleInstanceImpl<I: Interface> {
    _interface: PhantomData<I>,
    instance_specifier: InstanceSpecifier,
}

pub struct RuntimeImpl {}
impl crate::Runtime for RuntimeImpl {
    type InstanceSpecifier = InstanceSpecifier;

    // TBD: Impl for the to-be-done link between (runtime-agnostic) interface and (runtime.aware) instance

    /* type Instance<I>
        = SampleInstanceImpl<I>
    where
        I: Interface;

    fn make_instance<I: Interface>(
        &self,
        instance_specifier: Self::InstanceSpecifier,
    ) -> Self::Instance<I> {
        <I as SampleInstance<I>>::new(instance_specifier)
    } */
}

impl RuntimeImpl {
    pub fn make_instance<I: Interface>(
        &self,
        instance_specifier: <Self as crate::Runtime>::InstanceSpecifier,
    ) -> SampleInstanceImpl<I> {
        SampleInstanceImpl {
            _interface: PhantomData,
            instance_specifier,
        }
    }
}

pub struct RuntimeBuilderImpl {}

/// Generic trait for all "factory-like" types
impl Builder for RuntimeBuilderImpl {
    type Output = RuntimeImpl;
    fn build(self) -> crate::Result<Self::Output> {
        Ok(Self::Output {})
    }
}

/// Entry point for the default implementation for the com module of s-core
impl crate::RuntimeBuilder for RuntimeBuilderImpl {
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

impl<'a, T> super::Sample<T> for Sample<'a, T> where T: Send + Reloc {}

pub struct SampleMut<'a, T>
where
    T: Reloc,
{
    data: T,
    _lifetime: PhantomData<&'a T>,
}

impl<'a, T> crate::SampleMut<T> for SampleMut<'a, T>
where
    T: Reloc + Send,
{
    type Sample = Sample<'a, T>;

    fn into_sample(self) -> Self::Sample {
        todo!()
    }

    fn send(self) -> crate::Result<()> {
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
    T: crate::Reloc + Send,
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

impl<'a, T> crate::SampleMaybeUninit<T> for SampleMaybeUninit<'a, T>
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

    pub fn allocate(&self) -> super::Result<SampleMaybeUninit<T>> {
        Ok(SampleMaybeUninit {
            data: MaybeUninit::uninit(),
            _lifetime: PhantomData,
        })
    }
}

pub trait Subscriber<T: Reloc + Send> {
    fn receive_blocking(&self) -> super::Result<Sample<T>>;
    fn try_receive(&self) -> super::Result<Option<Sample<T>>>;
    fn receive_until(&self, until: SystemTime) -> super::Result<Sample<T>>;
    /*fn next<'a>(&'a self) -> impl Future<Output = super::Result<Sample<'a, T>>>
    where
        T: 'a;*/
    async fn receive<'a>(&'a self) -> super::Result<Sample<'a, T>>
    where
        T: 'a;
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
    T: Reloc + Send,
{
    fn receive_blocking(&self) -> super::Result<Sample<T>> {
        Err(Error::Fail)
    }

    fn try_receive(&self) -> super::Result<Option<Sample<T>>> {
        Ok(None)
    }

    fn receive_until(&self, _until: SystemTime) -> super::Result<Sample<T>> {
        Err(Error::Timeout)
    }

    async fn receive<'a>(&'a self) -> crate::Result<Sample<'a, T>>
    where
        T: 'a,
    {
        Err(Error::Fail)
    }
}
