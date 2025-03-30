// Copyright (c) 2025 Qorix GmbH
//
// This program and the accompanying materials are made available under the
// terms of the Apache License, Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0.
//
// SPDX-License-Identifier: Apache-2.0
use super::Dynamic;

use qor_core::prelude::*;

use crate::base::*;

use std::{
    fmt::Debug, ops::{Deref, DerefMut}, sync::Arc, time::Duration
};

//TODO: check for macro rules to expand future adapters

/// Convenience type definitions for local adapter
mod local {
    pub(crate) type Adapter = crate::adapter::local::Local;
    pub(crate) type EventBuilder<T> = <Adapter as crate::base::TransportAdapter>::EventBuilder<T>;
    pub(crate) type Event<T> =
        <EventBuilder<T> as crate::base::event::EventBuilder<Adapter, T>>::Event;
    pub(crate) type EventPublisher<T> =
        <Event<T> as crate::base::event::Event<Adapter, T>>::Publisher;
    pub(crate) type EventSubscriber<T> =
        <Event<T> as crate::base::event::Event<Adapter, T>>::Subscriber;

    pub(crate) type SampleMaybeUninit<T> =
        <EventPublisher<T> as crate::base::event::Publisher<Adapter, T>>::SampleMaybeUninit;
    pub(crate) type SampleMut<T> =
        <SampleMaybeUninit<T> as crate::base::event::SampleMaybeUninit<Adapter, T>>::SampleMut;
    pub(crate) type Sample<T> = <SampleMut<T> as crate::base::event::SampleMut<Adapter, T>>::Sample;
}

#[derive(Debug)]

pub(crate) enum EventBuilderSelector<T>
where
    T: Debug + Send + TypeTag + Coherent + Reloc,
{
    Local(local::EventBuilder<T>),
}

#[derive(Debug)]
enum EventSelector<T>
where
    T: Debug + Send + TypeTag + Coherent + Reloc,
{
    Local(Arc<local::Event<T>>),
}

impl<T> Clone for EventSelector<T>
where
    T: Debug + Send + TypeTag + Coherent + Reloc,
{
    fn clone(&self) -> Self {
        match self {
            EventSelector::Local(inner) => EventSelector::Local(inner.clone()),
        }
    }
}

#[derive(Debug)]
enum PublisherSelector<T>
where
    T: Debug + Send + TypeTag + Coherent + Reloc,
{
    Local(local::EventPublisher<T>),
}

#[derive(Debug)]
enum SubscriberSelector<T>
where
    T: Debug + Send + TypeTag + Coherent + Reloc,
{
    Local(local::EventSubscriber<T>),
}

#[derive(Debug)]
enum SampleMaybeUninitSelector<T>
where
    T: Debug + Send + TypeTag + Coherent + Reloc,
{
    Local(local::SampleMaybeUninit<T>),
}

#[derive(Debug)]
enum SampleMutSelector<T>
where
    T: Debug + Send + TypeTag + Coherent + Reloc,
{
    Local(local::SampleMut<T>),
}

#[derive(Debug)]
enum SampleSelector<T>
where
    T: Debug + Send + TypeTag + Coherent + Reloc,
{
    Local(local::Sample<T>),
}

#[derive(Debug)]
pub struct DynamicSample<T>
where
    T: Debug + Send + TypeTag + Coherent + Reloc,
{
    inner: SampleSelector<T>,
}
impl<T> Deref for DynamicSample<T>
where
    T: Debug + Send + TypeTag + Coherent + Reloc,
{
    type Target = T;

    fn deref(&self) -> &Self::Target {
        match &self.inner {
            SampleSelector::Local(inner) => inner.deref(),
        }
    }
}

impl<T> Sample<Dynamic, T> for DynamicSample<T> where T: Debug + Send + TypeTag + Coherent + Reloc {}

#[derive(Debug)]
pub struct DynamicSampleMut<T>
where
    T: Debug + Send + TypeTag + Coherent + Reloc,
{
    inner: SampleMutSelector<T>,
}

impl<T> Deref for DynamicSampleMut<T>
where
    T: Debug + Send + TypeTag + Coherent + Reloc,
{
    type Target = T;

    fn deref(&self) -> &Self::Target {
        match &self.inner {
            SampleMutSelector::Local(inner) => inner.deref(),
        }
    }
}

