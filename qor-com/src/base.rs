// Copyright (c) 2025 Qorix GmbH
//
// This program and the accompanying materials are made available under the
// terms of the Apache License, Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0.
//
// SPDX-License-Identifier: Apache-2.0

//!
//! # Overview
//!
//! The base module defines the basic types and traits used in the qor-com stack.
//!
//! # Concepts
//!
//! The fundamental concept is based on a transport adapter logic.
//!
//! ## Data Access
//!
//! Data is always accessed through buffers. Three buffer types exist:
//!
//! - Buffer: Immutable data buffer
//! - BufferMut: Mutable data buffer
//! - BufferMaybeUninit: Buffer with uninitialized data that can convert into a BufferMut or Buffer when initialized.
//!
//! Buffers can be obtained only through providing methods of communication patterns.
//!
//! ## Communication Patterns
//!
//! The transport adapter provides methods to
//! produce builders for the different communication patterns:
//!
//! - Event
//! - Topic
//! - Remote Procedure
//!
//! Each of the elements can produce a sending and a receiving part.
//!
//! - Event: Notifier and Listener
//! - Topic: Publisher and Subscriber
//! - Remote Procedure: Invoker and Invokee
//!
//! ## API elements
//!
//! The elements of the API are expressed through traits. This allows for flexible implementations of the transport adapter.
//! As different transport adapter implementations may exist, all API elements are generic with respect to the transport adapter.
//!
//! The corresponding generic parameter is named `TA` in the API elements.
//!
//! ## Lifetime management
//!
//! The implementation of the API elements requires great care for the validity of all entities. Therefore all traits come with
//! strict lifecycle bounds to ensure the correct usage of the API.
//!
//! The lifetime parameters used in the implementation are
//!
//! - 's: The `Self` lifetime of each trait implementation
//! - 't: The lifetime of the underlying transport adapter
//! - 'n: The lifetime of successing buffers created from a MaybeUninit
//TODO: See if we can get rid of 'n if we use a local 'a for all buffers
//! - 'a: The lifetime given by the caller of a method.
//!  

use qor_core::prelude::*;

use std::{
    fmt::Debug,
    future::Future,
    ops::{Deref, DerefMut},
    time::Duration,
};

/// Communication module result type
pub type ComResult<T> = std::result::Result<T, Error>;

/// The `QueuePolicy` defines the overwrite policy of a queue when the queue is full.
///
#[derive(Debug, Clone, Copy)]
pub enum QueuePolicy {
    /// Raise an error if the queue is full.
    Error,

    /// Overwrite the oldest sample in the queue.
    OverwriteOldest,

    /// Overwrite the latest sample in the queue.
    OverwriteLatest,
}

/// The static configuration of a transport adapter.
///
/// The static configuration contains
//TODO: Move to qor-core
pub trait StaticConfig<A: Adapter + ?Sized>: Debug {
    /// Get the name of the adapter
    fn name() -> &'static str;

    /// Get the vendor of the adapter
    fn vendor() -> &'static str;

    /// Get the version of the adapter
    fn version() -> Version;
}

////////////////////////////////////////////////////////////////
//
// Infrastructure elements
//

/// The `NodeId` struct represents the unique identifier of a node in the communication network.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct NodeId(u32);

impl NodeId {
    pub const INVALID: NodeId = Self { 0: u32::MAX };
    pub const BROADCAST: NodeId = Self { 0: u32::MAX - 1 };
    pub const LOCAL: NodeId = Self { 0: u32::MAX - 2 };

    /// Create a new node id with the given id number
    #[inline(always)]
    pub const fn new(id: u32) -> Self {
        Self(id)
    }

    /// Check validity of node id
    #[inline(always)]
    pub const fn is_valid(&self) -> bool {
        self.0 != Self::INVALID.0
    }
}

impl From<u32> for NodeId {
    fn from(id: u32) -> Self {
        Self::new(id)
    }
}

