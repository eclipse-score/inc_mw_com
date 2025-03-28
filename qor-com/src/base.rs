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
//! - signal
//! - Event
//! - Remote Procedure
//!
//! Each of the elements can produce a sending and a receiving part.
//!
//! - signal: Notifier and Listener
//! - Event: Publisher and Subscriber
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

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub enum ComError {
    LockError,
    Timeout,
    QueueEmpty,
    QueueFull,
    FanError,
}

/// Communication module result type
pub type ComResult<T> = std::result::Result<T, ComError>;

////////////////////////////////////////////////////////////////
//
// Data elements
//
#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub enum QueuePolicy {
    OverwriteOldest,
    OverwriteNewest,
    ErrorOnFull,
}


////////////////////////////////////////////////////////////////
//
// Information elements
//

////////////////////////////////////////////////////////////////
// Signal

/// The signal notify trait is used by notifier implementations of signals to issue notifications.
pub trait Notifier<A>: Debug + Send
where
    A: TransportAdapter + ?Sized,
{
    /// Notify all listeners of the signal.
    fn notify(&self);
}

/// The signal listen trait is used by listener implementations of signals to listen for notifications.
pub trait Listener<A>: Debug + Send
where
    A: TransportAdapter + ?Sized,
{
    /// Check the current state of the signal.
    ///
    /// Returns `true` if the signal is set. Does not change the state of the signal.
    fn check(&self) -> ComResult<bool>;

    /// Check the current state of the signal and reset the signal atomically.
    ///
    /// Returns `true` if the signal was set before the reset.
    /// After this operation the signal is reset.
    fn check_and_reset(&self) -> ComResult<bool>;

    /// Wait for notifications of the signal.
    ///
    /// This blocks the current thread
    fn wait(&self) -> ComResult<bool>;

    /// Wait for notifications of the signal with a timeout.
    ///
    /// This blocks the current thread.
    /// The boolean value indicates with `true` if the timeout has expired.
    fn wait_timeout(&self, timeout: Duration) -> ComResult<bool>;

    /// Wait asynchronously for notifications of the signal.
    fn wait_async(&self) -> impl Future<Output = ComResult<bool>>;

    /// Wait asynchronously for notification of the signal with timeout.
    fn wait_timeout_async(&self, timeout: Duration) -> impl Future<Output = ComResult<bool>>;
}

/// The `Signal` trait represents a reference to a signal in the communication system
/// that can be used to notify listeners of a state change.
pub trait Signal<A>: Debug + Clone + Send
where
    A: TransportAdapter + ?Sized,
{
    /// Associated Notifier type of the signal type
    type Notifier: Notifier<A>;

    /// Associated Listener type of the signal type
    type Listener: Listener<A>;

    /// Get a notifier for this signal
    fn notifier(&self) -> ComResult<Self::Notifier>;

    /// Get a listener for this signal
    fn listener(&self) -> ComResult<Self::Listener>;
}

/// The Builder for a `Signal`
pub trait SignalBuilder<A>: Debug
where
    A: TransportAdapter + ?Sized,
{
    /// The type of the created signal
    type Signal: Signal<A>;

    /// Build the signal
    fn build(self) -> ComResult<Self::Signal>;
}

////////////////////////////////////////////////////////////////
// Event

/// A `Sample` provides a reference to a memory buffer with immutable value.
///
/// By implementing the `Deref` trait implementations of the trait support the `.` operator for dereferencing.
/// The buffers with its data lives as long as there are references to it existing in the framework.
pub trait Sample<A, T>: Debug + Deref<Target = T> + Send
where
    A: TransportAdapter + ?Sized,
    T: Debug + Send + TypeTag + Coherent + Reloc,
{
}

/// A `SampleMut` provides a reference to a memory buffer with mutable value.
///
/// By implementing the `DerefMut` trait implementations of the trait support the `.` operator for dereferencing.
/// The buffers with its data lives as long as there are references to it existing in the framework.
pub trait SampleMut<A, T>: Debug + DerefMut<Target = T> + Send
where
    A: TransportAdapter + ?Sized,
    T: Debug + Send + TypeTag + Coherent + Reloc,
{
    /// The associated read-only sample type.
    type Sample: Sample<A, T>;

    /// Convert the sample into an immutable sample.
    fn into_sample(self) -> Self::Sample;

    /// Publish the sample and consume it.
    fn publish(self) -> ComResult<()>;
}

