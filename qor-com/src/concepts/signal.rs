// Copyright (c) 2025 Qorix GmbH
//
// This program and the accompanying materials are made available under the
// terms of the Apache License, Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0.
//
// SPDX-License-Identifier: Apache-2.0

use super::*;

use std::{fmt::Debug, future::Future, time::Duration};

////////////////////////////////////////////////////////////////
// Signal: Event-Driven Synchronization Messaging Pattern

/// The signal `Emitter` trait is used to notify occurrences of the signal to the subscribed `Listners`.
pub trait EmitterConcept<A>: Debug + Send
where
    A: TransportAdapterConcept + ?Sized,
{
    /// Comunicate an occurrence of the signal to all listeners of the signal.
    fn emit(&self);
}

/// A signal `Collector` receives the emissions of the subscribed `Signal`.
pub trait CollectorConcept<A>: Debug + Send
where
    A: TransportAdapterConcept + ?Sized,
{
    /// Check the current state of the signal.
    ///
    /// Returns `true` if the signal is set. Does not change the state of the signal.
    fn check(&self) -> ComResult<bool>;

    /// Check the current state of the signal and reset the signal atomically.
    ///
    /// Returns `true` if the signal was set before the reset.
    /// After this operation the signal is reset.
    fn check_and_reset(&self) -> ComResult<bool>;

    /// Wait for notifications of the signal.
    ///
    /// This blocks the current thread
    fn wait(&self) -> ComResult<bool>;

    /// Wait for notifications of the signal with a timeout.
    ///
    /// This blocks the current thread.
    /// The boolean value indicates with `true` if the timeout has expired.
    fn wait_timeout(&self, timeout: Duration) -> ComResult<bool>;

    /// Wait asynchronously for notifications of the signal.
    fn wait_async(&self) -> impl Future<Output = ComResult<bool>>;

    /// Wait asynchronously for notification of the signal with timeout.
    fn wait_timeout_async(&self, timeout: Duration) -> impl Future<Output = ComResult<bool>>;
}

/// A `Signal` represents an occurence of a state change. An emitter notifies about the change and a collector listens to such notifications.
///
/// The signal works like a `Condvar` but operates within the bounds of the communication subsystem.
pub trait SignalConcept<A>: Debug + Clone + Send
where
    A: TransportAdapterConcept + ?Sized,
{
    /// Associated Notifier type of the signal type
    type Emitter: EmitterConcept<A>;

    /// Associated Listener type of the signal type
    type Collector: CollectorConcept<A>;

    /// Get a emit for this signal
    fn emitter(&self) -> ComResult<Self::Emitter>;

    /// Get a collector for this signal
    fn collector(&self) -> ComResult<Self::Collector>;
}

/// The Builder for a `Signal`
pub trait SignalBuilderConcept<A>: Debug
where
    A: TransportAdapterConcept + ?Sized,
{
    /// The type of the created signal
    type Signal: SignalConcept<A>;

    /// Build the signal
    fn build(self) -> ComResult<Self::Signal>;
}
