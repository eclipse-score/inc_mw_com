// Copyright (c) 2025 Qorix GmbH
//
// This program and the accompanying materials are made available under the
// terms of the Apache License, Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0.
//
// SPDX-License-Identifier: Apache-2.0

use super::LocalAdapter;

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
struct LocalPendingRequestInner<Args, R>
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
    result_control: (Mutex<bool>, Condvar),
}

unsafe impl<Args, R> Send for LocalPendingRequestInner<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
}

unsafe impl<Args, R> Sync for LocalPendingRequestInner<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
}

impl<Args, R> LocalPendingRequestInner<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
    /// Create a new `LocalPendingRequestInner` with the given arguments and result.
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
pub struct LocalRequestMaybeUninit<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
    inner: Arc<UnsafeCell<LocalPendingRequestInner<Args, R>>>,
}

impl<Args, R> LocalRequestMaybeUninit<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
    /// Create a new `LocalRequestMaybeUninit` with the given arguments.
    pub(crate) fn new() -> Self {
        Self {
            inner: Arc::new(UnsafeCell::new(LocalPendingRequestInner::new())),
        }
    }
}

impl<Args, R> RequestMaybeUninit<LocalAdapter, Args, R> for LocalRequestMaybeUninit<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
    type RequestMut = LocalRequestMut<Args, R>;

    fn write(self, args: Args) -> Self::RequestMut {
        // SAFETY: The `args` is initialized and the `LocalRequestMut` is created.
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
pub struct LocalRequestMut<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
    inner: Arc<UnsafeCell<LocalPendingRequestInner<Args, R>>>,
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
            let inner = &mut *self.inner.get();
            debug_assert!(*inner.args_control.0.lock().unwrap());

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
            let inner = &mut *self.inner.get();
            debug_assert!(*inner.args_control.0.lock().unwrap());

            // SAFETY: The `args` are initialized inside LocalRequestMut
            &mut *(inner.args.as_ptr() as *const Args as *mut Args)
        }
    }
}

impl<Args, R> RequestMut<LocalAdapter, Args, R> for LocalRequestMut<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
    type PendingRequest = LocalPendingRequest<Args, R>;

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
        Ok(LocalPendingRequest { inner: self.inner })
    }
}

#[derive(Debug)]
pub struct LocalPendingRequest<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
    inner: Arc<UnsafeCell<LocalPendingRequestInner<Args, R>>>,
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

impl<Args, R> PendingRequest<LocalAdapter, Args, R> for LocalPendingRequest<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
    type Response = LocalResponse<Args, R>;

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
            return Ok(Some(LocalResponse {
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
    inner: Arc<UnsafeCell<LocalPendingRequestInner<Args, R>>>,
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
            let inner = &mut *self.inner.get();
            debug_assert!(*inner.args_control.0.lock().unwrap());

            // SAFETY: The `args` are initialized inside LocalRequestMut
            &*(inner.args.as_ptr() as *const Args)
        }
    }
}

impl<Args, R> Request<LocalAdapter, Args, R> for LocalRequest<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
    type ResponseMaybeUninit = LocalResponseMaybeUninit<Args, R>;

    fn loan_response_uninit(&self) -> ComResult<Self::ResponseMaybeUninit> {
        Ok(LocalResponseMaybeUninit {
            inner: self.inner.clone(),
        })
    }
}

#[derive(Debug)]
pub struct LocalResponseMaybeUninit<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
    inner: Arc<UnsafeCell<LocalPendingRequestInner<Args, R>>>,
}

impl<Args, R> ResponseMaybeUninit<LocalAdapter, Args, R> for LocalResponseMaybeUninit<Args, R>
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
    inner: Arc<UnsafeCell<LocalPendingRequestInner<Args, R>>>,
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
            debug_assert!(*inner.result_control.0.lock().unwrap());

            // SAFETY: The `args` are initialized inside LocalRequestMut
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
            debug_assert!(*inner.result_control.0.lock().unwrap());

            // SAFETY: The `args` are initialized inside LocalRequestMut
            &mut *(inner.result.as_ptr() as *const R as *mut R)
        }
    }
}

impl<Args, R> ResponseMut<LocalAdapter, Args, R> for LocalResponseMut<Args, R>
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
pub struct LocalResponse<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
    inner: Arc<UnsafeCell<LocalPendingRequestInner<Args, R>>>,
}

impl<Args, R> Deref for LocalResponse<Args, R>
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
            debug_assert!(*inner.result_control.0.lock().unwrap());

            // SAFETY: The `args` are initialized inside LocalRequestMut
            &*(inner.result.as_ptr() as *const R)
        }
    }
}

impl<Args, R> Response<LocalAdapter, Args, R> for LocalResponse<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
}


// on with: LocalInvoker, LocalInvokee, LocalRemoteProcedure and LocalRemoteProcedureBuilder

/// LocalInvoker
#[derive(Debug)]
pub struct LocalInvoker<Args, R>
where
    Args: ParameterPack,
    R: ReturnValue,
{
    inner: Arc<UnsafeCell<LocalPendingRequestInner<Args, R>>>,
}