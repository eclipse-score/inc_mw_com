// Copyright (c) 2025 Qorix GmbH
//
// This program and the accompanying materials are made available under the
// terms of the Apache License, Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0.
//
// SPDX-License-Identifier: Apache-2.0

use qor_com::prelude::*;
use qor_core::prelude::*;

use core::time::Duration;
use std::sync::{atomic::AtomicBool, atomic::Ordering, Arc, Condvar};

const CYCLE_TIME: Duration = Duration::from_millis(100);

#[derive(Debug)]
struct Payload {
    value: u32,
    arr: [u32; 8],
}

impl TypeTag for Payload {
    const TYPE_TAG: Tag = Tag::new(*b"_Payload");
}

impl Coherent for Payload {}

unsafe impl Reloc for Payload {}

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let adapter = adapter::local::Local::new();
    let stop_signal = Arc::new((AtomicBool::new(false), Condvar::new()));

    // our event
    let event = adapter.event::<Payload>(Label::new("DemoEvent")).build()?;

    // subscriber and thread
    let subscriber = event.subscriber()?;
    let stop_clone = stop_signal.clone();
    let subscribe = std::thread::spawn(move || {
        loop {
            // check the stop signal
            if stop_clone.0.load(Ordering::Acquire) {
                break;
            }

            // wait for the event
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
    });

    // publisher and thread
    let publisher = event.publisher()?;
    let stop_clone = stop_signal.clone();
    let publish = std::thread::spawn(move || {
        let mut counter: u32 = 0;

        let mut fibo_mod: [u32; 8] = [0, 1, 1, 2, 3, 5, 8, 13];

        loop {
            // check the stop signal
            if stop_clone.0.load(Ordering::Acquire) {
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

            // emit the signal
            publisher.publish(payload).unwrap();
            println!("Publish:   payload published");

            // wait for the cycle time
            std::thread::sleep((counter % 5) * CYCLE_TIME);
        }
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