impl Into<u32> for NodeId {
    fn into(self) -> u32 {
        self.0
    }
}

/// The `EndpointId` struct represents the unique identifier of an endpoint in the communication network.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct EndpointId(u32);

impl EndpointId {
    /// The invalid endpoint id
    pub const INVALID: EndpointId = Self { 0: u32::MAX };

    /// The broadcast endpoint id
    pub const BROADCAST: EndpointId = Self { 0: u32::MAX - 1 };

    /// The local endpoint id
    pub const LOCAL: EndpointId = Self { 0: u32::MAX - 2 };

    /// Create a new endpoint id with the given id number
    #[inline(always)]
    pub const fn new(id: u32) -> Self {
        Self(id)
    }

    /// Check validity of endpoint id
    #[inline(always)]
    pub const fn is_valid(&self) -> bool {
        self.0 != Self::INVALID.0
    }
}

impl From<u32> for EndpointId {
    fn from(id: u32) -> Self {
        Self::new(id)
    }
}

impl Into<u32> for EndpointId {
    fn into(self) -> u32 {
        self.0
    }
}

/// The address of an endpoint in the communication network.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct Address(u64);

impl Address {
    /// The invalid address
    pub const INVALID: Address = Address::new(NodeId::INVALID, EndpointId::INVALID);

    /// The address of the local node
    pub const LOCALNODE: Address = Address::new(NodeId::LOCAL, EndpointId::INVALID);

    /// The address of the broadcast node
    #[inline(always)]
    pub const fn new(node_id: NodeId, endpoint_id: EndpointId) -> Address {
        Address {
            0: (node_id.0 as u64) << 32 | endpoint_id.0 as u64,
        }
    }

    #[inline(always)]
    pub fn is_valid(&self) -> bool {
        self.0 != Address::INVALID.0
    }

    #[inline(always)]
    pub const fn node_id(&self) -> NodeId {
        NodeId::new((self.0 >> 32) as u32)
    }

    #[inline(always)]
    pub const fn endpoint_id(&self) -> EndpointId {
        EndpointId::new((self.0 & 0xFFFF_FFFF) as u32)
    }
}

/// The `Nodematch` struct is a selector for NodeIds through a mask and a set bit pattern.
///
/// The match is defined as `match := (node_id & mask) == set`.
///
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct Nodematch(u32, u32);

impl Nodematch {
    /// The any nodemask
    pub const ANY: Nodematch = Nodematch::new(0x0000_0000, 0x0000_0000);

    /// Create a new nodemask with the given mask and set pattern
    #[inline(always)]
    pub const fn new(mask: u32, set: u32) -> Self {
        Self { 0: mask, 1: set }
    }

    /// Check if the given node id matches the nodemask
    #[inline(always)]
    pub const fn matches(&self, node_id: NodeId) -> bool {
        (node_id.0 & self.0) == self.1
    }
}

////////////////////////////////////////////////////////////////
//
// Data access
//

/// A `Buffer` provides a reference to a memory buffer with immutable value.
///
/// By implementing the `Deref` trait implementations of the trait support the `.` operator for dereferencing.
/// The buffers with its data lives as long as there are references to it existing in the framework.
pub trait Buffer<'s, 't, TA, T>: Debug + Deref<Target = T> + Send + 's
where
    TA: TransportAdapter<'t> + ?Sized,
    T: Debug + Send + TypeTag + Coherent + Reloc + 's,
    't: 's,
{
}

/// A `BufferMut` provides a reference to a memory buffer with mutable value.
///
/// By implementing the `DerefMut` trait implementations of the trait support the `.` operator for dereferencing.
/// The buffers with its data lives as long as there are references to it existing in the framework.
pub trait BufferMut<'s, 't, TA, T>: Debug + DerefMut<Target = T> + Send + 's
where
    TA: TransportAdapter<'t> + ?Sized,
    T: Debug + Send + TypeTag + Coherent + Reloc + 's,
    't: 's,
{
    /// Convert the buffer into an immutable buffer.
    fn into_buffer(self) -> impl Buffer<'s, 't, TA, T>;
}

