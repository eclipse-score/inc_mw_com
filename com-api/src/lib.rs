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

use crate::sample_impl::{Publisher, Subscriber};
use std::fmt::Debug;
use std::mem::MaybeUninit;
use std::ops::{Deref, DerefMut};
use std::path::Path;

#[derive(Debug)]
pub enum Error {
    Fail,
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

    // TBD: HOW?!?
    /*
    type Instance<I>: Instance<I>
    where
        I: Interface;

    fn make_instance<I: Interface>(
        &self,
        instance_specifier: Self::InstanceSpecifier,
    ) -> Self::Instance<I>;*/
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
    unsafe fn assume_init(self) -> Self::SampleMut;
}

pub struct InstanceSpecifier {}

// interface Auto {
//     linkes_rad: Event<Rad>,
//     auspuff: Event<Auspuff>,
//     set_blinker_zustand: FnMut(blinkmodus: BlinkModus) -> Result<bool>,
// }

struct Rad {}

struct Auspuff {}

pub trait ProducerBuilder: Builder {}
pub trait ConsumerBuilder: Builder {}

pub trait Interface {}

pub trait Instance<I: Interface> {
    type ProducerBuilder: ProducerBuilder;
    type ConsumerBuilder: ConsumerBuilder;

    fn producer(&self) -> Self::ProducerBuilder;
    fn consumer(&self) -> Self::ConsumerBuilder;
}

pub trait SampleInstance<I: Interface> {
    type Instance;
    fn new(instance_specifier: InstanceSpecifier) -> Self::Instance;
    type ProducerBuilder: ProducerBuilder;
    type ConsumerBuilder: ConsumerBuilder;

    fn producer(&self) -> Self::ProducerBuilder;
    fn consumer(&self) -> Self::ConsumerBuilder;
}

/// Generic
struct AutoInterface {}

/// Generic
impl Interface for AutoInterface {}

/// Runtime specific
struct AutoInstance {
    instance_specifier: InstanceSpecifier,
}

impl SampleInstance<AutoInterface> for AutoInterface {
    type Instance = AutoInstance;

    fn new(instance_specifier: InstanceSpecifier) -> Self::Instance {
        AutoInstance { instance_specifier }
    }

    type ProducerBuilder = AutoProducerBuilder;
    type ConsumerBuilder = AutoConsumerBuilder;

    fn producer(&self) -> Self::ProducerBuilder {
        AutoProducerBuilder {}
    }

    fn consumer(&self) -> Self::ConsumerBuilder {
        AutoConsumerBuilder {}
    }
}

struct AutoProducerBuilder {}

impl Builder for AutoProducerBuilder {
    type Output = AutoProducer;

    fn build(self) -> Result<Self::Output> {
        todo!()
    }
}

impl ProducerBuilder for AutoProducerBuilder {}

struct AutoConsumerBuilder {}

impl Builder for AutoConsumerBuilder {
    type Output = AutoConsumer;

    fn build(self) -> Result<Self::Output> {
        todo!()
    }
}

impl ConsumerBuilder for AutoConsumerBuilder {}

struct AutoProducer {
    linkes_rad: Publisher<Rad>,
    auspuff: Publisher<Auspuff>,
}

struct AutoConsumer {
    linkes_rad: Subscriber<Rad>,
    auspuff: Subscriber<Auspuff>,
}

mod sample_impl;

#[cfg(test)]
mod test {
    use super::*;
    use std::time::Duration;

    #[test]
    fn create_producer() {
        let runtime_builder = sample_impl::RuntimeBuilderImpl::new();
        let runtime = runtime_builder.build().unwrap();
        let builder = runtime.make_instance::<AutoInterface>(InstanceSpecifier {});
        //.key_str("key", "value");
        let _producer = builder.producer().build().unwrap();
        let _consumer = builder.consumer().build().unwrap();
    }

    #[test]
    fn receive_stuff() {
        let test_subscriber = crate::sample_impl::Subscriber::<u32>::new();
        for _ in 0..10 {
            match test_subscriber.wait(Some(Duration::from_secs(5))) {
                crate::sample_impl::WaitResult::SamplesAvailable => match test_subscriber.next() {
                    Ok(Some(sample)) => println!("{}", *sample),
                    Ok(None) => println!("Nothing"),
                    Err(e) => eprintln!("{:?}", e),
                },
                crate::sample_impl::WaitResult::Expired => println!("No sample received"),
            }
        }
    }

    #[test]
    fn send_stuff() {
        let test_publisher = crate::sample_impl::Publisher::new();
        for _ in 0..5 {
            let sample = test_publisher.allocate();
            match sample {
                Ok(mut sample) => {
                    let init_sample = unsafe {
                        *sample.as_mut_ptr() = 42u32;
                        sample.assume_init()
                    };
                    assert!(init_sample.send().is_ok());
                }
                Err(e) => eprintln!("Oh my! {:?}", e),
            }
        }
    }

    fn is_sync<T: Sync>(_val: T) {}

    #[test]
    fn builder_is_sync() {
        is_sync(crate::sample_impl::RuntimeBuilderImpl::new());
    }

    #[test]
    fn build_production_runtime() {}
}
