// Copyright (c) 2025 Qorix GmbH
//
// This program and the accompanying materials are made available under the
// terms of the Apache License, Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0.
//
// SPDX-License-Identifier: Apache-2.0

use qor_core::prelude::*;
use super::Local;

use crate::base::remote_procedure::{
    Invoked, Invoker, PendingRequest, Rpc, RpcBuilder, Request,
    RequestMaybeUninit, RequestMut, Response, ResponseMaybeUninit, ResponseMut,
};
use crate::base::*;

use std::cell::UnsafeCell;
use std::collections::VecDeque;
use std::marker::PhantomData;
use std::mem::MaybeUninit;
use std::ops::Deref;
use std::ops::DerefMut;
use std::sync::Condvar;
use std::sync::Mutex;
use std::{fmt::Debug, sync::Arc};

//
// Rpc
//

type PendingQueue<Args, R> = Arc<(
    Mutex<VecDeque<Arc<UnsafeCell<LocalRequestState<Args, R>>>>>,
    Condvar,
)>;

/// The inner structure of a remote procedure request and response
#[derive(Debug)]
struct LocalRequestState<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
    /// The arguments of the pending request and condition variable
    ///
    /// SAFETY: The args are initialized as soon as this inner is no longer owned by `LocalRequestMaybeUninit`
    /// SAFETY: A Deref or DerefMut on the args is only possible when they are initialized.
    args: MaybeUninit<Args>,
    args_control: (Mutex<bool>, Condvar),

    /// The result of a pending request and condition variable
    ///
    /// SAFETY: The result is initialized as soon as this inner is no longer owned by `LocalResponseMaybeUninit`
    /// SAFETY: A Deref or DerefMut on the result is only possible when it is initialized.
    result: MaybeUninit<R>,
    result_control: (Mutex<(bool, bool)>, Condvar),
}

unsafe impl<Args, R> Send for LocalRequestState<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
}

unsafe impl<Args, R> Sync for LocalRequestState<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
}

impl<Args, R> LocalRequestState<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
    /// Create a new `LocalPendingRequestInner` with the given arguments and result.
    fn new() -> Self {
        Self {
            args: MaybeUninit::uninit(),
            args_control: (Mutex::new(false), Condvar::new()),

            result: MaybeUninit::uninit(),
            result_control: (Mutex::new((false, false)), Condvar::new()),
        }
    }
}

#[derive(Debug)]
pub struct LocalRequestMaybeUninit<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
    queue: PendingQueue<Args, R>,
    state: Arc<UnsafeCell<LocalRequestState<Args, R>>>,
}

impl<Args, R> LocalRequestMaybeUninit<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
    /// Create a new `LocalRequestMaybeUninit` with the given arguments.
    fn new(queue: PendingQueue<Args, R>) -> Self {
        Self {
            queue,
            state: Arc::new(UnsafeCell::new(LocalRequestState::new())),
        }
    }
}

impl<Args, R> RequestMaybeUninit<Local, Args, R> for LocalRequestMaybeUninit<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
    type RequestMut = LocalRequestMut<Args, R>;

    fn write(self, args: Args) -> Self::RequestMut {
        // SAFETY: The `args` is initialized and the `LocalRequestMut` is created.
        unsafe {
            let inner = &mut *self.state.get();
            inner.args = MaybeUninit::new(args);
            self.assume_init()
        }
    }

    fn as_mut_ptr(&mut self) -> *mut Args {
        self.state.get() as *mut Args
    }

    unsafe fn assume_init(self) -> Self::RequestMut {
        Self::RequestMut {
            queue: self.queue,
            request: self.state,
        }
    }
}

#[derive(Debug)]
pub struct LocalRequestMut<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
    queue: PendingQueue<Args, R>,
    request: Arc<UnsafeCell<LocalRequestState<Args, R>>>,
}

impl<Args, R> Deref for LocalRequestMut<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
    type Target = Args;

    fn deref(&self) -> &Self::Target {
        // SAFETY: Only the owner of the LocalRequestMut can call this. LocalRequestMut is not Send, so we have single thread guarantee
        unsafe {
            // get the inner structure
            let inner = &mut *self.request.get();

            // SAFETY: The `args` are initialized inside LocalRequestMut
            &*(inner.args.as_ptr() as *const Args)
        }
    }
}

impl<Args, R> DerefMut for LocalRequestMut<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
    fn deref_mut(&mut self) -> &mut Self::Target {
        // SAFETY: Only the owner of the LocalRequestMut can call this. LocalRequestMut is not Send, so we have single thread guarantee
        unsafe {
            // get the inner structure
            let inner = &mut *self.request.get();

            // SAFETY: The `args` are initialized inside LocalRequestMut
            &mut *(inner.args.as_ptr() as *const Args as *mut Args)
        }
    }
}