/// A `BufferMaybeUninit` provides a reference to a memory buffer with a `MaybeUninit` value.
///
/// Utilizing `DerefMut` on the buffer reveals a reference to the internal `MaybeUninit<T>`.
/// The buffer can be assumed initialized with mutable access by calling `assume_init` which returns a `BufferMutRef`.
/// The buffers with its data lives as long as there are references to it existing in the framework.
pub trait BufferMaybeUninit<'s, 't, TA, T>: Debug + Send + 's
where
    TA: TransportAdapter<'t> + ?Sized,
    T: Debug + Send + TypeTag + Coherent + Reloc + 's,
    't: 's,
{
    /// Buffer type for immutable data after initialization
    type Buffer: Buffer<'s, 't, TA, T>;

    /// Buffer type for mutable data after initialization
    type BufferMut: BufferMut<'s, 't, TA, T>;

    /// Write a value into the buffer and render it initialized.
    ///
    /// This corresponds to `MaybeUninit::write`.
    fn write(self, value: T) -> Self::BufferMut;

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
    unsafe fn assume_init(self) -> Self::BufferMut;

    /// Render the buffer initialized for immutable access.
    ///
    /// This corresponds to `MaybeUninit::assume_init_read`.
    ///
    /// # Safety
    ///
    /// The caller has to make sure to initialize the data in the buffer before calling this method.
    unsafe fn assume_init_read<'a>(self) -> Self::Buffer;
}

////////////////////////////////////////////////////////////////
//
// Information elements
//

////////////////////////////////////////////////////////////////
// Event

/// The event notify trait is used by notifier implementations of events to issue notifications.
pub trait Notifier<'s, 't, TA>: Debug + Send + 's
where
    TA: TransportAdapter<'t> + ?Sized,
    't: 's,
{
    /// Notify all listeners of the event.
    fn notify(&self);
}

/// The event listen trait is used by listener implementations of events to listen for notifications.
pub trait Listener<'s, 't, TA>: Debug + Send + 's
where
    TA: TransportAdapter<'t> + ?Sized,
    't: 's,
{
    /// Check the current state of the event.
    ///
    /// Returns `true` if the event is set. Does not change the state of the event.
    fn check(&self) -> ComResult<bool>;

    /// Check the current state of the event and reset the event atomically.
    ///
    /// Returns `true` if the event was set before the reset.
    /// After this operation the event is reset.
    fn check_and_reset(&self) -> ComResult<bool>;

    /// Wait for notifications of the event.
    ///
    /// This blocks the current thread
    fn wait(&self) -> ComResult<bool>;

    /// Wait for notifications of the event with a timeout.
    ///
    /// This blocks the current thread.
    /// The boolean value indicates with `true` if the timeout has expired.
    fn wait_timeout(&self, timeout: Duration) -> ComResult<bool>;

    /// Listen asynchronously for notifications of the event.
    fn listen(&self) -> impl Future<Output = ComResult<bool>>;
}

pub trait Event<'s, 't, TA>: Debug + Clone + Send + 's
where
    TA: TransportAdapter<'t> + ?Sized,
    't: 's,
{
    /// Get a notifier for this event
    fn notifier(&self) -> ComResult<impl Notifier<'s, 't, TA>>;

    /// Get a listener for this event
    fn listener(&self) -> ComResult<impl Listener<'s, 't, TA>>;
}

/// The Builder for an `Event`
pub trait EventBuilder<'t, TA>: Debug
where
    TA: TransportAdapter<'t> + ?Sized,
{
    /// Build the event
    fn build<'a>(self) -> ComResult<impl Event<'a, 't, TA>>
    where
        't: 'a;
}

////////////////////////////////////////////////////////////////
// Topic

