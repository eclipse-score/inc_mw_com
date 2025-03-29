// Copyright (c) 2025 Qorix GmbH
//
// This program and the accompanying materials are made available under the
// terms of the Apache License, Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0.
//
// SPDX-License-Identifier: Apache-2.0

use super::HeapAdapter;

use crate::base::*;

use std::cell::UnsafeCell;
use std::mem::MaybeUninit;
use std::ops::Deref;
use std::ops::DerefMut;
use std::sync::Condvar;
use std::sync::Mutex;
use std::{fmt::Debug, sync::Arc};

//
// RemoteProcedure
//

/// The inner structure of a remote procedure request and response
#[derive(Debug)]
struct HeapPendingRequestInner<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
    /// The arguments of the pending request and condition variable
    ///
    /// SAFETY: The args are initialized as soon as this inner is no longer owned by `HeapRequestMaybeUninit`
    /// SAFETY: A Deref or DerefMut on the args is only possible when they are initialized.
    args: MaybeUninit<Args>,
    args_control: (Mutex<bool>, Condvar),

    /// The result of a pending request and condition variable
    ///
    /// SAFETY: The result is initialized as soon as this inner is no longer owned by `HeapResponseMaybeUninit`
    /// SAFETY: A Deref or DerefMut on the result is only possible when it is initialized.
    result: MaybeUninit<R>,
    result_control: (Mutex<bool>, Condvar),
}

unsafe impl<Args, R> Send for HeapPendingRequestInner<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
}

unsafe impl<Args, R> Sync for HeapPendingRequestInner<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
}

impl<Args, R> HeapPendingRequestInner<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
    /// Create a new `HeapPendingRequestInner` with the given arguments and result.
    pub(crate) fn new() -> Self {
        Self {
            args: MaybeUninit::uninit(),
            args_control: (Mutex::new(false), Condvar::new()),

            result: MaybeUninit::uninit(),
            result_control: (Mutex::new(false), Condvar::new()),
        }
    }
}

#[derive(Debug)]
pub struct HeapRequestMaybeUninit<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
    inner: Arc<UnsafeCell<HeapPendingRequestInner<Args, R>>>,
}

impl<Args, R> HeapRequestMaybeUninit<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
    /// Create a new `HeapRequestMaybeUninit` with the given arguments.
    pub(crate) fn new() -> Self {
        Self {
            inner: Arc::new(UnsafeCell::new(HeapPendingRequestInner::new())),
        }
    }
}

impl<Args, R> RequestMaybeUninit<HeapAdapter, Args, R> for HeapRequestMaybeUninit<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
    type RequestMut = HeapRequestMut<Args, R>;

    fn write(self, args: Args) -> Self::RequestMut {
        // SAFETY: The `args` is initialized and the `HeapRequestMut` is created.
        unsafe {
            let inner = &mut *self.inner.get();
            inner.args = MaybeUninit::new(args);
            self.assume_init()
        }
    }

    fn as_mut_ptr(&mut self) -> *mut Args {
        self.inner.get() as *mut Args
    }

    unsafe fn assume_init(self) -> Self::RequestMut {
        Self::RequestMut { inner: self.inner }
    }
}

#[derive(Debug)]
pub struct HeapRequestMut<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
    inner: Arc<UnsafeCell<HeapPendingRequestInner<Args, R>>>,
}

impl<Args, R> Deref for HeapRequestMut<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
    type Target = Args;

    fn deref(&self) -> &Self::Target {
        // SAFETY: Only the owner of the HeapRequestMut can call this. HeapRequestMut is not Send, so we have single thread guarantee
        unsafe {
            // get the inner structure
            let inner = &mut *self.inner.get();
            debug_assert!(*inner.args_control.0.lock().unwrap());

            // SAFETY: The `args` are initialized inside HeapRequestMut
            &*(inner.args.as_ptr() as *const Args)
        }
    }
}

impl<Args, R> DerefMut for HeapRequestMut<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
    fn deref_mut(&mut self) -> &mut Self::Target {
        // SAFETY: Only the owner of the HeapRequestMut can call this. HeapRequestMut is not Send, so we have single thread guarantee
        unsafe {
            // get the inner structure
            let inner = &mut *self.inner.get();
            debug_assert!(*inner.args_control.0.lock().unwrap());

            // SAFETY: The `args` are initialized inside HeapRequestMut
            &mut *(inner.args.as_ptr() as *const Args as *mut Args)
        }
    }
}

impl<Args, R> RequestMut<HeapAdapter, Args, R> for HeapRequestMut<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
    type PendingRequest = HeapPendingRequest<Args, R>;

    fn execute(self) -> ComResult<Self::PendingRequest> {
        let inner = unsafe { &mut *self.inner.get() };

        // acquire lock
        let mut lock = inner
            .args_control
            .0
            .lock()
            .map_err(|_| ComError::LockError)?;

        // notify 
        *lock = true;
        inner.args_control.1.notify_one();

        // ok
        Ok(HeapPendingRequest { inner: self.inner })
    }
}

#[derive(Debug)]
pub struct HeapPendingRequest<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
    inner: Arc<UnsafeCell<HeapPendingRequestInner<Args, R>>>,
}

