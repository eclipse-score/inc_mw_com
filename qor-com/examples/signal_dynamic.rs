// Copyright (c) 2025 Qorix GmbH
//
// This program and the accompanying materials are made available under the
// terms of the Apache License, Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0.
//
// SPDX-License-Identifier: Apache-2.0

#[allow(unused_imports)]
use qor_core::prelude::*;


#[cfg(feature = "dynamic_adapter")]
#[cfg(feature = "signals_supported")]
use qor_com::prelude::*;

#[allow(unused_imports)]
use core::time::Duration;

#[allow(unused_imports)]
use std::sync::{atomic::AtomicBool, atomic::Ordering, Arc, Condvar};

#[allow(dead_code)]
const CYCLE_TIME: Duration = Duration::from_secs(1);

/// Here is our emitter thread with Dynamic Emitter in the type agnostic version using `impl Emitter<Dynamic>` 
#[cfg(feature = "dynamic_adapter")]
#[cfg(feature = "signals_supported")]
fn emitter_thread(stop_signal: Arc<(AtomicBool, Condvar)>, emitter: impl Emitter<Dynamic>) {
    // loop until the stop signal is set
    loop {
        // check the stop signal
        if stop_signal.0.load(Ordering::Acquire) {
            break;
        }

        // emit the signal
        emitter.emit();
        println!("Signal emitted");

        // wait for the cycle time
        std::thread::sleep(CYCLE_TIME);
    }
}

/// Here is our listener thread with Dynamic Listener in the type explicit version using `DynamicSignalListener`
#[cfg(feature = "dynamic_adapter")]
#[cfg(feature = "signals_supported")]
fn listener_thread(stop_signal: Arc<(AtomicBool, Condvar)>, listener: DynamicCollector) {
    // loop until the stop signal is set
    loop {
        // check the stop signal
        if stop_signal.0.load(Ordering::Acquire) {
            break;
        }

        // wait for the signal
        match listener.wait_timeout(2 * CYCLE_TIME) {
            Ok(true) => {
                println!("Signal received");
            }
            Ok(false) => {
                println!("Signal not received");
            }
            Err(e) => {
                println!("Error: {:?}", e);
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

    // our signal (now through Dynamic)
    let signal = adapter.signal(Label::INVALID).build()?;

    // listener and thread
    let listener = signal.collector()?;
    let stop_clone = stop_signal.clone();
    let listen = std::thread::spawn(move || {
        listener_thread(stop_clone, listener);
    });

    // emitter and thread
    let emitter = signal.emitter()?;
    let stop_clone = stop_signal.clone();
    let emit = std::thread::spawn(move || {
        emitter_thread(stop_clone, emitter);
    });

    // wait for a while and stop
    std::thread::sleep(Duration::from_secs(10));
    stop_signal.0.store(true, Ordering::Release);

    // wait for the threads to finish
    emit.join().unwrap();
    listen.join().unwrap();
    println!("Threads joined");

    Ok(())
}

#[cfg(feature = "dynamic_adapter")]
#[cfg(not(feature = "events_supported"))]
fn main() {
    println!("This example requires the `signals_supported` feature to be enabled")
}

#[cfg(not(feature = "dynamic_adapter"))]
fn main() {
    println!("This example requires the `dynamic_adapter` feature to be enabled")
}