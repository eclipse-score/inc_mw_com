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

use com_api::{
    Builder, Consumer, ConsumerBuilder, ConsumerDescriptor, InstanceSpecifier, Interface,
    ServiceDiscovery,
};
use com_api_sample_runtime::RuntimeImpl;

use std::marker::PhantomData;
use std::path::Path;

pub struct RuntimeBuilderImpl {}

impl Builder for RuntimeBuilderImpl {
    type Output = RuntimeImpl;
    fn build(self) -> com_api::Result<Self::Output> {
        Ok(Self::Output {})
    }
}

/// Entry point for the default implementation for the com module of s-core
impl com_api::RuntimeBuilder for RuntimeBuilderImpl {
    fn load_config(&mut self, _config: &Path) -> &mut Self {
        self
    }
}

impl RuntimeBuilderImpl {
    /// Creates a new instance of the default implementation of the com layer
    pub fn new() -> Self {
        Self {}
    }

    pub fn find_service<I: Interface>(
        runtime: &RuntimeImpl,
        instance_specifier: InstanceSpecifier,
    ) -> SampleConsumerDiscovery<I> {
        SampleConsumerDiscovery::new(runtime, instance_specifier)
    }

    pub fn create_provided_service<I: Interface>(
        runtime: &RuntimeImpl,
        instance_specifier: InstanceSpecifier,
    ) -> SampleProducerBuilder<I> {
        SampleProducerBuilder::new(runtime, instance_specifier)
    }
}

pub trait ImplementedBySample: Interface {
    type Consumer: Consumer;
}

// Generic starts here
pub struct SampleConsumerDiscovery<I> {
    _interface: PhantomData<I>,
}

impl<I> SampleConsumerDiscovery<I> {
    fn new(_runtime: &RuntimeImpl, instance_specifier: InstanceSpecifier) -> Self {
        Self {
            _interface: PhantomData,
        }
    }
}

impl<I: ImplementedBySample> ServiceDiscovery<I, RuntimeImpl> for SampleConsumerDiscovery<I> {
    type ConsumerDescriptor = SampleConsumerDescriptor<I>;
    type ServiceEnumerator = Vec<SampleConsumerDescriptor<I>>;

    fn get_available_instances(&self) -> com_api::Result<Self::ServiceEnumerator> {
        Ok(Vec::new())
    }
}

pub struct SampleProducerBuilder<I: Interface> {
    instance_specifier: InstanceSpecifier,
    _interface: PhantomData<I>,
}

impl<I: Interface> SampleProducerBuilder<I> {
    fn new(_runtime: &RuntimeImpl, instance_specifier: InstanceSpecifier) -> Self {
        Self {
            instance_specifier,
            _interface: PhantomData,
        }
    }
}

pub struct SampleConsumerDescriptor<I: Interface> {
    _interface: PhantomData<I>,
}

impl<I: Interface> Clone for SampleConsumerDescriptor<I> {
    fn clone(&self) -> Self {
        Self {
            _interface: PhantomData,
        }
    }
}

impl<I: ImplementedBySample> ConsumerDescriptor<I, RuntimeImpl> for SampleConsumerDescriptor<I> {
    type ConsumerBuilder = SampleConsumerBuilder<I>;

    fn get_instance_id(&self) -> usize {
        todo!()
    }

    fn into_builder(self) -> Self::ConsumerBuilder {
        todo!()
    }
}

pub struct SampleConsumerBuilder<I: Interface> {
    instance_specifier: InstanceSpecifier,
    _interface: PhantomData<I>,
}

impl<I: ImplementedBySample> ConsumerBuilder<I, RuntimeImpl> for SampleConsumerBuilder<I> {}

impl<I: ImplementedBySample> Builder for SampleConsumerBuilder<I> {
    type Output = I::Consumer;

    fn build(self) -> com_api::Result<Self::Output> {
        todo!()
    }
}

mod generated {
    use crate::{ImplementedBySample, SampleProducerBuilder};
    use com_api::{Builder, Consumer, OfferedProducer, Producer, ProducerBuilder};
    use com_api_sample_interface::VehicleInterface;
    use com_api_sample_runtime::RuntimeImpl;

    // Generated starts here
    impl ImplementedBySample for VehicleInterface {
        type Consumer = VehicleConsumer;
    }

    pub struct VehicleProducer {}

    impl Producer for VehicleProducer {
        type Interface = VehicleInterface;
        type OfferedProducer = VehicleOfferedProducer;

        fn offer(self) -> com_api::Result<Self::OfferedProducer> {
            todo!()
        }
    }

    pub struct VehicleOfferedProducer {
        pub left_tire: com_api_sample_runtime::Publisher<com_api_sample_interface::Tire>,
        pub exhaust: com_api_sample_runtime::Publisher<com_api_sample_interface::Exhaust>,
    }

    impl OfferedProducer for VehicleOfferedProducer {
        type Interface = VehicleInterface;
        type Producer = VehicleProducer;

        fn unoffer(self) -> Self::Producer {
            VehicleProducer {}
        }
    }

    pub struct VehicleConsumer {
        pub left_tire: com_api_sample_runtime::SubscribableImpl<com_api_sample_interface::Tire>,
        pub exhaust: com_api_sample_runtime::SubscribableImpl<com_api_sample_interface::Exhaust>,
    }

    impl Builder for SampleProducerBuilder<VehicleInterface> {
        type Output = VehicleProducer;

        fn build(self) -> com_api::Result<Self::Output> {
            todo!()
        }
    }

    impl ProducerBuilder<VehicleInterface, RuntimeImpl> for SampleProducerBuilder<VehicleInterface> {}

    impl Consumer for VehicleConsumer {}
}
