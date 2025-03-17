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
use std::{
    sync::{Arc, Condvar, Mutex},
    thread,
};

use tracing::{info, span, Level};

const CYCLE_TIME: Duration = Duration::from_secs(1);

#[derive(Debug)]
struct StructuredData {
    a: u32,
    b: u32,
}

impl TypeTag for StructuredData {
    const TYPE_TAG: Tag = Tag::new(*b"MyData__");
}
impl Coherent for StructuredData {}
unsafe impl Reloc for StructuredData {}

type Payload = StructuredData;

fn thread_proc_publish<'s, 't>(
    topic: impl Topic<'s, 't, HeapAdapter<'t>, Payload>,
    stop_signal: Arc<(Mutex<bool>, Condvar)>,
) where
    't: 's,
{
    let span = span!(Level::INFO, "thread_proc_publish");
    let _guard = span.enter();

    info!("start publish. Preparing...");
    thread::sleep(5*CYCLE_TIME);

    info!("publishing samples");
    let publisher = topic.publisher().unwrap();

    for i in 0..10 {
        let sample = publisher.loan_with(Payload { a: 42, b: i }).unwrap();

        thread::sleep(CYCLE_TIME);

        info!("publish sample");
        sample.publish().unwrap();
    }

    let mut lock = stop_signal.0.lock().unwrap();
    *lock = true;
    stop_signal.1.notify_one();
}

fn thread_proc_subscribe<'s, 't>(
    topic: impl Topic<'s, 't, HeapAdapter<'t>, Payload>,
    stop_signal: Arc<(Mutex<bool>, Condvar)>,
) -> ComResult<u32>
where
    't: 's,
{
    let span = span!(Level::INFO, "thread_proc_subscribe");
    let _guard = span.enter();

    info!("start subscribe");
    let subscriber = topic.subscriber().unwrap();
    let mut sum = 0;

    info!("waiting for samples");
    while !*stop_signal.0.lock().unwrap() {
        let sample = subscriber.receive_timeout(2 * CYCLE_TIME);
        match sample {
            Err(_) => {
                info!("Timeout");
            }
            Ok(sample) => {
                info!("received sample {:?}", *sample);
                sum += sample.b;
            }
        }
    }

    Ok(sum)
}

fn main() -> Result<(), Box<dyn std::error::Error>> {
    static ADAPTER: std::sync::LazyLock<HeapAdapter> =
        std::sync::LazyLock::new(|| HeapAdapter::new());

    tracing_subscriber::fmt::init();

    let stop_signal = Arc::new((Mutex::new(false), Condvar::new()));

    let topic = ADAPTER
        .topic::<Payload>(Label::INVALID)
        .with_queue_depth(4)
        .build()
        .unwrap();

    let topic_clone = topic.clone();
    let stop_signal_clone = stop_signal.clone();
    let handle_publisher =
        std::thread::spawn(move || thread_proc_publish(topic_clone, stop_signal_clone));

    let topic_clone = topic.clone();
    let stop_signal_clone = stop_signal.clone();
    let handle_subscriber =
        std::thread::spawn(move || thread_proc_subscribe(topic_clone, stop_signal_clone));

    // wait for both threads
    handle_publisher.join().unwrap();
    let result = handle_subscriber.join().unwrap();
    info!("topic threads joined. Sum[.b] = {:?}", result);

    Ok(())
}
