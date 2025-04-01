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
// Remote Procedure Calls: Request-Response Messaging Pattern

/// A `RequestMaybeUninit` is an uninitialized invocation request used on client side for invocations
pub trait RequestMaybeUninit<A, Args, R>: Debug
where
    A: TransportAdapter + ?Sized,
    Args: ParameterPack,
    R: ReturnValue,
{
    type RequestMut: RequestMut<A, Args, R>;

    /// Write the arguments into the buffer and render it initialized.
    fn write(self, args: Args) -> Self::RequestMut;

    /// Get a mutable pointer to the internal maybe uninitialized `Args`.
    fn as_mut_ptr(&mut self) -> *mut Args;

    /// Render the buffer initialized for mutable access.
    unsafe fn assume_init(self) -> Self::RequestMut;
}

/// A `RequestMut` is a mutable invocation request used on client side for invocations
pub trait RequestMut<A, Args, R>: Debug + DerefMut<Target = Args>
where
    A: TransportAdapter + ?Sized,
    Args: ParameterPack,
    R: ReturnValue,
{
    type PendingRequest: PendingRequest<A, Args, R>;

    /// Execute the request by sending it to the service.
    ///
    /// This consumes the request as it can be executed only once.
    fn execute(self) -> ComResult<Self::PendingRequest>;
}

/// An PendingRequest is a pending invocation waiting for a response.
///
/// This works similar to a future.
///
/// SAFETY: Once a response is obtained consecutive calls
/// to the receive functions is considered undefined behavior.
pub trait PendingRequest<A, Args, R>: Debug + Send
where
    A: TransportAdapter + ?Sized,
    Args: ParameterPack,
    R: ReturnValue,
{
    type Response: Response<A, Args, R>;

    /// Check if the response is ready.
    fn try_receive(&self) -> ComResult<Option<Self::Response>>;

    /// Wait for the response blocking the current thread.
    fn receive(&self) -> ComResult<Self::Response>;

    /// Wait for the response with a timeout blocking the current thread.
    fn receive_timeout(&self, timeout: Duration) -> ComResult<Self::Response>;

    /// Wait for the response asynchronously.
    fn receive_async(&self) -> impl Future<Output = ComResult<Self::Response>>;

    /// Wait for the response asynchronously with a timeout.
    fn receive_timeout_async(
        &self,
        timeout: Duration,
    ) -> impl Future<Output = ComResult<Self::Response>>;
}

/// The `Request` is a read-only invocation request used on service side for incoming invocations
pub trait Request<A, Args, R>: Debug + Send + Deref<Target = Args>
where
    A: TransportAdapter + ?Sized,
    Args: ParameterPack,
    R: ReturnValue,
{
    type ResponseMaybeUninit: ResponseMaybeUninit<A, Args, R>;

    /// Get an uninitialized response for the return value type.
    fn loan_response_uninit(&self) -> ComResult<Self::ResponseMaybeUninit>;

    /// Get an initialized response for the given return value type.
    ///
    /// Read as `loan_response(&self, value: R) -> ComResult<ResponseMut<Self, Args, R>>`.
    /// The return type is the associated ResponseMut type of the ResponseMaybeUninit.
    fn loan_response(&self, value: R) -> ComResult<<<Self as Request<A, Args, R>>::ResponseMaybeUninit as ResponseMaybeUninit<A, Args, R>>::ResponseMut>{
        let response = self.loan_response_uninit()?;
        Ok(response.write(value))
    }

    /// Send the response to the client.
    #[inline(always)]
    fn respond(&self, value: R) -> ComResult<()> {
        let response = self.loan_response(value)?;
        response.send()
    }
}

/// The `Response` is an immutable response to a function call used on client side for receiving results.
///
/// The response can only obtained by a `PendingRequest`. In case of a communication error,
/// PendingRequest will return an error instead of a response.
pub trait Response<A, Args, R>: Debug + Deref<Target = R>
where
    A: TransportAdapter + ?Sized,
    R: ReturnValue,
{
}

