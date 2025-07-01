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

use com_api::{
    Builder, Consumer, ConsumerBuilder, Interface, OfferedProducer, Producer, ProducerBuilder,
    Reloc,
};
use com_api_sample_runtime::RuntimeImpl;
use com_api_sample_runtime::{SampleConsumerBuilder, SampleProducerBuilder};

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

pub struct VehicleProducer {}

impl Producer for VehicleProducer {
    type Interface = VehicleInterface;
    type OfferedProducer = VehicleOfferedProducer;

    fn offer(self) -> com_api::Result<Self::OfferedProducer> {
        todo!()
    }
}

pub struct VehicleOfferedProducer {
    pub left_tire: com_api_sample_runtime::Publisher<Tire>,
    pub exhaust: com_api_sample_runtime::Publisher<Exhaust>,
}

impl OfferedProducer for VehicleOfferedProducer {
    type Interface = VehicleInterface;
    type Producer = VehicleProducer;

    fn unoffer(self) -> Self::Producer {
        VehicleProducer {}
    }
}

impl Builder<VehicleProducer> for SampleProducerBuilder<VehicleInterface> {
    fn build(self) -> com_api::Result<VehicleProducer> {
        todo!()
    }
}

impl ProducerBuilder<VehicleInterface, RuntimeImpl, VehicleProducer>
    for SampleProducerBuilder<VehicleInterface>
{
}

pub struct VehicleConsumer {
    pub left_tire: com_api_sample_runtime::SubscribableImpl<Tire>,
    pub exhaust: com_api_sample_runtime::SubscribableImpl<Exhaust>,
}

impl Consumer for VehicleConsumer {}

impl ConsumerBuilder<VehicleInterface, RuntimeImpl> for SampleConsumerBuilder<VehicleInterface> {}

impl Builder<VehicleConsumer> for SampleConsumerBuilder<VehicleInterface> {
    fn build(self) -> com_api::Result<VehicleConsumer> {
        todo!()
    }
}
