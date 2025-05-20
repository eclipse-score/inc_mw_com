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
    use com_api::{Builder, Instance, SampleMaybeUninit, SampleMut, Subscriber};
    use com_api_sample_interface::AutoInterface;
    use com_api_sample_runtime::InstanceSpecifier;
    use std::time::{Duration, SystemTime};

    #[test]
    fn create_producer() {
        // Factory
        let runtime_builder = com_api_sample_runtime::RuntimeBuilderImpl::new();
        let runtime = runtime_builder.build().unwrap();
        let instance_builder = com_api_sample_instance::InstanceBuilder::<AutoInterface>::new(
            &runtime,
            InstanceSpecifier {},
        );
        //.key_str("key", "value");
        let _producer = instance_builder.producer().build().unwrap();
        let consumer = instance_builder.consumer().build().unwrap();

        // Business logic
        match consumer
            .linkes_rad
            .receive_until(SystemTime::now() + Duration::from_secs(5))
        {
            Ok(sample) => println!("{:?}", *sample),
            Err(com_api::Error::Timeout) => panic!("No sample received"),
            Err(e) => panic!("{:?}", e),
        }
    }

    #[test]
    fn receive_stuff() {
        let test_subscriber = com_api_sample_runtime::SubscriberImpl::<u32>::new();
        for _ in 0..10 {
            match test_subscriber.receive_until(SystemTime::now() + Duration::from_secs(5)) {
                Ok(sample) => println!("{}", *sample),
                Err(com_api::Error::Timeout) => panic!("No sample received"),
                Err(e) => panic!("{:?}", e),
            }
        }
    }

    #[test]
    fn receive_async_stuff() {
        let test_subscriber = com_api_sample_runtime::SubscriberImpl::<u32>::new();
        // block on an asynchronous reception of data from test_subscriber
        futures::executor::block_on(async {
            match test_subscriber.receive().await {
                Ok(sample) => println!("{}", *sample),
                Err(e) => panic!("{:?}", e),
            }
        })
    }

    #[test]
    fn send_stuff() {
        let test_publisher = com_api_sample_runtime::Publisher::new();
        for _ in 0..5 {
            let sample = test_publisher.allocate();
            match sample {
                Ok(mut sample) => {
                    let init_sample = unsafe {
                        *sample.as_mut_ptr() = 42u32;
                        sample.assume_init_2()
                    };
                    assert!(init_sample.send().is_ok());
                }
                Err(e) => eprintln!("Oh my! {:?}", e),
            }
        }
    }

    fn is_sync<T: Sync>(_val: T) {}

    #[test]
    fn builder_is_sync() {
        is_sync(com_api_sample_runtime::RuntimeBuilderImpl::new());
    }

    #[test]
    fn build_production_runtime() {}
}
