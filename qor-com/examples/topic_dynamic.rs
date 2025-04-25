// Copyright (c) 2025 Qorix GmbH
//
// This program and the accompanying materials are made available under the
// terms of the Apache License, Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0.
//
// SPDX-License-Identifier: Apache-2.0

#[allow(unused_imports)]
use qor_com::prelude::*;

#[allow(unused_imports)]
use qor_core::prelude::*;

use core::time::Duration;
#[allow(unused_imports)]
use std::{
    fmt::Debug,
    sync::{atomic::AtomicBool, atomic::Ordering, Arc, Condvar},
};

#[allow(dead_code)]
const CYCLE_TIME: Duration = Duration::from_millis(100);

#[derive(Debug)]
#[cfg(feature = "dynamic_adapter")]
#[cfg(feature = "signals_supported")]
struct Payload {
    value: u32,
    arr: [u32; 8],
}

#[cfg(feature = "dynamic_adapter")]
#[cfg(feature = "signals_supported")]
impl TypeTag for Payload {
    const TYPE_TAG: Tag = Tag::new(*b"_Payload");
}

#[cfg(feature = "dynamic_adapter")]
#[cfg(feature = "signals_supported")]
impl Coherent for Payload {}

#[cfg(feature = "dynamic_adapter")]
#[cfg(feature = "signals_supported")]
unsafe impl Reloc for Payload {}

/// Here is our publisher thread with Dynamic Publisher in the type agnostic version using `impl Publisher<Dynamic, Payload>`
#[cfg(feature = "dynamic_adapter")]
#[cfg(feature = "signals_supported")]
fn publisher_thread(
    stop_signal: Arc<(AtomicBool, Condvar)>,
    publisher: TopicPublisher<Dynamic, Payload>,
) {
    // loop until the stop signal is set
    let mut counter: u32 = 0;

    let mut fibo_mod: [u32; 8] = [0, 1, 1, 2, 3, 5, 8, 13];

    loop {
        // check the stop signal
        if stop_signal.0.load(Ordering::Acquire) {
            break;
        }

        let payload = Payload {
            value: counter,
            arr: fibo_mod,
        };
        counter += 1;

        // modify the Fibonacci sequence
        for i in 0..8 {
            fibo_mod[i] = (fibo_mod[(i + 6) % 8] + fibo_mod[(i + 7) % 8]) % 65536;
        }

        // publish the payload
        publisher.publish(payload).unwrap();
        println!("Publish:   payload published");

        // wait for the cycle time
        std::thread::sleep((counter % 5) * CYCLE_TIME);
    }
}

/// Here is our subscriber thread with Dynamic Subscriber in the type explicit version using `DynamicSubscriber<Payload>`
#[cfg(feature = "dynamic_adapter")]
#[cfg(feature = "signals_supported")]
fn subscriber_thread(
    stop_signal: Arc<(AtomicBool, Condvar)>,
    subscriber: TopicSubscriber<Dynamic, Payload>,
) {
    // loop until the stop signal is set
    loop {
        // check the stop signal
        if stop_signal.0.load(Ordering::Acquire) {
            break;
        }

        // wait for a sample
        match subscriber.receive_timeout(10 * CYCLE_TIME) {
            Ok(sample) => {
                println!(
                    "Subscribe: Ok: value: {}, arr: {:?}",
                    sample.value, sample.arr
                );
            }
            Err(e) => {
                println!("Subscribe: Error: {:?}", e);
            }
        }
    }
}

#[cfg(feature = "dynamic_adapter")]
#[cfg(feature = "signals_supported")]
fn main() -> Result<(), Box<dyn std::error::Error>> {
    // build the actual adapter
    let inner_adapter = adapter::local::Local::new();

    // wrap the actual transport adapter into a dynamic adapter
    let adapter = Dynamic::new(inner_adapter);
    let stop_signal = Arc::new((AtomicBool::new(false), Condvar::new()));

    // our event (now through Dynamic)
    let topic = adapter.topic_builder::<Payload>(Label::new("DemoEvent")).build()?;

    // subscriber and thread
    let subscriber = topic.subscriber()?;
    let stop_clone = stop_signal.clone();
    let subscribe = std::thread::spawn(move || {
        subscriber_thread(stop_clone, subscriber);
    });

    // publisher and thread
    let publisher = topic.publisher()?;
    let stop_clone = stop_signal.clone();
    let publish = std::thread::spawn(move || {
        publisher_thread(stop_clone, publisher);
    });

    // wait for a while and stop
    std::thread::sleep(Duration::from_secs(10));
    stop_signal.0.store(true, Ordering::Release);

    // wait for the threads to finish
    publish.join().unwrap();
    subscribe.join().unwrap();
    println!("Threads joined");

    Ok(())
}

#[cfg(feature = "dynamic_adapter")]
#[cfg(not(feature = "signals_supported"))]
fn main() {
    println!("This example requires the `signals_supported` feature to be enabled")
}

#[cfg(not(feature = "dynamic_adapter"))]
fn main() {
    println!("This example requires the `dynamic_adapter` feature to be enabled")
}
