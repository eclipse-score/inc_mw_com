// Copyright (c) 2025 Qorix GmbH
//
// This program and the accompanying materials are made available under the
// terms of the Apache License, Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0.
//
// SPDX-License-Identifier: Apache-2.0
use super::*;

use crate::types::{Signal, SignalBuilder, SignalCollector, SignalEmitter};

use crate::adapter::local::Local as LocalAdapter;

use std::time::Duration;

#[derive(Debug)]
pub(crate) enum SignalBuilderSelector {
    Local(SignalBuilder<LocalAdapter>),
}

#[derive(Debug)]
enum SignalSelector {
    Local(Arc<Signal<LocalAdapter>>),
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
    Local(SignalEmitter<LocalAdapter>),
}

#[derive(Debug)]
enum CollectorSelector {
    Local(SignalCollector<LocalAdapter>),
}

#[derive(Debug)]
pub struct DynamicEmitter {
    inner: EmitterSelector,
}

impl EmitterConcept<Dynamic> for DynamicEmitter {
    // Notify the event by setting the flag and waking up all listeners
    fn emit(&self) {
        match &self.inner {
            EmitterSelector::Local(inner) => inner.emit(),
        }
    }
}

#[derive(Debug)]
pub struct DynamicCollector {
    inner: CollectorSelector,
}

impl CollectorConcept<Dynamic> for DynamicCollector {
    #[inline(always)]
    fn check(&self) -> ComResult<bool> {
        match &self.inner {
            CollectorSelector::Local(inner) => inner.check(),
        }
    }

    #[inline(always)]
    fn check_and_reset(&self) -> ComResult<bool> {
        match &self.inner {
            CollectorSelector::Local(inner) => inner.check_and_reset(),
        }
    }

    fn wait(&self) -> ComResult<bool> {
        match &self.inner {
            CollectorSelector::Local(inner) => inner.wait(),
        }
    }

    fn wait_timeout(&self, timeout: std::time::Duration) -> ComResult<bool> {
        match &self.inner {
            CollectorSelector::Local(inner) => inner.wait_timeout(timeout),
        }
    }

    async fn wait_async(&self) -> ComResult<bool> {
        match &self.inner {
            CollectorSelector::Local(inner) => inner.wait_async().await,
        }
    }

    async fn wait_timeout_async(&self, timeout: Duration) -> ComResult<bool> {
        match &self.inner {
            CollectorSelector::Local(inner) => inner.wait_timeout_async(timeout).await,
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

impl SignalConcept<Dynamic> for DynamicSignal {
    type Emitter = DynamicEmitter;
    type Collector = DynamicCollector;

    fn emitter(&self) -> ComResult<Self::Emitter> {
        match &self.inner {
            SignalSelector::Local(inner) => {
                let emitter = inner.emitter()?;
                Ok(DynamicEmitter {
                    inner: EmitterSelector::Local(emitter),
                })
            }
        }
    }

    fn collector(&self) -> ComResult<Self::Collector> {
        match &self.inner {
            SignalSelector::Local(inner) => {
                let listener = inner.collector()?;
                Ok(DynamicCollector {
                    inner: CollectorSelector::Local(listener),
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

impl SignalBuilderConcept<Dynamic> for DynamicSignalBuilder {
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