/// A `SampleMaybeUninit` provides a reference to a memory buffer with a `MaybeUninit` value.
///
/// Utilizing `DerefMut` on the buffer reveals a reference to the internal `MaybeUninit<T>`.
/// The buffer can be assumed initialized with mutable access by calling `assume_init` which returns a `SampleMut`.
/// The buffers with its data lives as long as there are references to it existing in the framework.
pub trait SampleMaybeUninit<A, T>: Debug + Send
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

    /// Render the buffer initialized for immutable access.
    ///
    /// This corresponds to `MaybeUninit::assume_init_read`.
    ///
    /// # Safety
    ///
    /// The caller has to make sure to initialize the data in the buffer before calling this method.
    unsafe fn assume_init_read(
        self,
    ) -> <<Self as SampleMaybeUninit<A, T>>::SampleMut as SampleMut<A, T>>::Sample;
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
    /// let sample = publisher.loan_uninit().unwrap();
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
    fn loan_with(
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
    /// This requires a copy of the data for publication.
    #[inline(always)]
    fn publish(&self, data: T) -> ComResult<()> {
        self.loan_with(data)?.publish()
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
    fn receive_async(&self) -> impl Future<Output = ComResult<Self::Sample>> + Send;

    /// Receive new data asynchronously with timeout.
    ///
    /// This method returns a future that can be used to `await` the arrival of new data.
    fn receive_timeout_async(
        &self,
        timeout: Duration,
    ) -> impl Future<Output = ComResult<Self::Sample>> + Send;
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

////////////////////////////////////////////////////////////////
// Remote Procedure Call

/// Parameter packs are tuples with all elements implementing the `TypeTag + Coherent + Reloc` traits.
pub trait ParameterPack: Debug + Send + TypeTag + Coherent + Reloc + Tuple {}

/// A return value is a marker that combines the TypeTag, Coherent and Reloc traits.
pub trait ReturnValue: Debug + Send + TypeTag + Coherent + Reloc {}


/// The `Request` is a read-only invocation request used on service side for incoming invocations
pub trait Request<A, Args>:
    Debug + Send + Deref<Target = Args>
where
    A: TransportAdapter + ?Sized,
    Args: ParameterPack,
{
    type ResponseMaybeUninit<R>: ResponseMaybeUninit<A, R> where R: ReturnValue;

    fn prepare_response_uninit<R>(&self) -> ComResult<Self::ResponseMaybeUninit<R>> where R: ReturnValue;
    fn prepare_response<R>(&self) -> ComResult<<<Self as Request<A, Args>>::ResponseMaybeUninit<R> as ResponseMaybeUninit<A, R>>::ResponseMut> where R: ReturnValue;
}

/// A `RequestMut` is a mutable invocation request used on client side for invocations
pub trait RequestMut<A, Args>:
    Debug + Send + From<Args> + DerefMut<Target = Args>
where
    A: TransportAdapter + ?Sized,
    Args: ParameterPack,
{
    fn send(&self) -> ComResult<()>;
}

/// A `RequestMaybeUninit` is an uninitialized invocation request used on client side for invocations
pub trait RequestMaybeUninit<A, Args>:
    Debug + Send + TypeTag + Coherent + Reloc
where
    A: TransportAdapter + ?Sized,
    Args: ParameterPack,
{
    type RequestMut: RequestMut<A, Args>;

    fn write(self, args: Args) -> Self::RequestMut;
    fn as_mut_ptr(&mut self) -> *mut Args;
    unsafe fn assume_init(self) -> Self::RequestMut;
}


/// The `Response` is an immutable response to a remote procedure call used on client side for receiving results.
pub trait Response<A, R>: Debug + Send + Deref<Target = R> 
where
    A: TransportAdapter + ?Sized,
    R: ReturnValue,
{
}

/// The `ResponseMut` is a mutable response of a remote procedure call used on service side for sending results.
pub trait ResponseMut<A, R>: Debug + Send + DerefMut<Target = R> 
where
    A: TransportAdapter + ?Sized,
    R: ReturnValue,
{
    fn send(&self) -> ComResult<()>;
}

/// The `ResponseMaybeUninit` is an uninitialized response of a remote procedure call used on service side for sending results.
pub trait ResponseMaybeUninit<A, R>: Debug + Send
where
    A: TransportAdapter + ?Sized,
    R: ReturnValue,
{
    type ResponseMut: ResponseMut<A, R>;

    fn write(self, value: R) -> Self::ResponseMut;
    fn as_mut_ptr(&mut self) -> *mut R;
    unsafe fn assume_init(self) -> Self::ResponseMut;
}


/// The Invoker trait is implemented by the client side of remote procedures.
pub trait Invoker<A, Args, R>: Debug
where
    A: TransportAdapter + ?Sized,
    Args: ParameterPack,
    R: ReturnValue,
{
    type Request: RequestMaybeUninit<A, Args>;
    type Response: Response<A, R>;

    type RequestSampleMaybeUninit: SampleMaybeUninit<A, Self::Request>;
    type ResponseSubscriber: Subscriber<A, Self::Response>;

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
    /// signalually existing channels to the remote side will be closed and corresponding resources will be released.
    #[inline(always)]
    fn revoke(&self) -> ComResult<()> {
        Ok(())
    }

    /// Prepare an invocation with uninizialized arguments.
    ///
    /// This loans a buffer for uninitialized arguments and also prepares a subscriber for the result.
    /// Writing arguments into the buffer and publishing the returned InvokeArgsBuffer will trigger the Method invocation.
    ///
    /// The result subscriber can be used to wait for the result of the rpc invocation.
    fn prepare_uninit(
        &self,
    ) -> ComResult<(Self::RequestSampleMaybeUninit, Self::ResponseSubscriber)>;

    /// Prepare an invocation with arguments.
    ///
    /// This loans a buffer with given arguments and also prepares a subscriber for the result.
    /// Publishing the returned `Publish` element will trigger the Method invocation.
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
        &self,
        args: Args,
    ) -> ComResult<(
        <<Self as Invoker<A, Args, R>>::RequestSampleMaybeUninit as SampleMaybeUninit<
            A,
            Self::Request,
        >>::SampleMut,
        Self::ResponseSubscriber,
    )> {
        let (args_event, res) = self.prepare_uninit()?;
        let args_event = args_event.write(Self::Request::from(args));
        Ok((args_event, res))
    }

    /// Synchronously invoke the Method with the given arguments as blocking operation.
    ///
    /// The operation will copy the arguments into the buffer used for the invocation.
    ///
    /// This operation will block until the result of the Method invocation is available.
    /// The result of a completed Method operation will always wrap into a `ComResult` as the communication itself may fail.
    fn invoke(
        &self,
        args: Args,
    ) -> ComResult<<Self::ResponseSubscriber as Subscriber<A, Self::Response>>::Sample> {
        let (args, res) = self.prepare(args)?;
        args.publish()?;
        res.receive()
    }

    /// Synchronously invoke the Method with the given arguments as blocking operation with timeout.
    ///
    /// The operation will copy the arguments into the buffer used for the invocation.
    ///
    /// This operation will block until the result of the Method invocation is available or the timeout occurs.
    /// The result of a completed Method operation will always wrap into a `ComResult` as the communication itself may fail.
    fn invoke_timeout(
        &self,
        args: Args,
        timeout: Duration,
    ) -> ComResult<<Self::ResponseSubscriber as Subscriber<A, Self::Response>>::Sample> {
        let (args, res) = self.prepare(args)?;
        args.publish()?;
        res.receive_timeout(timeout)
    }

    /// Invoke the Method with the given arguments as asynchronous operation.
    ///
    /// The operation will copy the arguments into the buffer used for the invocation.
    ///
    /// This operation does not block. Instead, it will return a future that, when awaited, will provide the result of the rpc invocation.
    /// The result of a completed Method operation will always wrap into a `ComResult` as the communication itself may fail.
    fn invoke_async(
        &self,
        args: Args,
    ) -> impl Future<
        Output = ComResult<<Self::ResponseSubscriber as Subscriber<A, Self::Response>>::Sample>,
    > + Send;

    /// Invoke the Method with the given arguments as asynchronous operation.
    ///
    /// The operation will copy the arguments into the buffer used for the invocation.
    ///
    /// This operation does not block. Instead, it will return a future that, when awaited, will provide the result of the rpc invocation.
    /// The result of a completed Method operation will always wrap into a `ComResult` as the communication itself may fail.
    fn invoke_timeout_async(
        &self,
        args: Args,
        timeout: Duration,
    ) -> impl Future<
        Output = ComResult<<Self::ResponseSubscriber as Subscriber<A, Self::Response>>::Sample>,
    > + Send;
}

/// The `Invokee` trait represents the service side of a remote procedure.
pub trait Invokee<A, Args, R>: Debug
where
    A: TransportAdapter + ?Sized,
    Args: ParameterPack,
    R: ReturnValue,
{
    type Request: ReceiveRequest<A, Args>;
    type Response: SendResponse<A, R>;

    type RequestSample: Sample<A, Self::Request>;
    type ResponseSampleMaybeUninit: SampleMaybeUninit<A, Self::Response>;

    /// test if an incoming request is present and return the corresponding buffers.
    ///
    /// The method returns a tuple with the received arguments buffer and the result publish buffer.
    /// When the request is successfully fetched it is expected to be processed by the caller.
    /// The returned result buffer shall be initialized with the result of the call.
    /// The execution completion is signaled by publishing the result buffer.
    ///
    /// When the incoming queue is empty, the method returns `None`.
    fn try_accept(&self) -> ComResult<Option<(Self::RequestSample, Self::ResponseSampleMaybeUninit)>>;

    /// wait for incoming calls blocking the current thread.
    fn accept(&self) -> ComResult<(Self::RequestSample, Self::ResponseSampleMaybeUninit)>;

    /// wait for incoming calls with a timeout blocking the current thread.
    fn accept_timeout(
        &self,
        timeout: Duration,
    ) -> ComResult<(Self::RequestSample, Self::ResponseSampleMaybeUninit)>;

    /// wait for incoming invocations and return the result of the execution of the given function with the received arguments.
    fn accept_and_execute<F>(&self, f: F) -> ComResult<()>
    where
        F: Fn(&Args) -> R,
    {
        // wait for incoming invocations
        let (req, res) = self.accept()?;

        // call the function with the received arguments
        let result = f(req.deref());

        // build and send the response
        let res = 
        let res = res.write((*transaction, result));
        res.publish()?;
        Ok(())
    }

    /// wait for incoming invocations with timeout and return the result of the execution of the given function with the received arguments.
    fn accept_timeout_and_execute<F>(&self, f: F, timeout: Duration) -> ComResult<()>
    where
        F: Fn(&Args) -> R,
    {
        // wait for incoming invocations
        let (req, res) = self.accept_timeout(timeout)?;
        let (_client_tag, transaction, args) = &*req;

        // call the function with the received arguments
        let result = f(&args);

        // build and send the response
        let res = res.write((*transaction, result));
        res.publish()?;
        Ok(())
    }

    /// wait for incoming invocations with a timeout and return the result of the call of the given function with the received arguments.
    fn accept_timeout_and_call<F>(&self, timeout: Duration, f: F) -> ComResult<()>
    where
        F: Fn(&Args) -> R,
    {
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
        &self,
    ) -> impl Future<Output = ComResult<(Self::RequestSample, Self::ResponseSampleMaybeUninit)>> + Send;

    /// accept creates a future that waits with a timeout for incoming calls on the remote procedure port and
    /// returns the arguments and a result publish buffer.
    fn accept_timeout_async(
        &self,
        timeout: Duration,
    ) -> impl Future<Output = ComResult<(Self::RequestSample, Self::ResponseSampleMaybeUninit)>> + Send;

    /// create a future that waits for incoming calls on the remote procedure port and
    /// returns the result of the invocation of the given async function with the received arguments.
    fn accept_and_execute_async<F, Fut>(&self, f: F) -> impl Future<Output = ComResult<()>>
    where
        F: Fn(&Args) -> Fut + Send,
        Fut: Future<Output = R> + Send,
    {
        async move {
            let (req, res) = self.accept_async().await?;
            let (_client_tag, transaction, args) = &*req;
            let result = f(args).await;
            let res = res.write((*transaction, result));
            res.publish()?;
            Ok(())
        }
    }

    /// create a future that waits for incoming calls on the remote procedure port and
    /// returns the result of the invocation of the given async function with the received arguments.
    fn accept_timeout_and_execute_async<F, Fut>(&self, f: F) -> impl Future<Output = ComResult<()>>
    where
        F: Fn(&Args) -> Fut + Send,
        Fut: Future<Output = R> + Send,
    {
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
pub trait Method<A, Args, R>: Debug + Clone + Send
where
    A: TransportAdapter + ?Sized,
    Args: ParameterPack,
    R: ReturnValue,
{
    type Invoker: Invoker<A, Args, R>;
    type Invokee: Invokee<A, Args, R>;

    /// Get a client-side invoker for this remote procedure
    fn invoker(&self) -> ComResult<Self::Invoker>;

    /// Get a service-side invokee for this remote procedure
    fn invokee(&self) -> ComResult<Self::Invokee>;
}

/// The Builder for an `RemoteProcedure`
pub trait MethodBuilder<A, Args, R>: Debug
where
    A: TransportAdapter + ?Sized,
    Args: ParameterPack,
    R: ReturnValue,
{
    type Method: Method<A, Args, R>;

    /// Set the queue depth of the event.
    fn with_queue_depth(self, queue_depth: usize) -> Self;

    /// Set the queue policy of the event.
    fn with_queue_policy(self, queue_policy: QueuePolicy) -> Self;

    /// Set the max fan-in of the event which limits the number of publishers.
    fn with_max_clients(self, fan_in: usize) -> Self;

    /// build the remote procedure
    fn build(self) -> ComResult<Self::Method>;
}

////////////////////////////////////////////////////////////////
//
// The transport adapter
//

pub trait StaticConfig<A>: Debug
where
    A: Adapter + ?Sized,
{
    /// Get the class name of the adapter.
    fn name() -> &'static str;

    /// Get the vendor of the adapter
    fn vendor() -> &'static str;

    /// Get the version of the adapter.
    fn version() -> Version;
}

/// An adapter is an abstraction that connects to an underlying framework implementation
//TODO: Move to qor-core
pub trait Adapter: Debug {
    type StaticConfig: StaticConfig<Self>;

    /// Get the static configuration of the adapter.
    fn static_config<'a>(&self) -> &'a Self::StaticConfig;
}

/// A transport adapter connects to a memory provision framework.
///
/// The provision framework must be able to provide memory buffers for raw data access
/// as well a providing the fundamental communication elements like events, signals and rpcs.
///
/// All elements connected with an adapter carry the type of the adapter as a type parameter.
/// This prsignals at compile time that elements from different adapter types can be mixed up.
///
pub trait TransportAdapter: Adapter {
    type SignalBuilder: SignalBuilder<Self>;
    type EventBuilder<T>: EventBuilder<Self, T>
    where
        T: TypeTag + Coherent + Reloc + Send + Debug;
    type MethodBuilder<Args, R>: MethodBuilder<Self, Args, R>
    where
        Args: ParameterPack,
        R: ReturnValue;

    /// Get an signal builder.
    ///
    /// signals signal the occurrence of a state change.
    /// They issue notifiers to emit the signal and listeners to wait for the signal.
    fn signal(&self, label: Label) -> Self::SignalBuilder;

    /// Get a event builder for the given type.
    ///
    /// Events are information elements transporting values of a given type.
    /// They issue publishers that publish new values and subscribers that receive the values.
    fn event<T>(&self, label: Label) -> Self::EventBuilder<T>
    where
        T: TypeTag + Coherent + Reloc + Send + Debug;

    /// Get a remote procedure builder for the given argument and result types.
    fn method<Args, R>(&self, label: Label) -> Self::MethodBuilder<Args, R>
    where
        Args: ParameterPack,
        R: ReturnValue;
}
