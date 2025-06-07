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
    Builder, ConsumerBuilder, ConsumerDescriptor, InstanceSpecifier, Interface, ServiceDiscovery,
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

// Type needs to be defined in the generate crate.
// Reason: impl ServiceDiscovery references SampleConsumerDescriptor, causing a cyclic dependency
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

impl<I: Interface> ServiceDiscovery<I, RuntimeImpl> for SampleConsumerDiscovery<I>
where
    SampleConsumerDescriptor<I>: ConsumerDescriptor<I, RuntimeImpl>,
{
    type ConsumerDescriptor = SampleConsumerDescriptor<I>;
    type ServiceEnumerator = Vec<SampleConsumerDescriptor<I>>;

    fn get_available_instances(&self) -> com_api::Result<Self::ServiceEnumerator> {
        Ok(Vec::new())
    }
}

// Type needs to be defined in the generate crate.
// Reason: Violation of orphan rule.
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

// Type needs to be defined in the generate crate.
// Reason: impl ConsumerDescriptor references SampleConsumerBuilder, causing a cyclic dependency
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

impl<I: Interface> ConsumerDescriptor<I, RuntimeImpl> for SampleConsumerDescriptor<I>
where
    SampleConsumerBuilder<I>: ConsumerBuilder<I, RuntimeImpl>,
{
    type ConsumerBuilder = SampleConsumerBuilder<I>;

    fn get_instance_id(&self) -> usize {
        todo!()
    }

    fn into_builder(self) -> Self::ConsumerBuilder {
        todo!()
    }
}

// Type needs to be defined in the generate crate.
// Reason: Violation of orphan rule.
pub struct SampleConsumerBuilder<I: Interface> {
    instance_specifier: InstanceSpecifier,
    _interface: PhantomData<I>,
}

mod generated {
    use crate::{SampleConsumerBuilder, SampleProducerBuilder};
    use com_api::{Builder, Consumer, ConsumerBuilder, OfferedProducer, Producer, ProducerBuilder};
    use com_api_sample_interface::VehicleInterface;
    use com_api_sample_runtime::RuntimeImpl;

    // Generated starts here

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

    impl Builder for SampleProducerBuilder<VehicleInterface> {
        type Output = VehicleProducer;

        fn build(self) -> com_api::Result<Self::Output> {
            todo!()
        }
    }

    impl ProducerBuilder<VehicleInterface, RuntimeImpl> for SampleProducerBuilder<VehicleInterface> {}
    pub struct VehicleConsumer {
        pub left_tire: com_api_sample_runtime::SubscribableImpl<com_api_sample_interface::Tire>,
        pub exhaust: com_api_sample_runtime::SubscribableImpl<com_api_sample_interface::Exhaust>,
    }

    impl Consumer for VehicleConsumer {}

    impl ConsumerBuilder<VehicleInterface, RuntimeImpl> for SampleConsumerBuilder<VehicleInterface> {}

    impl Builder for SampleConsumerBuilder<VehicleInterface> {
        type Output = VehicleConsumer;

        fn build(self) -> com_api::Result<Self::Output> {
            todo!()
        }
    }
}
