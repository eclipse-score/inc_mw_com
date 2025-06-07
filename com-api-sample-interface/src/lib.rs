// Copyright (c) 2025 Contributors to the Eclipse Foundation
//
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
//
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// <https://www.apache.org/licenses/LICENSE-2.0>
//
// SPDX-License-Identifier: Apache-2.0

//! This is the "generated" code for an interface that looks like this (pseudo-IDL):
//!
//! ```poor-persons-idl
//! interface Vehicle {
//!     left_tire: Event<Tire>,
//!     exhaust: Event<Exhaust>,
//!     set_indicator_state: FnMut(indicator_status: IndicatorStatus) -> Result<bool>,
//! }
//!
//! interface Another {}
//!
//! ```

use com_api::{Interface, Reloc};

#[derive(Debug)]
pub struct Tire {}
unsafe impl Reloc for Tire {}

pub struct Exhaust {}
unsafe impl Reloc for Exhaust {}

pub struct VehicleInterface {}

/// Generic
impl Interface for VehicleInterface {}

pub struct AnotherInterface {}

impl Interface for AnotherInterface {}