/// A `Publish` is a mutable data buffer used to write and publish loaned data.
///
/// Publish buffers only exist temporarily and are consumed by the publish operation.
/// They can be obtained by loan operations of publishing elements.
pub trait PublishBuffer<'s, 't, TA, T>: Debug + DerefMut<Target = T> + 's
where
    TA: TransportAdapter<'t> + ?Sized,
    T: Debug + Send + TypeTag + Coherent + Reloc + 's,
    't: 's,
{
    /// Publish the data in the buffer and consume the buffer.
    fn publish(self) -> ComResult<()>;
}

/// A publish buffer with maybe uninitialized data.
///
/// This buffer can be used to write data into the buffer and publish it.
/// The buffer can be assumed initialized with mutable access by calling `assume_init` which returns a `PublishBuffer`.
pub trait PublishBufferMaybeUninit<'s, 't, TA, T>: Debug + 's
where
    TA: TransportAdapter<'t> + ?Sized,
    T: Debug + Send + TypeTag + Coherent + Reloc + 's,
    't: 's,
{
    /// Write a value into the buffer and render it initialized.
    ///
    /// This corresponds to `MaybeUninit::write`.
    fn write(self, value: T) -> impl PublishBuffer<'s, 't, TA, T>
    where
        Self: 's;

    /// Get a mutable pointer to the internal maybe uninitialized `T`.
    ///
    /// This corresponds to `MaybeUninit::as_mut_ptr`.
    ///
    /// # Safety
    ///
    /// The caller has to make sure to initialize the data in the buffer.
    /// Reading from the received pointer before initialization is undefined behavior.
    ///
    fn as_mut_ptr(&mut self) -> *mut T;

    /// Render the buffer initialized for mutable access.
    ///
    /// This corresponds to `MaybeUninit::assume_init`.
    unsafe fn assume_init(self) -> impl PublishBuffer<'s, 't, TA, T>;
}

/// The `Publisher` represents a publisher to a topic.
///
/// The publishing application obtains instances implementing this trait through
/// topics created by a transport adapter.
pub trait Publisher<'s, 't, TA, T>: Debug + Send + 's
where
    TA: TransportAdapter<'t> + ?Sized,
    T: Debug + Send + TypeTag + Coherent + Reloc + 's,
    't: 's,
{
    /// The buffer type eligible for reuse.
    type ReuseBuffer: BufferMut<'s, 't, TA, T>;

    /// Loan an unitialized buffer for new data to be published.
    ///
    /// # Example
    ///
    /// ```rust
    /// let adapter = HeapAdapter::new();
    ///
    /// let topic = adapter.topic::<u32>().build().unwrap();
    /// let publisher = topic.publisher().unwrap();
    ///
    /// let sample = publisher.loan_uninit().unwrap();
    /// let sample = sample.write(42);
    /// sample.send();
    /// ```
    fn loan_uninit(&self) -> ComResult<impl PublishBufferMaybeUninit<'s, 't, TA, T> + 's>
    where
        T: 's,
        't: 's;

    /// Loan a buffer with initialized data to be published.
    ///
    /// The implementation copies the given value into the buffer.
    ///
    /// # Example
    ///
    /// ```rust
    /// let adapter = HeapAdapter::new();
    ///
    /// let topic = adapter.topic::<u32>().build().unwrap();
    /// let publisher = topic.publisher().unwrap();
    ///
    /// let sample = publisher.loan(42).unwrap();
    /// sample.send();
    /// ```
    fn loan_with(&self, value: T) -> ComResult<impl PublishBuffer<'s, 't, TA, T>>
    where
        T: 's,
        't: 's;

    /// Reuse a buffer for new data to be published.
    ///
    /// The buffer provided as argument is converted into a publish buffer for reuse.
    /// The buffer is consumed by the operation. No memory allocation is performed.
    ///
    fn reuse(&self, buffer: Self::ReuseBuffer) -> ComResult<impl PublishBuffer<'s, 't, TA, T>>;

    /// Public new data as copy to the topic.
    ///
    /// This requires a copy of the data for publication.
    #[inline(always)]
    fn publish(&self, data: T) -> ComResult<()> {
        self.loan_with(data)?.publish()
    }
}