impl<Args, R> RequestMut<Local, Args, R> for LocalRequestMut<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
    type PendingRequest = LocalPendingRequest<Args, R>;

    fn execute(self) -> ComResult<Self::PendingRequest> {
        let inner = unsafe { &mut *self.request.get() };
        debug_assert!(!inner.args.as_ptr().is_null()); // must never happen as we cannot get here without initialized args

        // lock queue
        let mut queue = self.queue.0.lock().map_err(|_| ComError::LockError)?;

        // push request to queue of pending requests
        queue.push_back(self.request.clone());

        // notify queue
        self.queue.1.notify_all();

        // ok
        Ok(LocalPendingRequest {
            inner: self.request,
        })
    }
}

#[derive(Debug)]
pub struct LocalPendingRequest<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
    inner: Arc<UnsafeCell<LocalRequestState<Args, R>>>,
}

unsafe impl<Args, R> Send for LocalPendingRequest<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
}
unsafe impl<Args, R> Sync for LocalPendingRequest<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
}

impl<Args, R> PendingRequest<Local, Args, R> for LocalPendingRequest<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
    type Response = LocalResponse<Args, R>;

    fn try_receive(&self) -> ComResult<Option<Self::Response>> {
        // SAFETY: This will work as we checked _and_ hold the safety lock
        let inner = unsafe { &mut *self.inner.get() };

        // acquire the inner lock
        let mut lock = inner
            .result_control
            .0
            .lock()
            .map_err(|_| ComError::LockError)?;

        // check on result
        if (*lock).1 {
            // we have already consumed the result
            Err(ComError::ResponseConsumed)
        } else if (*lock).0 {
            // result is set but not consumed

            // mark consumed
            (*lock).1 = true;

            // and return response
            return Ok(Some(LocalResponse {
                inner: self.inner.clone(), // SAFETY: cannot fail as we have a value for sure here
            }));
        } else {
            // result is not yet set
            Ok(None)
        }
    }

    fn receive(&self) -> ComResult<Self::Response> {
        let inner = unsafe { &mut *self.inner.get() };

        // acquire lock
        let mut lock = inner
            .result_control
            .0
            .lock()
            .map_err(|_| ComError::LockError)?;

        if (*lock).1 {
            // double call results in error
            Err(ComError::ResponseConsumed)
        } else {
            // until we have a result
            loop {
                if (*lock).0 {
                    // ...which we have
                    return Ok(LocalResponse {
                        inner: self.inner.clone(),
                    });
                } else {
                    // ...or not: wait for Condvar
                    lock = inner
                        .result_control
                        .1
                        .wait(lock)
                        .map_err(|_| ComError::LockError)?;
                }
            }
        }
    }

    fn receive_timeout(&self, timeout: std::time::Duration) -> ComResult<Self::Response> {
        let inner = unsafe { &mut *self.inner.get() };

        // acquire lock
        let mut lock = inner
            .result_control
            .0
            .lock()
            .map_err(|_| ComError::LockError)?;

        if (*lock).1 {
            // result already consumed
            Err(ComError::ResponseConsumed)
        } else {
            // until we have a result
            loop {
                if (*lock).0 {
                    // ...which we have
                    return Ok(LocalResponse {
                        inner: self.inner.clone(),
                    });
                } else {
                    // ...or not: wait for Condvar with timeout
                    let (new_lock, tor) = inner
                        .result_control
                        .1
                        .wait_timeout(lock, timeout)
                        .map_err(|_| ComError::LockError)?;

                    // check if we have a result
                    if tor.timed_out() {
                        return Err(ComError::Timeout);
                    } else {
                        // looks good
                        lock = new_lock;
                    }
                }
            }
        }
    }

    async fn receive_async(&self) -> ComResult<Self::Response> {
        unimplemented!();
    }

    async fn receive_timeout_async(
        &self,
        _timeout: std::time::Duration,
    ) -> ComResult<Self::Response> {
        unimplemented!();
    }
}

#[derive(Debug)]
pub struct LocalRequest<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
    state: Arc<UnsafeCell<LocalRequestState<Args, R>>>,
}

unsafe impl<Args, R> Send for LocalRequest<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
}
unsafe impl<Args, R> Sync for LocalRequest<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
}

impl<Args, R> Deref for LocalRequest<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
    type Target = Args;

    fn deref(&self) -> &Self::Target {
        // SAFETY: Only the owner of the LocalRequestMut can call this. LocalRequestMut is not Send, so we have single thread guarantee
        unsafe {
            // get the inner structure
            let inner = &mut *self.state.get();
            debug_assert!(*inner.args_control.0.lock().unwrap());

            // SAFETY: The `args` are initialized inside LocalRequestMut
            &*(inner.args.as_ptr() as *const Args)
        }
    }
}