unsafe impl<Args, R> Send for HeapPendingRequest<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
}
unsafe impl<Args, R> Sync for HeapPendingRequest<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
}

impl<Args, R> PendingRequest<HeapAdapter, Args, R> for HeapPendingRequest<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
    type Response = HeapResponse<Args, R>;

    fn try_receive(&self) -> ComResult<Option<Self::Response>> {
        let inner = unsafe { &mut *self.inner.get() };

        // acquire lock
        let lock = inner
            .result_control
            .0
            .lock()
            .map_err(|_| ComError::LockError)?;

        if *lock {
            // result is already set
            return Ok(Some(HeapResponse {
                inner: self.inner.clone(),
            }));
        } else {
            // ...or not
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

        // until we have a result
        loop {
            if *lock {
                // ...which we have
                return Ok(HeapResponse {
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

    fn receive_timeout(&self, timeout: std::time::Duration) -> ComResult<Self::Response> {
        let inner = unsafe { &mut *self.inner.get() };

        // acquire lock
        let mut lock = inner
            .result_control
            .0
            .lock()
            .map_err(|_| ComError::LockError)?;

        // until we have a result
        loop {
            if *lock {
                // ...which we have
                return Ok(HeapResponse {
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
pub struct HeapRequest<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
    inner: Arc<UnsafeCell<HeapPendingRequestInner<Args, R>>>,
}

unsafe impl<Args, R> Send for HeapRequest<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
}
unsafe impl<Args, R> Sync for HeapRequest<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
}

impl<Args, R> Deref for HeapRequest<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
    type Target = Args;

    fn deref(&self) -> &Self::Target {
        // SAFETY: Only the owner of the HeapRequestMut can call this. HeapRequestMut is not Send, so we have single thread guarantee
        unsafe {
            // get the inner structure
            let inner = &mut *self.inner.get();
            debug_assert!(*inner.args_control.0.lock().unwrap());

            // SAFETY: The `args` are initialized inside HeapRequestMut
            &*(inner.args.as_ptr() as *const Args)
        }
    }
}

impl<Args, R> Request<HeapAdapter, Args, R> for HeapRequest<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
    type ResponseMaybeUninit = HeapResponseMaybeUninit<Args, R>;

    fn loan_response_uninit(&self) -> ComResult<Self::ResponseMaybeUninit> {
        Ok(HeapResponseMaybeUninit {
            inner: self.inner.clone(),
        })
    }
}

#[derive(Debug)]
pub struct HeapResponseMaybeUninit<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
    inner: Arc<UnsafeCell<HeapPendingRequestInner<Args, R>>>,
}

impl<Args, R> ResponseMaybeUninit<HeapAdapter, Args, R> for HeapResponseMaybeUninit<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
    type ResponseMut = HeapResponseMut<Args, R>;

    fn write(self, result: R) -> Self::ResponseMut {
        // SAFETY: The `args` is initialized and the `HeapRequestMut` is created.
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
pub struct HeapResponseMut<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
    inner: Arc<UnsafeCell<HeapPendingRequestInner<Args, R>>>,
}

impl<Args, R> Deref for HeapResponseMut<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
    type Target = R;

    fn deref(&self) -> &Self::Target {
        // SAFETY: Only the owner of the HeapRequestMut can call this. HeapResponseMut is not Send, so we have single thread guarantee
        unsafe {
            // get the inner structure
            let inner = &mut *self.inner.get();
            debug_assert!(*inner.result_control.0.lock().unwrap());

            // SAFETY: The `args` are initialized inside HeapRequestMut
            &*(inner.result.as_ptr() as *const R)
        }
    }
}
impl<Args, R> DerefMut for HeapResponseMut<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
    fn deref_mut(&mut self) -> &mut Self::Target {
        // SAFETY: Only the owner of the HeapRequestMut can call this. HeapResponseMut is not Send, so we have single thread guarantee
        unsafe {
            // get the inner structure
            let inner = &mut *self.inner.get();
            debug_assert!(*inner.result_control.0.lock().unwrap());

            // SAFETY: The `args` are initialized inside HeapRequestMut
            &mut *(inner.result.as_ptr() as *const R as *mut R)
        }
    }
}

impl<Args, R> ResponseMut<HeapAdapter, Args, R> for HeapResponseMut<Args, R>
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

        // notify
        *lock = true;
        inner.result_control.1.notify_one();
        Ok(())
    }
}

#[derive(Debug)]
pub struct HeapResponse<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
    inner: Arc<UnsafeCell<HeapPendingRequestInner<Args, R>>>,
}

impl<Args, R> Deref for HeapResponse<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
    type Target = R;

    fn deref(&self) -> &Self::Target {
         // SAFETY: Only the owner of the HeapRequestMut can call this. HeapResponseMut is not Send, so we have single thread guarantee
         unsafe {
            // get the inner structure
            let inner = &mut *self.inner.get();
            debug_assert!(*inner.result_control.0.lock().unwrap());

            // SAFETY: The `args` are initialized inside HeapRequestMut
            &*(inner.result.as_ptr() as *const R)
        }
    }
}

impl<Args, R> Response<HeapAdapter, Args, R> for HeapResponse<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
}
