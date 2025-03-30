// Copyright (c) 2025 Qorix GmbH
//
// This program and the accompanying materials are made available under the
// terms of the Apache License, Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0.
//
// SPDX-License-Identifier: Apache-2.0
use super::*;
use crate::base::signal::{ Emitter, Listener };

use std::time::Duration;

//TODO: check for macro rules to expand future adapters

/// Convenience type definitions for local adapter
mod local {
    pub(crate) type Adapter = crate::adapter::local::Local;
    pub(crate) type Builder = <Adapter as crate::base::TransportAdapter>::SignalBuilder;
    pub(crate) type Signal = <Builder as crate::base::SignalBuilder<Adapter>>::Signal;
    pub(crate) type Emitter = <Signal as crate::base::Signal<Adapter>>::Emitter;
    pub(crate) type Listener = <Signal as crate::base::Signal<Adapter>>::Listener;
}

#[derive(Debug)]
pub(crate) enum SignalBuilderSelector {
    Local(local::Builder),
}

#[derive(Debug)]
enum SignalSelector {
    Local(Arc<local::Signal>),
}

impl Clone for SignalSelector {
    fn clone(&self) -> Self {
        match self {
            SignalSelector::Local(inner) => SignalSelector::Local(inner.clone()),
        }
    }
}

#[derive(Debug)]
enum EmitterSelector {
    Local(local::Emitter),
}

#[derive(Debug)]
enum ListenerSelector {
    Local(local::Listener),
}

#[derive(Debug)]
pub struct DynamicSignalEmitter {
    inner: EmitterSelector,
}

impl Emitter<Dynamic> for DynamicSignalEmitter {
    // Notify the event by setting the flag and waking up all listeners
    fn emit(&self) {
        match &self.inner {
            EmitterSelector::Local(inner) => inner.emit(),
        }
    }
}

#[derive(Debug)]
pub struct DynamicSignalListener {
    inner: ListenerSelector,
}

impl Listener<Dynamic> for DynamicSignalListener {
    #[inline(always)]
    fn check(&self) -> ComResult<bool> {
        match &self.inner {
            ListenerSelector::Local(inner) => inner.check(),
        }
    }

    #[inline(always)]
    fn check_and_reset(&self) -> ComResult<bool> {
        match &self.inner {
            ListenerSelector::Local(inner) => inner.check_and_reset(),
        }
    }

    fn wait(&self) -> ComResult<bool> {
        match &self.inner {
            ListenerSelector::Local(inner) => inner.wait(),
        }
    }

    fn wait_timeout(&self, timeout: std::time::Duration) -> ComResult<bool> {
        match &self.inner {
            ListenerSelector::Local(inner) => inner.wait_timeout(timeout),
        }
    }

    async fn wait_async(&self) -> ComResult<bool> {
        match &self.inner {
            ListenerSelector::Local(inner) => inner.wait_async().await,
        }
    }

    async fn wait_timeout_async(&self, timeout: Duration) -> ComResult<bool> {
        match &self.inner {
            ListenerSelector::Local(inner) => inner.wait_timeout_async(timeout).await,
        }
    }
}

#[derive(Debug)]
pub struct DynamicSignal {
    inner: SignalSelector,
}

impl Clone for DynamicSignal {
    fn clone(&self) -> Self {
        Self {
            inner: self.inner.clone(),
        }
    }
}

impl Signal<Dynamic> for DynamicSignal {
    type Emitter = DynamicSignalEmitter;
    type Listener = DynamicSignalListener;

    fn emitter(&self) -> ComResult<Self::Emitter> {
        match &self.inner {
            SignalSelector::Local(inner) => {
                let emitter = inner.emitter()?;
                Ok(DynamicSignalEmitter {
                    inner: EmitterSelector::Local(emitter),
                })
            }
        }
    }

    fn listener(&self) -> ComResult<Self::Listener> {
        match &self.inner {
            SignalSelector::Local(inner) => {
                let listener = inner.listener()?;
                Ok(DynamicSignalListener {
                    inner: ListenerSelector::Local(listener),
                })
            }
        }
    }
}

#[derive(Debug)]
pub struct DynamicSignalBuilder {
    inner: SignalBuilderSelector,
}

impl DynamicSignalBuilder {
    #[inline(always)]
    pub(crate) fn new(builder: SignalBuilderSelector) -> Self {
        Self { inner: builder }
    }
}

impl SignalBuilder<Dynamic> for DynamicSignalBuilder {
    type Signal = DynamicSignal;

    fn build(self) -> ComResult<Self::Signal> {
        match self.inner {
            SignalBuilderSelector::Local(builder) => {
                let signal = builder.build()?;
                Ok(DynamicSignal {
                    inner: SignalSelector::Local(Arc::new(signal)),
                })
            }
        }
    }
}