impl<Args, R> Request<Local, Args, R> for LocalRequest<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
    type ResponseMaybeUninit = LocalResponseMaybeUninit<Args, R>;

    fn loan_response_uninit(&self) -> ComResult<Self::ResponseMaybeUninit> {
        Ok(LocalResponseMaybeUninit {
            inner: self.state.clone(),
        })
    }
}

#[derive(Debug)]
pub struct LocalResponseMaybeUninit<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
    inner: Arc<UnsafeCell<LocalRequestState<Args, R>>>,
}

impl<Args, R> ResponseMaybeUninit<Local, Args, R> for LocalResponseMaybeUninit<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
    type ResponseMut = LocalResponseMut<Args, R>;

    fn write(self, result: R) -> Self::ResponseMut {
        // SAFETY: The `args` is initialized and the `LocalRequestMut` is created.
        unsafe {
            let inner = &mut *self.inner.get();
            inner.result = MaybeUninit::new(result);
            self.assume_init()
        }
    }

    fn as_mut_ptr(&mut self) -> *mut R {
        self.inner.get() as *mut R
    }

    unsafe fn assume_init(self) -> Self::ResponseMut {
        Self::ResponseMut { inner: self.inner }
    }
}

#[derive(Debug)]
pub struct LocalResponseMut<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
    inner: Arc<UnsafeCell<LocalRequestState<Args, R>>>,
}

impl<Args, R> Deref for LocalResponseMut<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
    type Target = R;

    fn deref(&self) -> &Self::Target {
        // SAFETY: Only the owner of the LocalRequestMut can call this. LocalResponseMut is not Send, so we have single thread guarantee
        unsafe {
            // get the inner structure
            let inner = &mut *self.inner.get();

            // SAFETY: The `result` is initialized inside LocalRequestMut
            &*(inner.result.as_ptr() as *const R)
        }
    }
}
impl<Args, R> DerefMut for LocalResponseMut<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
    fn deref_mut(&mut self) -> &mut Self::Target {
        // SAFETY: Only the owner of the LocalRequestMut can call this. LocalResponseMut is not Send, so we have single thread guarantee
        unsafe {
            // get the inner structure
            let inner = &mut *self.inner.get();
            debug_assert!(!inner.result_control.0.lock().unwrap().0); // SAFETY: as `send` will later consume `self` here the result is not frozen yet

            // SAFETY: The `result` is initialized inside LocalRequestMut
            &mut *(inner.result.as_ptr() as *const R as *mut R)
        }
    }
}

impl<Args, R> ResponseMut<Local, Args, R> for LocalResponseMut<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
    fn send(self) -> ComResult<()> {
        let inner = unsafe { &mut *self.inner.get() };

        // acquire lock
        let mut lock = inner
            .result_control
            .0
            .lock()
            .map_err(|_| ComError::LockError)?;

        // mark result as present and frozen
        (*lock).0 = true;

        // notify potential pending requests
        inner.result_control.1.notify_one();
        Ok(())
    }
}

#[derive(Debug)]
pub struct LocalResponse<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
    inner: Arc<UnsafeCell<LocalRequestState<Args, R>>>,
}

impl<Args, R> Deref for LocalResponse<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
    type Target = R;

    fn deref(&self) -> &Self::Target {
        // SAFETY: Only the owner of the LocalResponse can call this. LocalResponse is not Send, so we have single thread guarantee
        unsafe {
            // get the inner structure
            let inner = &mut *self.inner.get();
            debug_assert!(inner.result_control.0.lock().unwrap().0);

            // SAFETY: The `args` are initialized inside LocalRequestMut
            &*(inner.result.as_ptr() as *const R)
        }
    }
}

impl<Args, R> Response<Local, Args, R> for LocalResponse<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
}

#[derive(Debug)]
pub struct LocalInvoker<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
    queue: PendingQueue<Args, R>,
}

unsafe impl<Args, R> Send for LocalInvoker<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
}

unsafe impl<Args, R> Sync for LocalInvoker<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
}

impl<Args, R> Invoker<Local, Args, R> for LocalInvoker<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
    type RequestMaybeUninit = LocalRequestMaybeUninit<Args, R>;

    fn loan_uninit(&self) -> ComResult<Self::RequestMaybeUninit> {
        Ok(LocalRequestMaybeUninit::new(self.queue.clone()))
    }

    async fn invoke_async(&self, _args: Args) -> ComResult<R> {
        unimplemented!()
    }

    async fn invoke_timeout_async(
        &self,
        _args: Args,
        _timeout: std::time::Duration,
    ) -> ComResult<R> {
        unimplemented!()
    }
}

