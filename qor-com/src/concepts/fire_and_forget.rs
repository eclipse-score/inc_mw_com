// Copyright (c) 2025 Qorix GmbH
//
// This program and the accompanying materials are made available under the
// terms of the Apache License, Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0.
//
// SPDX-License-Identifier: Apache-2.0

use super::*;

use std::{
    fmt::Debug,
    future::Future,
    ops::{Deref, DerefMut},
    time::Duration,
};

////////////////////////////////////////////////////////////////
// Remote Procedure Calls: Fire & Forget Messaging Pattern

/// A `Notification` is a read-only fire-and-forget message received on the service side.
pub trait NotificationConcept<A, Args>: Debug + Send + Deref<Target = Args>
where
    A: TransportAdapter + ?Sized,
    Args: Copy + Send,
{
}

/// A `NotificationMut` is a mutable fire-and-forget message used on the client side.
pub trait NotificationMutConcept<A, Args>: Debug + Send + DerefMut<Target = Args>
where
    A: TransportAdapter + ?Sized,
    Args: Copy + Send,
{
    fn notify(&self) -> ComResult<()>;
}

/// A `NotificationMaybeUninit` is an uninitialized fire-and-forget message used on the client side.
pub trait NotificationMaybeUninitConcept<A, Args>: Debug + Send
where
    A: TransportAdapter + ?Sized,
    Args: Copy + Send,
{
    type NotificationMut: NotificationMutConcept<A, Args>;

    /// Write the arguments into the buffer and render it initialized.
    fn write(self, args: Args) -> Self::NotificationMut;

    /// Get a mutable pointer to the internal maybe uninitialized `Args`.
    fn as_mut_ptr(&mut self) -> *mut Args;

    /// Render the buffer initialized for mutable access.
    unsafe fn assume_init(self) -> Self::NotificationMut;
}

/// A Notifier is a client side entity that sends a fire-and-forget message.
pub trait NotifierConcept<A, Args>: Debug + Send
where
    A: TransportAdapter + ?Sized,
    Args: Copy + Send,
{
    /// The associated uninitialized notification type.
    type NotificationMaybeUninit: NotificationMaybeUninitConcept<A, Args>;

    /// Loan an uninitialized notification for new data to be notified.
    fn loan_uninit(&self) -> ComResult<Self::NotificationMaybeUninit>;

    /// Loan a notification with initialized data to be notified.
    fn loan(
        &self,
        args: Args,
    ) -> Result<
        <<Self as Notifier<A, Args>>::NotificationMaybeUninit as NotificationMaybeUninit<
            A,
            Args,
        >>::NotificationMut,
        ComError,
    > {
        let notification = self.loan_uninit()?;
        Ok(notification.write(args))
    }

    /// Notify the connected procedure with the given arguments.
    #[inline(always)]
    fn notify(&self, args: Args) -> ComResult<()> {
        self.loan(args)?.notify()
    }
}

/// A `Receiver` is a service side entity that receives fire-and-forget messages.
pub trait ReceiverConcept<A, Args>: Debug + Send
where
    A: TransportAdapter + ?Sized,
    Args: Copy + Send,
{
    type Notification: NotificationConcept<A, Args>;

    /// Check for new notifications and consume it if present.
    fn try_receive(&self) -> ComResult<Option<Self::Notification>>;

    /// Wait for new notifications to arrive.
    ///
    /// Upon success the method returns a receive buffer containing the new data.
    fn receive(&self) -> ComResult<Self::Notification>;

    /// Wait for new notifications to arrive with a timeout.
    fn receive_timeout(&self, timeout: Duration) -> ComResult<Self::Notification>;

    /// Receive new notifications asynchronously.
    ///
    /// This method returns a future that can be used to `await` the arrival of new data.
    fn receive_async(&self) -> impl Future<Output = ComResult<Self::Notification>>;

    /// Receive new notifications asynchronously with timeout.
    ///
    /// This method returns a future that can be used to `await` the arrival of new data.
    fn receive_timeout_async(
        &self,
        timeout: Duration,
    ) -> impl Future<Output = ComResult<Self::Notification>>;
}

/// The `FireAndForget` trait represents a fire-and-forget procedure in the communication system
pub trait FireAndForgetConcept<A, Args>: Debug + Clone + Send
where
    A: TransportAdapter + ?Sized,
    Args: Copy + Send,
{
    type Notifier: NotifierConcept<A, Args>;
    type Receiver: ReceiverConcept<A, Args>;

    /// Get a notifier for this fire-and-forget procedure
    fn notifier(&self) -> ComResult<Self::Notifier>;

    /// Get a receiver for this fire-and-forget procedure
    fn receiver(&self) -> ComResult<Self::Receiver>;
}

/// A `FireAndForgetBuilder` is a builder for creating fire-and-forget procedures.
pub trait FireAndForgetBuilderConcept<A, Args>: Debug
where
    A: TransportAdapter + ?Sized,
    Args: Copy + Send,
{
    type FireAndForget: FireAndForgetConcept<A, Args>;

    /// Build a fire-and-forget procedure with the given arguments.
    fn build(&self) -> ComResult<Self::FireAndForget>;
}
