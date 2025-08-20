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
//! - We stick to the builder pattern down to a single service (TODO: Should this be introduced to the C++ API?)
//! - We make all elements mockable. This means we provide traits for the building blocks.
//!   We strive for enabling trait objects for mockable entities. (TODO: Inspect all consuming methods for adherence to this rule)
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

use std::collections::VecDeque;
use std::fmt::Debug;
use std::ops::{Deref, DerefMut};
use std::path::Path;

#[derive(Debug)]
pub enum Error {
    /// TODO: To be replaced, dummy value for "something went wrong"
    Fail,
    Timeout,
    AllocateFailed,
    SubscribeFailed,
}

pub type Result<T> = std::result::Result<T, Error>;

/// Generic trait for all "factory-like" types
pub trait Builder<Output> {
    /// TODO: Should this be &mut self so that this can be turned into a trait object?
    fn build(self) -> Result<Output>;
}

/// This represents the com implementation and acts as a root for all types and objects provided by
/// the implementation.
pub trait Runtime {
    type Sample<'a, T: Reloc + Send + std::fmt::Debug + 'a>: Sample<T>;
}

pub trait RuntimeBuilder<B>: Builder<B>
where
    B: Runtime,
{
    fn load_config(&mut self, config: &Path) -> &mut Self;
}

pub struct InstanceSpecifier {
    pub specifier: String,
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
///
/// The ordering of SamplePtrs is total over the reception order
// TODO: C++ doesn't yet support this. Expose API to compare SamplePtr ages.
pub trait Sample<T>: Deref<Target = T> + Send + PartialOrd + Ord
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
///
/// TODO: Shall we also require DerefMut<Target=MaybeUninit<T>> from implementing types? How to deal
/// TODO: with the ambiguous assume_init() then?
pub trait SampleMaybeUninit<T>
where
    T: Send + Reloc,
{
    /// Buffer type for mutable data after initialization
    type SampleMut: SampleMut<T>;
    /// Render the buffer initialized for mutable access.
    ///
    /// This corresponds to `MaybeUninit::assume_init`.
    ///
    /// TODO: Collision with MaybeUninit::assume_init() needs to be resolved.
    ///
    /// # Safety
    ///
    /// The caller has to make sure to initialize the data in the buffer before calling this method.
    //unsafe fn assume_init(self) -> Self::SampleMut;
    /// Write a value into the buffer and render it initialized.
    ///
    /// This corresponds to `MaybeUninit::write`.
    fn write(self, value: T) -> Self::SampleMut;
}

pub trait Interface {}

pub trait OfferedProducer {
    type Interface: Interface;
    type Producer: Producer<Interface = Self::Interface>;

    fn unoffer(self) -> Self::Producer;
}

pub trait Producer {
    type Interface: Interface;
    type OfferedProducer: OfferedProducer<Interface = Self::Interface>;

    fn offer(self) -> Result<Self::OfferedProducer>;
}

pub trait Consumer {}

pub trait ProducerBuilder<I: Interface, R: Runtime, P: Producer<Interface = I>>:
    Builder<P>
{
}

pub trait ServiceDiscovery<I: Interface, R: Runtime> {
    type ConsumerBuilder: ConsumerBuilder<I, R>;
    type ServiceEnumerator: IntoIterator<Item = Self::ConsumerBuilder>;

    fn get_available_instances(&self) -> Result<Self::ServiceEnumerator>;
    // TODO: Provide an async stream for newly available services / ServiceDescriptors
}

pub trait ConsumerDescriptor<R: Runtime> {
    fn get_instance_id(&self) -> usize; // TODO: Turn return type into separate type
}

pub trait ConsumerBuilder<I: Interface, R: Runtime>: ConsumerDescriptor<R> {}

pub trait Subscriber<T: Reloc + Send> {
    type Subscription: Subscription<T>;

    fn subscribe(self, max_num_samples: usize) -> Result<Self::Subscription>;
}

pub struct SampleContainer<S> {
    inner: VecDeque<S>,
}

impl<S> Default for SampleContainer<S> {
    fn default() -> Self {
        Self {
            inner: VecDeque::new(),
        }
    }
}

impl<S> SampleContainer<S> {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn iter<'a, T>(&'a self) -> impl Iterator<Item = &'a T>
    where
        S: Sample<T>,
        T: Reloc + Send + 'a,
    {
        self.inner.iter().map(<S as Deref>::deref)
    }

    pub fn pop_front(&mut self) -> Option<S> {
        self.inner.pop_front()
    }

    pub fn push_back(&mut self, new: S) -> Result<()> {
        self.inner.push_back(new);
        Ok(())
    }

    pub fn sample_count(&self) -> usize {
        self.inner.len()
    }

    pub fn front<T: Reloc + Send>(&self) -> Option<&T>
    where
        S: Sample<T>,
    {
        self.inner.front().map(<S as Deref>::deref)
    }
}

pub trait Subscription<T: Reloc + Send> {
    type Subscriber: Subscriber<T>;
    type Sample<'a>: Sample<T>
    where
        Self: 'a;

    fn unsubscribe(self) -> Self::Subscriber;

    /// Returns up to max_samples samples.
    ///
    /// This call polls for up to max_new samples from the IPC input buffers without blocking the
    /// calling thread.
    ///
    /// The method expects a (possibly empty) buffer that may contain samples from a previous call
    /// to `try_receive` of the same `Subscription` instance. If there are samples from other
    /// instances, the method fails. These sample pointers can then be reused by the method:
    ///
    /// TODO: How to make sure that the provided container contains samples from the same
    /// TODO: `Subscription` instance?
    ///
    /// - If there are less than max_samples in the communication buffer, the method will reuse
    ///   samples contained in the provided sample container, beginning from the last, going
    ///   backwards.
    /// - If the input container is empty on call, it will exclusively be filled with new samples.
    /// - If the input container contains more samples than max_samples before the call,
    ///   it will contain max_samples samples, potentially removing samples from
    ///   the container, even if no new samples were added.
    ///
    /// Returns the updated buffer and the number of newly added samples. All new samples are added
    /// to the back of the buffer, with the last sample being the newest. If less than max_samples
    /// could be added to the buffer, the samples that had been inside the buffer are retained, with
    /// the first samples getting removed as new samples are added to the back of the buffer.
    ///
    /// TODO: C++ cannot fully support this yet since there is no way to retain potentially-reusable
    /// TODO: samples.
    fn try_receive<'a>(
        &'a self,
        scratch: &'_ mut SampleContainer<Self::Sample<'a>>,
        max_samples: usize,
    ) -> Result<usize>;

    /// This method returns a future that resolves as soon as at least `new_samples` samples have
    /// been transferred from the communication buffer to the sample container.
    ///
    /// The replacement semantics as well as the post conditions of the resolved future are equal
    /// to `try_receive`.
    ///
    /// TODO: See above for C++ limitations.
    fn receive<'a>(
        &'a self,
        scratch: &'_ mut SampleContainer<Self::Sample<'a>>,
        new_samples: usize,
        max_samples: usize,
    ) -> impl Future<Output = Result<usize>> + Send;
}