impl<T> DerefMut for DynamicSampleMut<T>
where
    T: Debug + Send + TypeTag + Coherent + Reloc,
{
    fn deref_mut(&mut self) -> &mut Self::Target {
        match &mut self.inner {
            SampleMutSelector::Local(inner) => inner.deref_mut(),
        }
    }
}

impl<T> SampleMut<Dynamic, T> for DynamicSampleMut<T>
where
    T: Debug + Send + TypeTag + Coherent + Reloc,
{
    type Sample = DynamicSample<T>;

    fn into_sample(self) -> Self::Sample {
        match self.inner {
            SampleMutSelector::Local(inner) => DynamicSample {
                inner: SampleSelector::Local(inner.into_sample()),
            },
        }
    }

    fn send(self) -> ComResult<()> {
        match self.inner {
            SampleMutSelector::Local(inner) => inner.send(),
        }
    }
}

#[derive(Debug)]
pub struct DynamicSampleMaybeUninit<T>
where
    T: Debug + Send + TypeTag + Coherent + Reloc,
{
    inner: SampleMaybeUninitSelector<T>,
}

impl<T> SampleMaybeUninit<Dynamic, T> for DynamicSampleMaybeUninit<T>
where
    T: Debug + Send + TypeTag + Coherent + Reloc,
{
    type SampleMut = DynamicSampleMut<T>;

    fn write(self, value: T) -> Self::SampleMut {
        match self.inner {
            SampleMaybeUninitSelector::Local(inner) => DynamicSampleMut {
                inner: SampleMutSelector::Local(inner.write(value)),
            },
        }
    }

    fn as_mut_ptr(&mut self) -> *mut T {
        match &mut self.inner {
            SampleMaybeUninitSelector::Local(inner) => inner.as_mut_ptr(),
        }
    }

    unsafe fn assume_init(self) -> Self::SampleMut {
        match self.inner {
            SampleMaybeUninitSelector::Local(inner) => DynamicSampleMut {
                inner: SampleMutSelector::Local(inner.assume_init()),
            },
        }
    }
}

#[derive(Debug)]
pub struct DynamicPublisher<T>
where
    T: Debug + Send + TypeTag + Coherent + Reloc,
{
    inner: PublisherSelector<T>,
}

impl<T> Publisher<Dynamic, T> for DynamicPublisher<T>
where
    T: Debug + Send + TypeTag + Coherent + Reloc,
{
    type SampleMaybeUninit = DynamicSampleMaybeUninit<T>;

    fn loan_uninit(&self) -> ComResult<Self::SampleMaybeUninit> {
        match &self.inner {
            PublisherSelector::Local(inner) => {
                let sample = inner.loan_uninit()?;
                Ok(DynamicSampleMaybeUninit {
                    inner: SampleMaybeUninitSelector::Local(sample),
                })
            }
        }
    }
}

#[derive(Debug)]
pub struct DynamicSubscriber<T>
where
    T: Debug + Send + TypeTag + Coherent + Reloc,
{
    inner: SubscriberSelector<T>,
}
impl<T> Subscriber<Dynamic, T> for DynamicSubscriber<T>
where
    T: Debug + Send + TypeTag + Coherent + Reloc,
{
    type Sample = DynamicSample<T>;

    fn try_receive(&self) -> ComResult<Option<Self::Sample>> {
        match &self.inner {
            SubscriberSelector::Local(inner) => {
                let sample = inner.try_receive()?;
                Ok(sample.map(|s| DynamicSample {
                    inner: SampleSelector::Local(s),
                }))
            }
        }
    }

    fn receive(&self) -> ComResult<Self::Sample> {
        match &self.inner {
            SubscriberSelector::Local(inner) => {
                let sample = inner.receive()?;
                Ok(DynamicSample {
                    inner: SampleSelector::Local(sample),
                })
            }
        }
    }

    fn receive_timeout(&self, timeout: Duration) -> ComResult<Self::Sample> {
        match &self.inner {
            SubscriberSelector::Local(inner) => {
                let sample = inner.receive_timeout(timeout)?;
                Ok(DynamicSample {
                    inner: SampleSelector::Local(sample),
                })
            }
        }
    }

    async fn receive_async(&self) -> ComResult<Self::Sample> {
        match &self.inner {
            SubscriberSelector::Local(inner) => {
                inner.receive_async().await.map(|s| DynamicSample {
                    inner: SampleSelector::Local(s),
                })
            }
        }
    }

    async fn receive_timeout_async(&self, timeout: Duration) -> ComResult<Self::Sample> {
        match &self.inner {
            SubscriberSelector::Local(inner) => {
                inner
                    .receive_timeout_async(timeout)
                    .await
                    .map(|s| DynamicSample {
                        inner: SampleSelector::Local(s),
                    })
            }
        }
    }
}