/// The `ResponseMaybeUninit` is an uninitialized response of a remote procedure call used on service side for sending results.
pub trait ResponseMaybeUninit<A, Args, R>: Debug
where
    A: TransportAdapter + ?Sized,
    R: ReturnValue,
{
    type ResponseMut: ResponseMut<A, Args, R>;

    /// Write the value into the buffer and render it initialized.
    fn write(self, value: R) -> Self::ResponseMut;

    /// Get a mutable pointer to the internal maybe uninitialized `R`.
    fn as_mut_ptr(&mut self) -> *mut R;

    /// Render the buffer initialized for mutable access.
    unsafe fn assume_init(self) -> Self::ResponseMut;
}

/// The `ResponseMut` is a mutable response of a remote procedure call used on service side for sending results.
pub trait ResponseMut<A, Args, R>: Debug + DerefMut<Target = R>
where
    A: TransportAdapter + ?Sized,
    R: ReturnValue,
{
    fn send(self) -> ComResult<()>;
}

/// The `Invoker` trait represents the client-side stub of remote procedures.
///
/// With an invoker the client issues invocation requests to the service side.
pub trait Invoker<A, Args, R>: Debug + Send
where
    A: TransportAdapter + ?Sized,
    Args: ParameterPack,
    R: ReturnValue,
{
    type RequestMaybeUninit: RequestMaybeUninit<A, Args, R>;
    // Response := Self::RequestMaybeUninit::RequestMut::PendingRequest::Response;

    /// Prepare an invocation request with uninitialized arguments.
    ///
    /// This loans an argument buffer for uninitialized arguments and should also prepare a return channel for the response.
    /// Writing arguments into the buffer and sending it will trigger the Method invocation process.
    ///
    fn loan_uninit(&self) -> ComResult<Self::RequestMaybeUninit>;

    /// Prepare an invocation with arguments.
    ///
    /// This loans a buffer with given arguments and also prepares a return channel for the result.
    /// Executing the obtained request will trigger the Method invocation process.
    fn loan(
        &self,
        args: Args,
    ) -> ComResult<<<Self as Invoker<A, Args, R>>::RequestMaybeUninit as RequestMaybeUninit<A, Args, R>>::RequestMut>{
        let request = self.loan_uninit()?;
        Ok(request.write(args))
    }

    /// Synchronously invoke the Method with the given arguments as blocking operation.
    ///
    /// The operation will copy the arguments into the buffer used for the invocation.
    ///
    /// This operation will block until the result of the Method invocation is available.
    /// The result of a completed Method operation will always wrap into a `ComResult` as the communication itself may fail.
    fn invoke(&self, args: Args) -> ComResult<
    <<<<Self as Invoker<A, Args, R>>
                ::RequestMaybeUninit as RequestMaybeUninit<A, Args, R>>
            ::RequestMut as RequestMut<A, Args, R>>
        ::PendingRequest as PendingRequest<A, Args, R>>
    ::Response>
    {
        let request = self.loan(args)?;
        let pending = request.execute()?;
        pending.receive()
    }

    /// Synchronously invoke the Method with the given arguments as blocking operation with timeout.
    ///
    /// The operation will copy the arguments into the buffer used for the invocation.
    ///
    /// This operation will block until the result of the Method invocation is available or the timeout occurs.
    /// The result of a completed Method operation will always wrap into a `ComResult` as the communication itself may fail.
    fn invoke_timeout(&self, args: Args, timeout: Duration) -> ComResult<
        <<<<Self as Invoker<A, Args, R>>
                    ::RequestMaybeUninit as RequestMaybeUninit<A, Args, R>>
                ::RequestMut as RequestMut<A, Args, R>>
            ::PendingRequest as PendingRequest<A, Args, R>>
        ::Response>
    {
        let request = self.loan(args)?;
        let pending = request.execute()?;
        pending.receive_timeout(timeout)
    }

    /// Invoke the Method with the given arguments as asynchronous operation.
    ///
    /// The operation will copy the arguments into the buffer used for the invocation.
    ///
    /// This operation does not block. Instead, it will return a future that, when awaited, will provide the result of the rpc invocation.
    /// The result of a completed Method operation will always wrap into a `ComResult` as the communication itself may fail.
    fn invoke_async(&self, args: Args) -> impl Future<Output = ComResult<R>> + Send;

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
    ) -> impl Future<Output = ComResult<R>> + Send;
}

