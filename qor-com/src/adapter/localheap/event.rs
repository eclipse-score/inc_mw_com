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
pub struct HeapEvent<'s>
where
    Self: 's,
{
    _phantom: std::marker::PhantomData<&'s ()>,
    event: Arc<(Mutex<bool>, Condvar)>,
}

impl<'s> Clone for HeapEvent<'s> {
    fn clone(&self) -> Self {
        Self {
            _phantom: std::marker::PhantomData,
            event: self.event.clone(),
        }
    }
}

impl<'s, 't> Notifier<'s, 't, HeapAdapter<'t>> for HeapEvent<'s>
where
    't: 's,
{
    // Notify the event by setting the flag and waking up all listeners
    fn notify(&self) {
        let mut event = self.event.0.lock().unwrap();
        *event = true;
        self.event.1.notify_all();
    }
}

impl<'s, 't> Listener<'s, 't, HeapAdapter<'t>> for HeapEvent<'s>
where
    't: 's,
{
    // check the event flag
    #[inline(always)]
    fn check(&self) -> ComResult<bool> {
        Ok(*self
            .event
            .0
            .lock()
            .map_err(|_| Error::from_code(qor_core::core_errors::LOCK_ERROR))?)
    }

    // check and reset the event flag
    #[inline(always)]
    fn check_and_reset(&self) -> ComResult<bool> {
        let mut event = self
            .event
            .0
            .lock()
            .map_err(|_| Error::from_code(qor_core::core_errors::LOCK_ERROR))?;

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
            .map_err(|_| Error::from_code(qor_core::core_errors::LOCK_ERROR))?;
        loop {
            if *event {
                return Ok(true);
            } else {
                event = self
                    .event
                    .1
                    .wait(event)
                    .map_err(|_| Error::from_code(qor_core::core_errors::LOCK_ERROR))?;
            }
        }
    }

    fn wait_timeout(&self, timeout: Duration) -> ComResult<bool> {
        let mut event = self
            .event
            .0
            .lock()
            .map_err(|_| Error::from_code(qor_core::core_errors::LOCK_ERROR))?;

        loop {
            if *event {
                return Ok(true);
            } else {
                let (q, r) = self
                    .event
                    .1
                    .wait_timeout(event, timeout)
                    .map_err(|_| Error::from_code(qor_core::core_errors::LOCK_ERROR))?;
                event = q;
                if r.timed_out() {
                    return Ok(false);
                }
            }
        }
    }

    async fn listen(&self) -> ComResult<bool> {
        Err(Error::FEATURE_NOT_SUPPORTED)
    }
}

impl<'s, 't> Event<'s, 't, HeapAdapter<'t>> for HeapEvent<'s>
where
    't: 's,
{
    fn notifier(&self) -> ComResult<impl Notifier<'s, 't, HeapAdapter<'t>>> {
        Ok(self.clone())
    }

    fn listener(&self) -> ComResult<impl Listener<'s, 't, HeapAdapter<'t>>> {
        Ok(self.clone())
    }
}

impl<'s> HeapEvent<'s> {
    #[inline(always)]
    pub(crate) fn new() -> Self {
        Self {
            _phantom: std::marker::PhantomData,
            event: Arc::new((Mutex::new(false), Condvar::new())),
        }
    }
}

#[derive(Debug)]
pub struct HeapEventBuilder {}

impl<'t> EventBuilder<'t, HeapAdapter<'t>> for HeapEventBuilder {
    fn build<'a>(self) -> ComResult<impl Event<'a, 't, HeapAdapter<'t>>>
    where
        't: 'a,
    {
        Ok(HeapEvent::new())
    }
}

impl HeapEventBuilder {
    #[inline(always)]
    pub fn new() -> Self {
        Self {}
    }
}
