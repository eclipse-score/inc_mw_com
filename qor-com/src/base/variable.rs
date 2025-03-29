// Copyright (c) 2025 Qorix GmbH
//
// This program and the accompanying materials are made available under the
// terms of the Apache License, Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0.
//
// SPDX-License-Identifier: Apache-2.0

use super::*;

use std::{fmt::Debug, ops::Deref};

////////////////////////////////////////////////////////////////
// Variables

/// A setter is a read-write access to a variable.
pub trait Setter<A, T>: Debug + Send + Deref<Target = T>
where
    A: TransportAdapter + ?Sized,
    T: Debug + Send + TypeTag + Coherent + Reloc,
{
    /// Assign a value to the field.
    fn assign(&self, value: T) -> ComResult<()>;
}

/// A getter is a read-only access to a variable.
pub trait Getter<A, T>: Debug + Send + Deref<Target = T>
where
    A: TransportAdapter + ?Sized,
    T: Debug + Send + TypeTag + Coherent + Reloc,
{
}

/// A `Variable` is a single value to be read or written.
pub trait Variable<A, T>: Debug + Send + Deref<Target = T>
where
    A: TransportAdapter + ?Sized,
    T: Debug + Send + TypeTag + Coherent + Reloc,
{
    type Setter: Setter<A, T>;
    type Getter: Getter<A, T>;

    /// Get the setter for the field.
    fn setter(&self) -> Self::Setter;

    /// Get the getter for the field.
    fn getter(&self) -> Self::Getter;
}

pub trait VariableBuilder<A, T>: Debug + Send
where
    A: TransportAdapter + ?Sized,
    T: Debug + Send + TypeTag + Coherent + Reloc,
{
    type Variable: Variable<A, T>;

    fn build(&self, value: T) -> ComResult<Self::Variable>;
}