/// The `Invoked` trait represents the service side skeleton of a remote procedure.
///
/// The trait is used to receive and process incoming requests and send responses.
pub trait Invoked<A, Args, R>: Debug + Send
where
    A: TransportAdapter + ?Sized,
    Args: ParameterPack,
    R: ReturnValue,
{
    type Request: Request<A, Args, R>;
    // We do not define a Response, as ResponseMaybeUninit := <Self::Request as Request<A, Args, R>>::ResponseMybeUninit;

    /// test if an incoming invocation request is present and return the corresponding request.
    ///
    /// When the incoming queue is empty, the method returns `None`.
    fn try_receive(&self) -> ComResult<Option<Self::Request>>;

    /// wait for incoming calls blocking the current thread.
    fn receive(&self) -> ComResult<Self::Request>;

    /// wait for incoming calls with a timeout blocking the current thread.
    fn receive_timeout(&self, timeout: Duration) -> ComResult<Self::Request>;

    /// wait for incoming invocations and return the result of the execution of the given function with the received arguments.
    ///
    /// When this method is successful, a request has been received and the response has been sent to the client.
    /// The method returns with errors if the peer closed the connection or the communication failed otherwise.
    fn receive_and_execute<F>(&self, f: F) -> ComResult<()>
    where
        F: FnOnce(&Args) -> R,
    {
        // wait for incoming invocations
        let request: Self::Request = self.receive()?;

        // call the function with the received arguments
        let result = f(request.deref());

        // build and send the response
        request.respond(result)?;
        Ok(())
    }

    /// wait for incoming invocations with timeout and return the result of the execution of the given function with the received arguments.
    fn receive_timeout_and_execute<F>(&self, f: F, timeout: Duration) -> ComResult<()>
    where
        F: FnOnce(&Args) -> R,
    {
        // wait for incoming invocations
        let request = self.receive_timeout(timeout)?;

        // call the function with the received arguments
        let result = f(request.deref());

        // build and send the response
        request.respond(result)?;
        Ok(())
    }

    /// accept creates a future that waits for incoming calls on the remote procedure port and
    /// returns the arguments and a result publish buffer.
    fn receive_async(&self) -> impl Future<Output = ComResult<Self::Request>> + Send;

    /// accept creates a future that waits with a timeout for incoming calls on the remote procedure port and
    /// returns the arguments and a result publish buffer.
    fn receive_timeout_async(
        &self,
        timeout: Duration,
    ) -> impl Future<Output = ComResult<Self::Request>> + Send;

    /// create a future that waits for incoming calls on the remote procedure port and
    /// returns the result of the invocation of the given async function with the received arguments.
    fn receive_and_execute_async<F, Fut>(&self, f: F) -> impl Future<Output = ComResult<()>>
    where
        F: Fn(&Args) -> Fut + Send,
        Fut: Future<Output = R> + Send,
    {
        async move {
            let request = self.receive_async().await?;
            let result = f(request.deref()).await;
            request.respond(result)?;
            Ok(())
        }
    }
}

/// The `Rpc` trait represents a remote procedure call.
///
/// A Rpc is always considerered stateless. This means there are different bindings for incoming client connections.
/// Therefore, the invokee of the remote procedure can directly obtained through the `Rpc` trait.
pub trait Rpc<A, Args, R>: Debug + Clone + Send
where
    A: TransportAdapter + ?Sized,
    Args: ParameterPack,
    R: ReturnValue,
{
    type Invoker: Invoker<A, Args, R>;
    type Invoked: Invoked<A, Args, R>;

    /// Get a client-side invoker for this remote procedure
    fn invoker(&self) -> ComResult<Self::Invoker>;

    /// Get a service-side remote procedure skeleton, the `Invoked`
    fn invoked(&self) -> ComResult<Self::Invoked>;
}

/// The Builder for an `Rpc` remote procedure type
pub trait RpcBuilder<A, Args, R>: Debug
where
    A: TransportAdapter + ?Sized,
    Args: ParameterPack,
    R: ReturnValue,
{
    type Rpc: Rpc<A, Args, R>;

    /// Set the queue depth of the remote procedure.
    fn with_queue_depth(self, queue_depth: usize) -> Self;

    /// Set the max fan-in of `Invoker`s on client side of the remote procedure.
    fn with_max_invokers_client(self, fan_out: usize) -> Self;

    /// Set the total max number of connected `Invoker`s on the service side of the remote procedure.
    fn with_max_invokers_service(self, fan_out: usize) -> Self;

    /// build the remote procedure
    fn build(self) -> ComResult<Self::Rpc>;
}
