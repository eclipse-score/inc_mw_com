// Copyright (c) 2025 Qorix GmbH
//
// This program and the accompanying materials are made available under the
// terms of the Apache License, Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0.
//
// SPDX-License-Identifier: Apache-2.0

use super::*;

use std::{
    fmt::Debug,
    future::Future,
    ops::{Deref, DerefMut},
    time::Duration,
};

////////////////////////////////////////////////////////////////
// Event: Pub/Sub Messaging Pattern

/// A `Sample` provides a reference to a memory buffer of an event with immutable value.
///
/// By implementing the `Deref` trait implementations of the trait support the `.` operator for dereferencing.
/// The buffers with its data lives as long as there are references to it existing in the framework.
pub trait Sample<A, T>: Debug + Deref<Target = T> + Send
where
    A: TransportAdapter + ?Sized,
    T: Debug + Send + TypeTag + Coherent + Reloc,
{
}

/// A `SampleMut` provides a reference to a memory buffer of an event with mutable value.
///
/// By implementing the `DerefMut` trait implementations of the trait support the `.` operator for dereferencing.
/// The buffers with its data lives as long as there are references to it existing in the framework.
pub trait SampleMut<A, T>: Debug + DerefMut<Target = T>
where
    A: TransportAdapter + ?Sized,
    T: Debug + Send + TypeTag + Coherent + Reloc,
{
    /// The associated read-only sample type.
    type Sample: Sample<A, T>;

    /// Consume the sample into an immutable sample.
    fn into_sample(self) -> Self::Sample;

    /// Send the sample and consume it.
    fn send(self) -> ComResult<()>;
}

/// A `SampleMaybeUninit` provides a reference to a memory buffer of an event with a `MaybeUninit` value.
///
/// Utilizing `DerefMut` on the buffer reveals a reference to the internal `MaybeUninit<T>`.
/// The buffer can be assumed initialized with mutable access by calling `assume_init` which returns a `SampleMut`.
/// The buffers with its data lives as long as there are references to it existing in the framework.
pub trait SampleMaybeUninit<A, T>: Debug
where
    A: TransportAdapter + ?Sized,
    T: Debug + Send + TypeTag + Coherent + Reloc,
{
    /// Buffer type for mutable data after initialization
    type SampleMut: SampleMut<A, T>;

    /// Write a value into the buffer and render it initialized.
    ///
    /// This corresponds to `MaybeUninit::write`.
    fn write(self, value: T) -> Self::SampleMut;

    /// Get a mutable pointer to the internal maybe uninitialized `T`.
    ///
    /// # Safety
    ///
    /// The caller has to make sure to initialize the data in the buffer.
    /// Reading from the received pointer before initialization is undefined behavior.
    fn as_mut_ptr(&mut self) -> *mut T;

    /// Render the buffer initialized for mutable access.
    ///
    /// This corresponds to `MaybeUninit::assume_init`.
    ///
    /// # Safety
    ///
    /// The caller has to make sure to initialize the data in the buffer before calling this method.
    unsafe fn assume_init(self) -> Self::SampleMut;
}

/// The `Publisher` represents a publisher to a event.
///
/// The publishing application obtains instances implementing this trait through
/// events created by a transport adapter.
pub trait Publisher<A, T>: Debug + Send
where
    A: TransportAdapter + ?Sized,
    T: Debug + Send + TypeTag + Coherent + Reloc,
{
    /// The type of the uninitialized buffer for new data to be published.
    type SampleMaybeUninit: SampleMaybeUninit<A, T>;

    /// Loan an unitialized sample for new data to be published.
    ///
    /// # Example
    ///
    /// ```rust
    /// let adapter = HeapAdapter::new();
    ///
    /// let event = adapter.event::<u32>().build().unwrap();
    /// let publisher = event.publisher().unwrap();
    ///
    /// let sample = publisher.sample_uninit().unwrap();
    /// let sample = sample.write(42);
    /// sample.send();
    /// ```
    fn loan_uninit(&self) -> ComResult<Self::SampleMaybeUninit>;

    /// Loan a sample with initialized data to be published.
    ///
    /// The implementation copies the given value into the buffer.
    /// The type of the sample returned matches the sample types for the adapter used and is compatible with the SampleMaybeUninit type.
    ///
    /// The signature reads as `loan_with(&self, value: T) -> Result<SampleMut<T>>`.
    ///
    /// # Example
    ///
    /// ```rust
    /// let adapter = HeapAdapter::new();
    ///
    /// let event = adapter.event::<u32>().build().unwrap();
    /// let publisher = event.publisher().unwrap();
    ///
    /// let sample = publisher.loan(42).unwrap();
    /// sample.send();
    /// ```
    fn loan(
        &self,
        value: T,
    ) -> Result<
        <<Self as Publisher<A, T>>::SampleMaybeUninit as SampleMaybeUninit<A, T>>::SampleMut,
        ComError,
    > {
        let sample = self.loan_uninit()?;
        Ok(sample.write(value))
    }

    /// Public new data as copy to the event.
    ///
    /// This is a convenience function for quickly publishing new data.
    #[inline(always)]
    fn publish(&self, value: T) -> ComResult<()> {
        self.loan(value)?.send()
    }
}

