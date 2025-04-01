// Copyright (c) 2025 Qorix GmbH
//
// This program and the accompanying materials are made available under the
// terms of the Apache License, Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0.
//
// SPDX-License-Identifier: Apache-2.0
use super::Local;

use crate::base::*;
use crate::base::signal::{ Emitter, Collector };

use std::{
    fmt::Debug, sync::{Arc, Condvar, Mutex}, time::Duration
};

//
// Event
//

#[derive(Debug)]
pub struct LocalSignal
{
    event: Arc<(Mutex<bool>, Condvar)>,
}

impl Clone for LocalSignal {
    fn clone(&self) -> Self {
        Self {
            event: self.event.clone(),
        }
    }
}

impl Emitter<Local> for LocalSignal
{
    // Notify the event by setting the flag and waking up all listeners
    fn emit(&self) {
        let mut event = self.event.0.lock().unwrap();
        *event = true;
        self.event.1.notify_all();
    }
}

impl Collector<Local> for LocalSignal
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
                *event = false;
                return Ok(true);
            } else {
                let (q, r) = self
                    .event
                    .1
                    .wait_timeout(event, timeout)
                    .map_err(|_| ComError::LockError)?;
                event = q;
                if r.timed_out() {
                    return Err(ComError::Timeout);
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

impl Signal<Local> for LocalSignal
{
    type Emitter = Self;
    type Collector = Self;
    
    fn emitter(&self) -> ComResult<Self::Emitter> {
        Ok(self.clone())
    }

    fn collector(&self) -> ComResult<Self::Collector> {
        Ok(self.clone())
    }
}

impl LocalSignal {
    #[inline(always)]
    pub(crate) fn new() -> Self {
        Self {
            event: Arc::new((Mutex::new(false), Condvar::new())),
        }
    }
}

#[derive(Debug)]
pub struct LocalSignalBuilder {}

impl SignalBuilder<Local> for LocalSignalBuilder {
    type Signal  = LocalSignal;
    fn build(self) -> ComResult<Self::Signal> {
        Ok(LocalSignal::new())
    }
}

impl LocalSignalBuilder {
    #[inline(always)]
    pub fn new() -> Self {
        Self {}
    }
}