/// The `Subscriber` trait represents the receiving end of published data on a topic.
pub trait Subscriber<'s, 't, TA, T>: Debug + Send + 's
where
    TA: TransportAdapter<'t> + ?Sized,
    T: Debug + Send + TypeTag + Coherent + Reloc + 's,
    't: 's,
{
    // type Buffer: BufferMut<TA, T>;

    /// Check for new data and consume it if present.
    fn try_receive(&self) -> ComResult<Option<impl BufferMut<'s, 't, TA, T>>>;

    /// Wait for new data to arrive.
    ///
    /// Upon success the method returns a receive buffer containing the new data.
    fn receive(&self) -> ComResult<impl BufferMut<'s, 't, TA, T>>;

    /// Wait for new data to arrive with a timeout.
    ///
    /// The `ComResult` of the tuple contains the upon success a receive buffer containing the new data.
    fn receive_timeout(&self, timeout: Duration) -> ComResult<impl BufferMut<'s, 't, TA, T>>;

    /// Receive new data asynchronously.
    ///
    /// This method returns a future that can be used to `await` the arrival of new data.
    fn receive_async(&self) -> impl Future<Output = ComResult<impl BufferMut<'s, 't, TA, T>>>;
}

/// The `Topic` trait references to the implementation of a topic in the underlying transport framework.
///
/// A topic can only be created through the `topic` method of the corresponding transport adapter.
/// The publishers and subscribers can be obtained through the topics `publisher` and `subscriber` methods, respectively.
///
pub trait Topic<'s, 't, TA, T>: Debug + Clone + Send + 's
where
    TA: TransportAdapter<'t> + ?Sized,
    T: Debug + Send + TypeTag + Coherent + Reloc + 's,
    't: 's,
{
    /// Get a publisher for this topic
    ///
    /// The method succees if the topic still accepts new publishers.
    /// The fan-in of a topic is limited by the configuration given to the builder when creating the topic.
    fn publisher(&self) -> ComResult<impl Publisher<'s, 't, TA, T>>;

    /// Get a subscriber for this topic
    ///
    /// The method succeeds if the topic still accepts new subscribers.
    /// The fan-out of a topic is limited by the configuration given to the builder when creating the topic.    
    fn subscriber(&self) -> ComResult<impl Subscriber<'s, 't, TA, T>>;
}

/// The Builder for a `Topic`
pub trait TopicBuilder<'t, TA, T>: Debug
where
    TA: TransportAdapter<'t> + ?Sized,
    T: TypeTag + Coherent + Reloc + Send + Debug,
{
    /// Set the queue depth of the topic.
    fn with_queue_depth(self, queue_depth: usize) -> Self;

    /// Set the queue policy of the topic.
    fn with_queue_policy(self, queue_policy: QueuePolicy) -> Self;

    /// Set the fan-in of the topic which limits the number of publishers.
    fn with_max_fan_in(self, fan_in: usize) -> Self;

    /// Set the fan-out of the topic which limits the number of subscribers.
    fn with_max_fan_out(self, fan_out: usize) -> Self;

    /// Build the topic
    fn build<'a>(self) -> ComResult<impl Topic<'a, 't, TA, T>>
    where
        T: 'a,
        't: 'a;
}

////////////////////////////////////////////////////////////////
// Remote Procedure Call

/// Parameter packs are tuples with all elements implementing the `TypeTag + Coherent + Reloc` traits.
pub trait ParameterPack: Debug + Send + TypeTag + Coherent + Reloc + Tuple {}

/// A return value is a marker that combines the TypeTag, Coherent and Reloc traits.
pub trait Return: Debug + Send + TypeTag + Coherent + Reloc {}

