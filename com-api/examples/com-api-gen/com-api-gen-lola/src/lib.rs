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

use com_api::{Consumer, Interface, OfferedProducer, Producer, Reloc, Runtime, Subscriber};

#[derive(Debug)]
pub struct Tire {}
unsafe impl Reloc for Tire {}

pub struct Exhaust {}
unsafe impl Reloc for Exhaust {}

pub struct VehicleInterface {}

/// Generic
impl Interface for VehicleInterface {
    type Consumer<R: Runtime + ?Sized> = VehicleConsumer<R>;
    type Producer<R: Runtime + ?Sized> = VehicleProducer<R>;
}

pub struct VehicleConsumer<R: Runtime + ?Sized> {
    pub left_tire: R::Subscriber<Tire>,
    pub exhaust: R::Subscriber<Exhaust>,
}

impl<R: Runtime + ?Sized> Consumer<R> for VehicleConsumer<R> {
    fn new(instance_info: R::ConsumerInfo) -> Self {
        VehicleConsumer {
            left_tire: R::Subscriber::new("left_tire", instance_info.clone()),
            exhaust: R::Subscriber::new("exhaust", instance_info.clone()),
        }
    }
}

pub struct AnotherInterface {}

pub struct VehicleProducer<R: Runtime + ?Sized>
{
    _runtime: std::marker::PhantomData<R>,
}

impl<R: Runtime + ?Sized> Producer<R> for VehicleProducer<R> {
    type Interface = VehicleInterface;
    type OfferedProducer = VehicleOfferedProducer<R>;

    fn offer(self) -> com_api::Result<Self::OfferedProducer> {
        todo!()
    }
}

pub struct VehicleOfferedProducer<R: Runtime + ?Sized> {
    pub left_tire: R::Publisher<Tire>,
    pub exhaust: R::Publisher<Exhaust>,
}

impl<R: Runtime + ?Sized> OfferedProducer<R> for VehicleOfferedProducer<R> {
    type Interface = VehicleInterface;
    type Producer = VehicleProducer<R>;

    fn unoffer(self) -> Self::Producer {
        VehicleProducer {
            _runtime: std::marker::PhantomData,
        }
    }
}
