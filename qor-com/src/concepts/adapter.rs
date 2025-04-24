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
//! The concepts module defines the basic traits used in the qor-com stack.
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
//! - event
//! - Event
//! - Remote Procedure
//!
//! Each of the elements can produce a sending and a receiving part.
//!
//! - event: Notifier and Listener
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

use super::*;

use std::fmt::Debug;

////////////////////////////////////////////////////////////////
//
// The transport adapter
//

pub trait StaticConfigConcept<A>: Debug
where
    A: AdapterConcept + ?Sized,
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
pub trait AdapterConcept: Debug {
    type StaticConfig: StaticConfigConcept<Self>;

    /// Get the static configuration of the adapter.
    fn static_config(&self) -> &'static Self::StaticConfig;
}

/// A transport adapter connects to a memory provision framework.
///
/// The provision framework must be able to provide memory buffers for raw data access
/// as well a providing the fundamental communication elements like topics, events and rpcs.
///
/// All elements connected with an adapter carry the type of the adapter as a type parameter.
/// This prevents at compile time that elements from different adapter types can be mixed up.
///
pub trait TransportAdapterConcept: AdapterConcept {
    /// The builder used for topics
    #[cfg(feature = "signals_supported")]
    type SignalBuilder: signal::SignalBuilderConcept<Self>;

    /// The builder used for Variables
    #[cfg(feature = "variables_supported")]
    type VariableBuilder<T>: variable::Builder<Self, T>
    where
        T: TypeTag + Coherent + Reloc + Send + Debug;

    /// The builder used for topics
    #[cfg(feature = "topics_supported")]
    type TopicBuilder<T>: topic::TopicBuilderConcept<Self, T>
    where
        T: TypeTag + Coherent + Reloc + Send + Debug;

    /// The builder used for FireAndForget
    #[cfg(feature = "fire_and_forget_supported")]
    type FireAndForgetBuilder<Args>: fire_and_forget::Builder<Self, Args>
    where
        Args: Copy + Send;

    /// The builder used for remote procedures
    #[cfg(feature = "rpcs_supported")]
    type RpcBuilder<F, Args, R>: rpc::RpcBuilderConcept<Self, F, Args, R>
    where
        F: Fn(Args)->R,
        Args: Copy + Send,
        R: Send + Copy + TypeTag + Coherent + Reloc;

    /// Get a signal builder.
    ///
    /// Signals communicate the occurrence of a state change.
    #[cfg(feature = "signals_supported")]
    fn signal_builder(&self, label: Label) -> Self::SignalBuilder;

    /// Get a variable builder for the given type.
    // fn variable<T>(&self, label: Label) -> Self::VariableBuilder<T>
    // where
    //     T: TypeTag + Coherent + Reloc + Send + Debug;

    /// Get a topic builder for the given type.
    ///
    /// Topics are information elements transporting values of a given type.
    /// They issue publishers that publish new values and subscribers that receive the values.
    #[cfg(feature = "topics_supported")]
    fn topic_builder<T>(&self, label: Label) -> Self::TopicBuilder<T>
    where
        T: TypeTag + Coherent + Reloc + Send + Debug;

    /// Get a fire-and-forget builder for the given argument types.
    #[cfg(feature = "fire_and_forget_supported")]
    fn fire_and_forget_builder<Args>(&self, label: Label) -> Self::FireAndForgetBuilder<Args>
    where
        Args: Copy + Send;

    /// Get a remote procedure builder for the given argument and result types.
    #[cfg(feature = "rpcs_supported")]
    fn rpc_builder<F, Args, R>(&self, label: Label) -> Self::RpcBuilder<F, Args, R>
    where
        F: Fn(Args)->R,
        Args: Copy + Send,
        R: Send + Copy + TypeTag + Coherent + Reloc;
}

/// The IntoDynamic trait is used for adapters that have support in the `Dynamic` adapter.
#[cfg(feature = "dynamic_adapter")]
pub trait IntoDynamic<T>
where
    T: TransportAdapterConcept,
{
    /// Convert the adapter into a dynamic adapter.
    fn into_dynamic(self) -> super::adapter::dynamic::Dynamic;
}
