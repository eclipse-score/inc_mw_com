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

fn main() {
    println!("Hello, world!");
}

#[cfg(test)]
mod test {
    use com_api::{
        Builder, ConsumerDescriptor, InstanceSpecifier, Producer, SampleMaybeUninit, SampleMut,
        ServiceDiscovery, Subscriber, Subscription,
    };
    use com_api_sample_interface::{Tire, VehicleInterface};
    use std::collections::VecDeque;

    #[test]
    fn create_producer() {
        // Factory
        let runtime_builder = com_api_sample_instance::RuntimeBuilderImpl::new();
        let runtime = runtime_builder.build().unwrap();
        let producer_builder = com_api_sample_instance::RuntimeBuilderImpl::create_provided_service::<
            VehicleInterface,
        >(&runtime, InstanceSpecifier {});
        let producer = producer_builder.build().unwrap();
        let offered_producer = producer.offer().unwrap();

        // Business logic
        let uninit_sample = offered_producer.left_tire.allocate().unwrap();
        let sample = uninit_sample.write(Tire {});
        sample.send().unwrap();
    }

    #[test]
    fn create_consumer() {
        // Create runtime
        let runtime_builder = com_api_sample_instance::RuntimeBuilderImpl::new();
        let runtime = runtime_builder.build().unwrap();

        // Create service discovery
        let consumer_discovery = com_api_sample_instance::RuntimeBuilderImpl::find_service::<
            VehicleInterface,
        >(&runtime, InstanceSpecifier {});
        let available_services = consumer_discovery.get_available_instances().unwrap();

        // Create consumer from first discovered service
        let descriptor = available_services
            .into_iter()
            .find(|desc| desc.get_instance_id() == 42)
            .unwrap();
        let consumer_builder = descriptor.into_builder();
        let consumer = consumer_builder.build().unwrap();

        // Subscribe to one event
        let subscribed = consumer.left_tire.subscribe(3).unwrap();

        // Create sample buffer to be used during receive
        let mut sample_buf = Some(VecDeque::new());
        for _ in 0..10 {
            match subscribed.try_receive(sample_buf.take().unwrap(), 1) {
                (_, Ok(0)) => panic!("No sample received"),
                (mut buf, Ok(x)) => {
                    let sample = buf.pop_front().unwrap();
                    sample_buf = Some(buf); // Reuse the buffer
                    println!("{} samples received: sample[0] = {:?}", x, *sample)
                }
                (_, Err(e)) => panic!("{:?}", e),
            }
        }
    }
}
