// Copyright (c) 2025 Qorix GmbH
//
// This program and the accompanying materials are made available under the
// terms of the Apache License, Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0.
//
// SPDX-License-Identifier: Apache-2.0
use super::HeapAdapter;

use crate::base::*;
use qor_core::prelude::*;

use std::{
    fmt::Debug,
    sync::{Arc, Condvar, Mutex},
    time::Duration,
};

//
// Event
//

#[derive(Debug)]
pub struct HeapSignal
{
    event: Arc<(Mutex<bool>, Condvar)>,
}

impl Clone for HeapSignal {
    fn clone(&self) -> Self {
        Self {
            event: self.event.clone(),
        }
    }
}

impl Notifier<HeapAdapter> for HeapSignal
{
    // Notify the event by setting the flag and waking up all listeners
    fn notify(&self) {
        let mut event = self.event.0.lock().unwrap();
        *event = true;
        self.event.1.notify_all();
    }
}

impl Listener<HeapAdapter> for HeapSignal
{
    // check the event flag
    #[inline(always)]
    fn check(&self) -> ComResult<bool> {
        Ok(*self
            .event
            .0
            .lock()
            .map_err(|_| ComError::LockError)?)
    }

    // check and reset the event flag
    #[inline(always)]
    fn check_and_reset(&self) -> ComResult<bool> {
        let mut event = self
            .event
            .0
            .lock()
            .map_err(|_| ComError::LockError)?;

        // store state and reset
        let result = *event;
        *event = false;
        Ok(result)
    }

    fn wait(&self) -> ComResult<bool> {
        let mut event = self
            .event
            .0
            .lock()
            .map_err(|_| ComError::LockError)?;
        loop {
            if *event {
                return Ok(true);
            } else {
                event = self
                    .event
                    .1
                    .wait(event)
                    .map_err(|_| ComError::LockError)?;
            }
        }
    }

    fn wait_timeout(&self, timeout: Duration) -> ComResult<bool> {
        let mut event = self
            .event
            .0
            .lock()
            .map_err(|_| ComError::LockError)?;

        loop {
            if *event {
                return Ok(true);
            } else {
                let (q, r) = self
                    .event
                    .1
                    .wait_timeout(event, timeout)
                    .map_err(|_| ComError::LockError)?;
                event = q;
                if r.timed_out() {
                    return Ok(false);
                }
            }
        }
    }

    async fn wait_async(&self) -> ComResult<bool> {
        unimplemented!()
    }

    async fn wait_timeout_async(&self, _timeout: Duration) -> ComResult<bool> {
        unimplemented!()
    }
}

impl Signal<HeapAdapter> for HeapSignal
{
    type Notifier = Self;
    type Listener = Self;
    
    fn notifier(&self) -> ComResult<Self::Notifier> {
        Ok(self.clone())
    }

    fn listener(&self) -> ComResult<Self::Listener> {
        Ok(self.clone())
    }
}

impl HeapSignal {
    #[inline(always)]
    pub(crate) fn new() -> Self {
        Self {
            event: Arc::new((Mutex::new(false), Condvar::new())),
        }
    }
}

#[derive(Debug)]
pub struct HeapSignalBuilder {}

impl SignalBuilder<HeapAdapter> for HeapSignalBuilder {
    type Signal  = HeapSignal;
    fn build(self) -> ComResult<Self::Signal> {
        Ok(HeapSignal::new())
    }
}

impl HeapSignalBuilder {
    #[inline(always)]
    pub fn new() -> Self {
        Self {}
    }
}