#[derive(Debug)]
pub struct LocalInvoked<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
    queue: PendingQueue<Args, R>,
}

unsafe impl<Args, R> Send for LocalInvoked<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
}
unsafe impl<Args, R> Sync for LocalInvoked<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
}

impl<Args, R> Invoked<Local, Args, R> for LocalInvoked<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
    type Request = LocalRequest<Args, R>;

    fn try_receive(&self) -> ComResult<Option<Self::Request>> {
        let mut queue = self.queue.0.lock().map_err(|_| ComError::LockError)?;

        // check content
        if let Some(state) = queue.pop_front() {
            // ...and we have a request waiting
            Ok(Some(LocalRequest { state }))
        } else {
            // ...or not
            Ok(None)
        }
    }

    fn receive(&self) -> ComResult<Self::Request> {
        let mut queue = self.queue.0.lock().map_err(|_| ComError::LockError)?;

        // until we have a result
        loop {
            if let Some(state) = queue.pop_front() {
                // ...which we have
                return Ok(LocalRequest { state });
            } else {
                // ...or not: wait for Condvar
                queue = self.queue.1.wait(queue).map_err(|_| ComError::LockError)?;
            }
        }
    }

    fn receive_timeout(&self, timeout: std::time::Duration) -> ComResult<Self::Request> {
        let mut queue = self.queue.0.lock().map_err(|_| ComError::LockError)?;

        // until we have a result
        loop {
            if let Some(state) = queue.pop_front() {
                // ...which we have
                return Ok(LocalRequest { state });
            } else {
                // ...or not: wait for Condvar with timeout
                let (new_queue, tor) = self
                    .queue
                    .1
                    .wait_timeout(queue, timeout)
                    .map_err(|_| ComError::LockError)?;

                // handle timeout
                if tor.timed_out() {
                    return Err(ComError::Timeout);
                }

                // and try again
                queue = new_queue;
            }
        }
    }

    async fn receive_async(&self) -> ComResult<Self::Request> {
        unimplemented!()
    }

    async fn receive_timeout_async(
        &self,
        _timeout: std::time::Duration,
    ) -> ComResult<Self::Request> {
        unimplemented!()
    }
}

#[derive(Debug)]
pub struct LocalRpc<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
    queue: PendingQueue<Args, R>,
}

unsafe impl<Args, R> Send for LocalRpc<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
}

unsafe impl<Args, R> Sync for LocalRpc<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
}

impl<Args, R> Clone for LocalRpc<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
    fn clone(&self) -> Self {
        Self {
            queue: self.queue.clone(),
        }
    }
}

impl<Args, R> Rpc<Local, Args, R> for LocalRpc<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
    type Invoker = LocalInvoker<Args, R>;
    type Invoked = LocalInvoked<Args, R>;

    fn invoker(&self) -> ComResult<Self::Invoker> {
        Ok(LocalInvoker {
            queue: self.queue.clone(),
        })
    }

    fn invoked(&self) -> ComResult<Self::Invoked> {
        Ok(LocalInvoked {
            queue: self.queue.clone(),
        })
    }
}

impl<Args, R> LocalRpc<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
    /// Create a new `LocalRpc` with the given arguments and result.
    pub(crate) fn new() -> Self {
        Self {
            queue: Arc::new((Mutex::new(VecDeque::new()), Condvar::new())),
        }
    }
}

#[derive(Debug)]
pub struct LocalRpcBuilder<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
    max_queue_depth: usize,
    max_invokers_client: usize,
    max_invokers_service: usize,

    _phantom: std::marker::PhantomData<(Args, R)>,
}

impl<Args, R> LocalRpcBuilder<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
    pub(crate) fn new(_label: Label) -> Self {
        LocalRpcBuilder {
            max_queue_depth: 8,
            max_invokers_client: 1,
            max_invokers_service: 8,

            _phantom: PhantomData,
        }
    }
}

impl<Args, R> RpcBuilder<Local, Args, R> for LocalRpcBuilder<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
    type Rpc = LocalRpc<Args, R>;

    fn with_queue_depth(mut self, queue_depth: usize) -> Self {
        self.max_queue_depth = queue_depth;
        self
    }

    fn with_max_invokers_client(mut self, fan_in: usize) -> Self {
        self.max_invokers_client = fan_in;
        self
    }

    fn with_max_invokers_service(mut self, fan_out: usize) -> Self {
        self.max_invokers_service = fan_out;
        self
    }

    fn build(self) -> ComResult<Self::Rpc> {
        // create a new remote procedure
        let remote_procedure = LocalRpc::<Args, R>::new();

        // return the remote procedure
        Ok(remote_procedure)
    }
}
