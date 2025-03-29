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
pub trait Emitter<A>: Debug + Send
where
    A: TransportAdapter + ?Sized,
{
    /// Comunicate an occurrence of the signal to all listeners of the signal.
    fn emit(&self);
}

/// A signal `Listener` receives the emissions of a subscribed `Signal`.
pub trait Listener<A>: Debug + Send
where
    A: TransportAdapter + ?Sized,
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

/// A `Signal` represents a signal in the communication system notifying listeners of the occurrence of a state change connected to the signal.
///
/// The signal works like a `Condvar` but operates within the bounds of the communication subsystem.
pub trait Signal<A>: Debug + Clone + Send
where
    A: TransportAdapter + ?Sized,
{
    /// Associated Notifier type of the signal type
    type Emitter: Emitter<A>;

    /// Associated Listener type of the signal type
    type Listener: Listener<A>;

    /// Get a emit for this signal
    fn emit(&self) -> ComResult<Self::Emitter>;

    /// Get a listener for this signal
    fn listener(&self) -> ComResult<Self::Listener>;
}

/// The Builder for a `Signal`
pub trait SignalBuilder<A>: Debug
where
    A: TransportAdapter + ?Sized,
{
    /// The type of the created signal
    type Signal: Signal<A>;

    /// Build the signal
    fn build(self) -> ComResult<Self::Signal>;
}