#[derive(Debug)]
pub struct DynamicEvent<T>
where
    T: Debug + Send + TypeTag + Coherent + Reloc,
{
    inner: EventSelector<T>,
}

impl<T> Clone for DynamicEvent<T>
where
    T: Debug + Send + TypeTag + Coherent + Reloc,
{
    fn clone(&self) -> Self {
        Self {
            inner: self.inner.clone(),
        }
    }
}

impl<T> Event<Dynamic, T> for DynamicEvent<T>
where
    T: Debug + Send + TypeTag + Coherent + Reloc,
{
    type Publisher = DynamicPublisher<T>;
    type Subscriber = DynamicSubscriber<T>;

    fn publisher(&self) -> ComResult<Self::Publisher> {
        match &self.inner {
            EventSelector::Local(inner) => {
                let publisher = inner.publisher()?;
                Ok(DynamicPublisher {
                    inner: PublisherSelector::Local(publisher),
                })
            }
        }
    }

    fn subscriber(&self) -> ComResult<Self::Subscriber> {
        match &self.inner {
            EventSelector::Local(inner) => {
                let subscriber = inner.subscriber()?;
                Ok(DynamicSubscriber {
                    inner: SubscriberSelector::Local(subscriber),
                })
            }
        }
    }
}

#[derive(Debug)]
pub struct DynamicEventBuilder<T>
where
    T: Debug + Send + TypeTag + Coherent + Reloc,
{
    inner: EventBuilderSelector<T>,
}

impl<T> DynamicEventBuilder<T>
where
    T: Debug + Send + TypeTag + Coherent + Reloc,
{
    #[inline(always)]
    pub(crate) fn new(inner: EventBuilderSelector<T>) -> Self {
        Self { inner }
    }
}

impl<T> EventBuilder<Dynamic, T> for DynamicEventBuilder<T>
where
    T: Debug + Send + TypeTag + Coherent + Reloc,
{
    type Event = DynamicEvent<T>;

    fn with_queue_depth(self, queue_depth: usize) -> Self {
        match self.inner {
            EventBuilderSelector::Local(inner) => {
                let builder = inner.with_queue_depth(queue_depth);
                DynamicEventBuilder {
                    inner: EventBuilderSelector::Local(builder),
                }
            }
        }
    }

    fn with_queue_policy(self, queue_policy: QueuePolicy) -> Self {
        match self.inner {
            EventBuilderSelector::Local(inner) => {
                let builder = inner.with_queue_policy(queue_policy);
                DynamicEventBuilder {
                    inner: EventBuilderSelector::Local(builder),
                }
            }
        }
    }
    fn with_max_fan_in(self, max_fan_in: usize) -> Self {
        match self.inner {
            EventBuilderSelector::Local(inner) => {
                let builder = inner.with_max_fan_in(max_fan_in);
                DynamicEventBuilder {
                    inner: EventBuilderSelector::Local(builder),
                }
            }
        }
    }
    fn with_max_fan_out(self, max_fan_out: usize) -> Self {
        match self.inner {
            EventBuilderSelector::Local(inner) => {
                let builder = inner.with_max_fan_out(max_fan_out);
                DynamicEventBuilder {
                    inner: EventBuilderSelector::Local(builder),
                }
            }
        }
    }

    fn build(self) -> ComResult<Self::Event> {
        match self.inner {
            EventBuilderSelector::Local(inner) => {
                let event = inner.build()?;
                Ok(DynamicEvent {
                    inner: EventSelector::Local(Arc::new(event)),
                })
            }
        }
    }
}
