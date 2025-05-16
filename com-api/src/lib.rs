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

//! # API Design principles
//!
//! - We stick to the builder pattern down to a single service
//! - We make all elements mockable. This means we provide traits for the building blocks.
//!   We strive for enabling trait objects for mockable entities.
//! - We allow for the allocation of heap memory during initialization phase (offer, connect, ...)
//!   but prevent heap memory usage during the running phase. Any heap memory allocations during the
//!   run phase must happen on preallocated memory chunks.
//! - We support data provision on potentially uninitialized memory for efficiency reasons.
//! - We want to be type safe by deriving as much type information as possible from the interface
//!   description.
//! - We want to design interfaces such that they're hard to misuse.
//! - Data communicated over IPC need to be position independent.
//! - Data may not reference other data outside its provenance.
//! - We provide simple defaults for customization points.
//! - Sending / receiving / calling is always done explicitly.
//! - COM API does not enforce the necessity for internal threads or executors.
//!
//! # Lower layer implementation principles
//!
//! - We add safety nets to detect ABI incompatibilities
//!
//! # Supported IPC ABI
//!
//! - Primitives
//! - Static lists / strings
//! - Dynamic lists / strings
//! - Key-value
//! - Structures
//! - Tuples

use std::fmt::Debug;
use std::mem::MaybeUninit;
use std::ops::{Deref, DerefMut};
use std::path::Path;
use std::time::SystemTime;

#[derive(Debug)]
pub enum Error {
    /// TODO: To be replaced, dummy value for "something went wrong"
    Fail,
    Timeout,
}

pub type Result<T> = std::result::Result<T, Error>;

pub trait Builder {
    type Output;
    /// Open point: Should this be &mut self so that this can be turned into a trait object?
    fn build(self) -> Result<Self::Output>;
}

/// This represents the com implementation and acts as a root for all types and objects provided by
/// the implementation.
pub trait Runtime {
    type InstanceSpecifier;
}

pub trait RuntimeBuilder: Builder
where
    <Self as Builder>::Output: Runtime,
{
    fn load_config(&mut self, config: &Path) -> &mut Self;
}

/// This trait shall ensure that we can safely use an instance of the implementing type across
/// address boundaries. This property may be violated by the following circumstances:
/// - usage of pointers to other members of the struct itself (akin to !Unpin structs)
/// - usage of Rust pointers or references to other data
///
/// This can be trivially achieved by not using any sort of reference. In case a reference (either
/// to self or to other data) is required, the following options exist:
/// - Use indices into other data members of the same structure
/// - Use offset pointers _to the same memory chunk_ that point to different (external) data
///
/// # Safety
///
/// Since it is yet to be proven whether this trait can be implemented safely (assumption is: no) it
/// is unsafe for now. The expectation is that very few users ever need to implement this manually.
pub unsafe trait Reloc {}

unsafe impl Reloc for () {}
unsafe impl Reloc for u32 {}

/// A `Sample` provides a reference to a memory buffer of an event with immutable value.
///
/// By implementing the `Deref` trait implementations of the trait support the `.` operator for dereferencing.
/// The buffers with its data lives as long as there are references to it existing in the framework.
pub trait Sample<T>: Deref<Target = T> + Send
where
    T: Send + Reloc,
{
}

/// A `SampleMut` provides a reference to a memory buffer of an event with mutable value.
///
/// By implementing the `DerefMut` trait implementations of the trait support the `.` operator for dereferencing.
/// The buffers with its data lives as long as there are references to it existing in the framework.
pub trait SampleMut<T>: DerefMut<Target = T>
where
    T: Send + Reloc,
{
    /// The associated read-only sample type.
    type Sample: Sample<T>;

    /// Consume the sample into an immutable sample.
    fn into_sample(self) -> Self::Sample;

    /// Send the sample and consume it.
    fn send(self) -> Result<()>;
}

/// A `SampleMaybeUninit` provides a reference to a memory buffer of an event with a `MaybeUninit` value.
///
/// Utilizing `DerefMut` on the buffer reveals a reference to the internal `MaybeUninit<T>`.
/// The buffer can be assumed initialized with mutable access by calling `assume_init` which returns a `SampleMut`.
/// The buffers with its data lives as long as there are references to it existing in the framework.
pub trait SampleMaybeUninit<T>: DerefMut<Target = MaybeUninit<T>>
where
    T: Send + Reloc,
{
    /// Buffer type for mutable data after initialization
    type SampleMut: SampleMut<T>;

    /// Write a value into the buffer and render it initialized.
    ///
    /// This corresponds to `MaybeUninit::write`.
    fn write(self, value: T) -> Self::SampleMut;

    /// Render the buffer initialized for mutable access.
    ///
    /// This corresponds to `MaybeUninit::assume_init`.
    ///
    /// # Safety
    ///
    /// The caller has to make sure to initialize the data in the buffer before calling this method.
    unsafe fn assume_init_2(self) -> Self::SampleMut;
}

pub trait Interface {}

pub trait ProducerBuilder<R: Runtime>: Builder {}
pub trait ConsumerBuilder<R: Runtime>: Builder {}

pub trait InstanceBuilder<I: Interface>:
    Builder<Output: Instance<Self::Runtime, Interface = I>>
{
    type Runtime: Runtime;
}

pub trait Instance<R: Runtime> {
    type Interface: Interface;
    type ProducerBuilder: ProducerBuilder<R>;
    type ConsumerBuilder: ConsumerBuilder<R>;

    fn producer(&self) -> Self::ProducerBuilder;
    fn consumer(&self) -> Self::ConsumerBuilder;
}

pub trait Subscriber<T: Reloc + Send> {
    fn receive_blocking(&self) -> Result<impl Sample<T>>;
    fn try_receive(&self) -> Result<Option<impl Sample<T>>>;
    fn receive_until(&self, until: SystemTime) -> Result<impl Sample<T>>;
    /*fn next<'a>(&'a self) -> impl Future<Output = Result<Sample<'a, T>>>
    where
        T: 'a;*/
    fn receive<'a>(&'a self) -> impl Future<Output = Result<impl Sample<T> + 'a>> + Send
    where
        T: 'a;
}
