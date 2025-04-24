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
//! - Event
//! - Topic
//! - Remote Procedure
//!
//! Each of the elements can produce a sending and a receiving part.
//!
//! - Event: Emitter and Receiver
//! - Topic: Publisher and Subscriber
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
use super::base::*;

////////////////////////////////////////////////////////////////
//
// Information elements
//

#[cfg(feature = "signals_supported")]
pub mod signal;
#[cfg(feature = "signals_supported")]
pub use signal::*;

#[cfg(feature = "variables_supported")]
pub mod variable;
#[cfg(feature = "variables_supported")]
pub use variable::*;

#[cfg(feature = "topics_supported")]
pub mod topic;
#[cfg(feature = "topics_supported")]
pub use topic::*;

#[cfg(feature = "fire_and_forget_supported")]
pub mod fire_and_forget;
#[cfg(feature = "fire_and_forget_supported")]
pub use fire_and_forget::*;

#[cfg(feature = "rpcs_supported")]
pub mod rpc;
#[cfg(feature = "rpcs_supported")]
pub use rpc::*;

pub mod adapter;
pub use adapter::*;