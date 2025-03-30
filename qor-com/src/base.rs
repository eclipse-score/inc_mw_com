// Copyright (c) 2025 Qorix GmbH
//
// This program and the accompanying materials are made available under the
// terms of the Apache License, Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0.
//
// SPDX-License-Identifier: Apache-2.0

//!
//! # Overview
//!
//! The base module defines the basic types and traits used in the qor-com stack.
//!
//! # Concepts
//!
//! The fundamental concept is based on a transport adapter logic.
//!
//! ## Data Access
//!
//! Data is always accessed through buffers. Three buffer types exist:
//!
//! - Buffer: Immutable data buffer
//! - BufferMut: Mutable data buffer
//! - BufferMaybeUninit: Buffer with uninitialized data that can convert into a BufferMut or Buffer when initialized.
//!
//! Buffers can be obtained only through providing methods of communication patterns.
//!
//! ## Communication Patterns
//!
//! The transport adapter provides methods to
//! produce builders for the different communication patterns:
//!
//! - signal
//! - Event
//! - Remote Procedure
//!
//! Each of the elements can produce a sending and a receiving part.
//!
//! - signal: Notifier and Listener
//! - Event: Publisher and Subscriber
//! - Remote Procedure: Invoker and Invokee
//!
//! ## API elements
//!
//! The elements of the API are expressed through traits. This allows for flexible implementations of the transport adapter.
//! As different transport adapter implementations may exist, all API elements are generic with respect to the transport adapter.
//!
//! The corresponding generic parameter is named `TA` in the API elements.
//!
//! ## Lifetime management
//!
//! The implementation of the API elements requires great care for the validity of all entities. Therefore all traits come with
//! strict lifecycle bounds to ensure the correct usage of the API.
//!
//! The lifetime parameters used in the implementation are
//!
//! - 's: The `Self` lifetime of each trait implementation
//! - 't: The lifetime of the underlying transport adapter
//! - 'n: The lifetime of successing buffers created from a MaybeUninit
//TODO: See if we can get rid of 'n if we use a local 'a for all buffers
//! - 'a: The lifetime given by the caller of a method.
//!  

use qor_core::prelude::*;

use std::fmt::Debug;
use std::error::Error;

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub enum ComError {
    LockError,
    Timeout,
    QueueEmpty,
    QueueFull,
    FanError,
    StateError,
}

impl std::fmt::Display for ComError {
    
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "Com: {:?}", self)
    }
}

impl Error for ComError {
}

/// Communication module result type
pub type ComResult<T> = std::result::Result<T, ComError>;

////////////////////////////////////////////////////////////////
//
// Data elements
//
#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub enum QueuePolicy {
    OverwriteOldest,
    OverwriteNewest,
    ErrorOnFull,
}

/// Parameter packs are tuples with all elements implementing the `TypeTag + Coherent + Reloc` traits.
pub trait ParameterPack: Debug + Send + TypeTag + Coherent + Reloc + Tuple {}

/// A return value is a marker that combines the TypeTag, Coherent and Reloc traits.
pub trait ReturnValue: Debug + Send + TypeTag + Coherent + Reloc {}

////////////////////////////////////////////////////////////////
//
// Information elements
//

pub mod signal;
pub use signal::*;

pub mod variable;
pub use variable::*;

pub mod event;
pub use event::*;

pub mod fire_and_forget;
pub use fire_and_forget::*;

pub mod remote_procedure;
pub use remote_procedure::*;

////////////////////////////////////////////////////////////////
//
// The transport adapter
//

pub trait StaticConfig<A>: Debug
where
    A: Adapter + ?Sized,
{
    /// Get the class name of the adapter.
    fn name() -> &'static str;

    /// Get the vendor of the adapter
    fn vendor() -> &'static str;

    /// Get the version of the adapter.
    fn version() -> Version;
}

/// An adapter is an abstraction that connects to an underlying framework implementation
//TODO: Move to qor-core
pub trait Adapter: Debug {
    type StaticConfig: StaticConfig<Self>;

    /// Get the static configuration of the adapter.
    fn static_config<'a>(&self) -> &'a Self::StaticConfig;
}

/// A transport adapter connects to a memory provision framework.
///
/// The provision framework must be able to provide memory buffers for raw data access
/// as well a providing the fundamental communication elements like events, signals and rpcs.
///
/// All elements connected with an adapter carry the type of the adapter as a type parameter.
/// This prsignals at compile time that elements from different adapter types can be mixed up.
///
pub trait TransportAdapter: Adapter {
    /// The builder used for Signals
    type SignalBuilder: SignalBuilder<Self>;

    /// The builder used for Variables
    // type VariableBuilder<T>: VariableBuilder<Self, T>
    // where
    //     T: TypeTag + Coherent + Reloc + Send + Debug;

    /// The builder used for Events
    type EventBuilder<T>: EventBuilder<Self, T>
    where
        T: TypeTag + Coherent + Reloc + Send + Debug;

    /// The builder used for FireAndForget
    // type FireAndForgetBuilder<Args>: FireAndForgetBuilder<Self, Args>
    // where
    //     Args: ParameterPack;

    /// The builder used for Remote Procedures
    // type RemoteProcedureBuilder<Args, R>: RemoteProcedureBuilder<Self, Args, R>
    // where
    //     Args: ParameterPack,
    //     R: ReturnValue;

    /// Get an signal builder.
    ///
    /// signals signal the occurrence of a state change.
    /// They issue notifiers to emit the signal and listeners to wait for the signal.
    fn signal(&self, label: Label) -> Self::SignalBuilder;

    /// Get a variable builder for the given type.
    // fn variable<T>(&self, label: Label) -> Self::VariableBuilder<T>
    // where
    //     T: TypeTag + Coherent + Reloc + Send + Debug;

    /// Get a event builder for the given type.
    ///
    /// Events are information elements transporting values of a given type.
    /// They issue publishers that publish new values and subscribers that receive the values.
    fn event<T>(&self, label: Label) -> Self::EventBuilder<T>
    where
        T: TypeTag + Coherent + Reloc + Send + Debug;

    // Get a fire-and-forget builder for the given argument types.
    // fn fire_and_forget<Args>(&self, label: Label) -> Self::FireAndForgetBuilder<Args>
    // where
    //     Args: ParameterPack;

    // Get a remote procedure builder for the given argument and result types.
    // fn remote_procedure<Args, R>(&self, label: Label) -> Self::RemoteProcedureBuilder<Args, R>
    // where
    //     Args: ParameterPack,
    //     R: ReturnValue;
}