/// The `RequestMessage` is a tuple with a tag, a transaction counter and the arguments of a remote procedure call.
///
/// The tag is a unique tag that identifies the client that sends the request.
/// The transaction counter is a monotonous counter that identifies the individual transaction of that client.
/// The `Args` tuple contains the arguments of the remote procedure call.
pub type RequestMessage<Args>
where
    Args: ParameterPack,
= (Tag, usize, Args);

/// The `ResponseMessage` is a tuple with a transaction counter and the result of a remote procedure call.
///
/// The transaction counter is the counter of the request this response is related to.
/// The `R` type contains the result of the remote procedure call.
pub type ResponseMessage<R>
where
    R: Return,
= (usize, R);

/// The Invoker trait is implemented by the client side of remote procedures.
pub trait Invoker<'s, 't, TA, Args, R>: Debug + 's
where
    TA: TransportAdapter<'t> + ?Sized,
    Args: ParameterPack + 's,
    R: Return + 's,
    't: 's,
{
    /// Announce the remote procedure invoker to the remote side.
    ///
    /// This is a one-time operation that announces the remote procedure to the remote side.
    /// Announcing the remote procedure invoker will allow the remote side to prepare for incoming invocations.
    /// The announcement is voluntary but will help to avoid time wasted on the initial call. Underlying implementations
    /// may choose to use the accouncement to open up the necessary communication channels.
    #[inline(always)]
    fn announce(&self) -> ComResult<()> {
        Ok(())
    }

    /// Revoke the announcement of the remote procedure invoker from the remote side.
    ///
    /// This is a one-time operation that revokes the announcement of the remote procedure from the remote side.
    /// Eventually existing channels to the remote side will be closed and corresponding resources will be released.
    #[inline(always)]
    fn revoke(&self) -> ComResult<()> {
        Ok(())
    }

    /// Prepare an invocation with arguments.
    ///
    /// This loans a buffer with given arguments and also prepares a subscriber for the result.
    /// Publishing the returned `Publish` element will trigger the Rpc invocation.
    ///
    /// The buffer to publish the arguments has three parts:
    ///
    /// 1. A tag of the client that sends the request
    /// 2. A monotonous transaction counter of the invocation with regard to the client
    /// 3. The arguments
    ///
    /// The result returns a subscriber for two values:
    ///
    /// 1. The transaction counter of the request this response is related to
    /// 2. The result as defined in the remote procedure
    ///
    /// The result subscriber can be used to wait for the result of the rpc invocation.
    fn prepare(
        &'s self,
        args: Args,
    ) -> ComResult<(
        impl PublishBuffer<'s, 't, TA, RequestMessage<Args>>,
        impl Subscriber<'s, 't, TA, ResponseMessage<R>>,
    )>;

    /// Prepare an invocation with uninizialized arguments.
    ///
    /// This loans a buffer for uninitialized arguments and also prepares a subscriber for the result.
    /// Writing arguments into the buffer and publishing the returned InvokeArgsBuffer will trigger the Rpc invocation.
    ///
    /// The result subscriber can be used to wait for the result of the rpc invocation.
    fn prepare_uninit(
        &'s self,
    ) -> ComResult<(
        impl PublishBufferMaybeUninit<'s, 't, TA, RequestMessage<Args>>,
        impl Subscriber<'s, 't, TA, ResponseMessage<R>>,
    )>;

    /// Synchronously invoke the Rpc with the given arguments as blocking operation.
    ///
    /// The operation will copy the arguments into the buffer used for the invocation.
    ///
    /// This operation will block until the result of the Rpc invocation is available.
    /// The result of a completed Rpc operation will always wrap into a `ComResult` as the communication itself may fail.
    fn invoke(&'s self, args: Args) -> ComResult<impl BufferMut<'s, 't, TA, ResponseMessage<R>>>;

    /// Invoke the Rpc with the given arguments as asynchronous operation.
    ///
    /// The operation will copy the arguments into the buffer used for the invocation.
    ///
    /// This operation does not block. Instead, it will return a future that, when awaited, will provide the result of the rpc invocation.
    /// The result of a completed Rpc operation will always wrap into a `ComResult` as the communication itself may fail.
    fn invoke_async(
        &'s self,
        args: Args,
    ) -> impl Future<Output = ComResult<impl BufferMut<'s, 't, TA, ResponseMessage<R>>>>;
}

/// The `Invokee` trait represents the service side of a remote procedure.
pub trait Invokee<'s, 't, TA, Args, R>: Debug + 's
where
    TA: TransportAdapter<'t> + ?Sized,
    Args: ParameterPack + 's,
    R: Return + 's,
    't: 's,
{
    /// test if an incoming request is present and return the corresponding buffers.
    ///
    /// The method returns a tuple with the received arguments buffer and the result publish buffer.
    /// When the request is successfully fetched it is expected to be processed by the caller.
    /// The returned result buffer shall be initialized with the result of the call.
    /// The execution completion is signaled by publishing the result buffer.
    ///
    /// When the incoming queue is empty, the method returns `None`.
    fn try_accept(
        &'s self,
    ) -> ComResult<
        Option<(
            impl Buffer<'s, 't, TA, RequestMessage<Args>>,
            impl PublishBufferMaybeUninit<'s, 't, TA, ResponseMessage<R>>,
        )>,
    >;

    /// wait for incoming calls blocking the current thread.
    fn accept(
        &'s self,
    ) -> ComResult<(
        impl Buffer<'s, 't, TA, RequestMessage<Args>>,
        impl PublishBufferMaybeUninit<'s, 't, TA, ResponseMessage<R>>,
    )>;

    /// wait for incoming calls with a timeout blocking the current thread.
    fn accept_timeout(
        &'s self,
        timeout: Duration,
    ) -> ComResult<(
        impl Buffer<'s, 't, TA, RequestMessage<Args>>,
        impl PublishBufferMaybeUninit<'s, 't, TA, ResponseMessage<R>>,
    )>;

    /// wait for incoming invocations and return the result of the call of the given function with the received arguments.
    fn accept_and_call<F: Fn(&Args) -> R>(&'s self, f: F) -> ComResult<()> {
        // wait for incoming invocations
        let (req, res) = self.accept()?;
        let (_client_tag, transaction, args) = &*req;

        // call the function with the received arguments
        let result = f(&args);

        // build and send the response
        let res = res.write((*transaction, result));
        res.publish()?;
        Ok(())
    }

    /// wait for incoming invocations with a timeout and return the result of the call of the given function with the received arguments.
    fn accept_timeout_and_call<F: Fn(&Args) -> R>(
        &'s self,
        timeout: Duration,
        f: F,
    ) -> ComResult<()> {
        let (req, res) = self.accept_timeout(timeout)?;
        let (_client_tag, transaction, args) = &*req;

        // call the function with the received arguments
        let result = f(&args);

        // build and send the response
        let res = res.write((*transaction, result));
        res.publish()?;
        Ok(())
    }

    /// accept creates a future that waits for incoming calls on the remote procedure port and
    /// returns the arguments and a result publish buffer.
    fn accept_async(
        &'s self,
    ) -> impl Future<
        Output = ComResult<(
            impl Buffer<'s, 't, TA, RequestMessage<Args>>,
            impl PublishBufferMaybeUninit<'s, 't, TA, ResponseMessage<R>>,
        )>,
    > + Send;

    /// create a future that waits for incoming calls on the remote procedure port and
    /// returns the result of the invocation of the given function with the received arguments.
    fn accept_async_and_call<F: Fn(&Args) -> R>(
        &'s self,
        f: F,
    ) -> impl Future<Output = ComResult<()>> {
        async move {
            let (req, res) = self.accept_async().await?;
            let (_client_tag, transaction, args) = &*req;
            let result = f(args);
            let res = res.write((*transaction, result));
            res.publish()?;
            Ok(())
        }
    }

    /// create a future that waits for incoming calls on the remote procedure port and
    /// returns the result of the invocation of the given async function with the received arguments.
    fn accept_async_and_call_async<
        F: Fn(&Args) -> Fut + Send,
        Fut: Future<Output = R> + Send + 's,
    >(
        &'s self,
        f: F,
    ) -> impl Future<Output = ComResult<()>> {
        async move {
            let (req, res) = self.accept_async().await?;
            let (_client_tag, transaction, args) = &*req;
            let result = f(args).await;
            let res = res.write((*transaction, result));
            res.publish()?;
            Ok(())
        }
    }
}

/// The `RemoteProcedure` trait represents a remote procedure call.
pub trait RemoteProcedure<'s, 't, TA, Args, R>: Debug + Clone + Send + 's
where
    TA: TransportAdapter<'t> + ?Sized,
    Args: ParameterPack + 's,
    R: Return + 's,
    't: 's,
{
    /// Get a client-side invoker for this remote procedure
    fn invoker(&self) -> ComResult<impl Invoker<'s, 't, TA, Args, R>>;

    /// Get a service-side invokee for this remote procedure
    fn invokee(&self) -> ComResult<impl Invokee<'s, 't, TA, Args, R>>;
}

/// The Builder for an `RemoteProcedure`
pub trait RemoteProcedureBuilder<'t, TA, Args, R>: Debug
where
    TA: TransportAdapter<'t> + ?Sized,
    Args: ParameterPack,
    R: Return,
{
    /// Set the queue depth of the topic.
    fn with_queue_depth(self, queue_depth: usize) -> Self;

    /// Set the queue policy of the topic.
    fn with_queue_policy(self, queue_policy: QueuePolicy) -> Self;

    /// Set the max fan-in of the topic which limits the number of publishers.
    fn with_max_clients(self, fan_in: usize) -> Self;

    /// build the remote procedure
    fn build<'a>(self) -> ComResult<impl RemoteProcedure<'a, 't, TA, Args, R>>
    where
        Args: 'a,
        R: 'a,
        't: 'a;
}

////////////////////////////////////////////////////////////////
//
// The transport adapter
//

/// An adapter is an abstraction that connects to an underlying framework implementation
//TODO: Move to qor-core
pub trait Adapter: Debug {
    type StaticConfig: StaticConfig<Self>;

    /// Get the static configuration of the adapter.
    fn static_config(&self) -> &'static Self::StaticConfig;
}

/// A transport adapter connects to a memory provision framework.
///
/// The provision framework must be able to provide memory buffers for raw data access
/// as well a providing the fundamental communication elements like topics, events and rpcs.
///
/// All elements connected with an adapter carry the type of the adapter as a type parameter.
/// This prevents at compile time that elements from different adapter types can be mixed up.
///
pub trait TransportAdapter<'t>: Adapter {
    /// Get an event builder.
    ///
    /// Events signal the occurrence of a state change.
    /// They issue notifiers to emit the signal and listeners to wait for the signal.
    fn event<'a>(&'t self, label: Label) -> impl EventBuilder<'t, Self>
    where
        't: 'a;

    /// Get a topic builder for the given type.
    ///
    /// Topics are information elements transporting values of a given type.
    /// They issue publishers that publish new values and subscribers that receive the values.
    fn topic<'a, T>(&'t self, label: Label) -> impl TopicBuilder<'t, Self, T>
    where
        T: TypeTag + Coherent + Reloc + Send + Debug + 'a,
        't: 'a;

    /// Get a remote procedure builder for the given argument and result types.
    fn remote_procedure<'a, Args, R>(
        &self,
        label: Label,
    ) -> impl RemoteProcedureBuilder<'t, Self, Args, R>
    where
        Args: ParameterPack + 'a,
        R: Return + 'a,
        't: 'a;
}