/// The `Subscriber` trait represents the receiving end of published data on a event.
pub trait Subscriber<A, T>: Debug + Send
where
    A: TransportAdapter + ?Sized,
    T: Debug + Send + TypeTag + Coherent + Reloc,
{
    type Sample: Sample<A, T>;

    /// Check for new data and consume it if present.
    fn try_receive(&self) -> ComResult<Option<Self::Sample>>;

    /// Wait for new data to arrive.
    ///
    /// Upon success the method returns a receive buffer containing the new data.
    fn receive(&self) -> ComResult<Self::Sample>;

    /// Wait for new data to arrive with a timeout.
    ///
    /// The `ComResult` of the tuple contains the upon success a receive buffer containing the new data.
    fn receive_timeout(&self, timeout: Duration) -> ComResult<Self::Sample>;

    /// Receive new data asynchronously.
    ///
    /// This method returns a future that can be used to `await` the arrival of new data.
    fn receive_async(&self) -> impl Future<Output = ComResult<Self::Sample>>;

    /// Receive new data asynchronously with timeout.
    ///
    /// This method returns a future that can be used to `await` the arrival of new data.
    fn receive_timeout_async(
        &self,
        timeout: Duration,
    ) -> impl Future<Output = ComResult<Self::Sample>>;
}

/// The `Event` trait references to the implementation of a event in the underlying transport framework.
///
/// A event can only be created through the `event` method of the corresponding transport adapter.
/// The publishers and subscribers can be obtained through the events `publisher` and `subscriber` methods, respectively.
///
pub trait Event<A, T>: Debug + Clone + Send
where
    A: TransportAdapter + ?Sized,
    T: Debug + Send + TypeTag + Coherent + Reloc,
{
    type Publisher: Publisher<A, T>;
    type Subscriber: Subscriber<A, T>;

    /// Get a publisher for this event
    ///
    /// The method succees if the event still accepts new publishers.
    /// The fan-in of a event is limited by the configuration given to the builder when creating the event.
    fn publisher(&self) -> ComResult<Self::Publisher>;

    /// Get a subscriber for this event
    ///
    /// The method succeeds if the event still accepts new subscribers.
    /// The fan-out of a event is limited by the configuration given to the builder when creating the event.    
    fn subscriber(&self) -> ComResult<Self::Subscriber>;
}

/// The Builder for a `Event`
pub trait EventBuilder<A, T>: Debug
where
    A: TransportAdapter + ?Sized,
    T: TypeTag + Coherent + Reloc + Send + Debug,
{
    type Event: Event<A, T>;

    /// Set the queue depth of the event.
    fn with_queue_depth(self, queue_depth: usize) -> Self;

    /// Set the queue policy of the event.
    fn with_queue_policy(self, queue_policy: QueuePolicy) -> Self;

    /// Set the fan-in of the event which limits the number of publishers.
    fn with_max_fan_in(self, fan_in: usize) -> Self;

    /// Set the fan-out of the event which limits the number of subscribers.
    fn with_max_fan_out(self, fan_out: usize) -> Self;

    /// Build the event
    fn build(self) -> ComResult<Self::Event>;
}
