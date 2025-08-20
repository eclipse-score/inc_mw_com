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

use com_api::*;
use com_api_runtime_lola::{
    LolaAdapter, LolaConsumerBuilder, LolaProducerBuilder, LolaPublisher, LolaSubscribable,
};

#[derive(Debug)]
pub struct Tire {}
unsafe impl Reloc for Tire {}

pub struct Exhaust {}
unsafe impl Reloc for Exhaust {}

pub struct VehicleInterface {}

/// Generic
impl InterfaceConcept for VehicleInterface {}

pub struct AnotherInterface {}

impl InterfaceConcept for AnotherInterface {}

pub struct VehicleProducer {}

impl ProducerConcept for VehicleProducer {
    type Interface = VehicleInterface;
    type OfferedProducer = VehicleOfferedProducer;

    fn offer(self) -> com_api::Result<Self::OfferedProducer> {
        todo!()
    }
}

pub struct VehicleOfferedProducer {
    pub left_tire: LolaPublisher<Tire>,
    pub exhaust: LolaPublisher<Exhaust>,
}

impl OfferedProducerConcept for VehicleOfferedProducer {
    type Interface = VehicleInterface;
    type Producer = VehicleProducer;

    fn unoffer(self) -> Self::Producer {
        VehicleProducer {}
    }
}

impl BuilderConcept<VehicleProducer> for LolaProducerBuilder<VehicleInterface> {
    fn build(self) -> com_api::Result<VehicleProducer> {
        todo!()
    }
}

impl ProducerBuilderConcept<VehicleInterface, LolaAdapter, VehicleProducer>
    for LolaProducerBuilder<VehicleInterface>
{
}

pub struct VehicleConsumer {
    pub left_tire: LolaSubscribable<Tire>,
    pub exhaust: LolaSubscribable<Exhaust>,
}

impl ConsumerConcept for VehicleConsumer {}

impl ConsumerBuilderConcept<VehicleInterface, LolaAdapter>
    for LolaConsumerBuilder<VehicleInterface>
{
}

impl BuilderConcept<VehicleConsumer> for LolaConsumerBuilder<VehicleInterface> {
    fn build(self) -> com_api::Result<VehicleConsumer> {
        todo!()
    }
}
