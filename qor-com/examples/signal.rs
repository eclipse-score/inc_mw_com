// Copyright (c) 2025 Qorix GmbH
//
// This program and the accompanying materials are made available under the
// terms of the Apache License, Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0.
//
// SPDX-License-Identifier: Apache-2.0

use qor_core::prelude::*;
use qor_com::prelude::*;

use core::time::Duration;
use std::sync::{atomic::AtomicBool, atomic::Ordering, Arc, Condvar};

const CYCLE_TIME: Duration = Duration::from_secs(1);

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let adapter = adapter::local::Local::new();
    let stop_signal = Arc::new((AtomicBool::new(false), Condvar::new()));

    // our signal
    let signal = adapter.signal(Label::INVALID).build()?;

    // listener and thread
    let listener = signal.listener()?;
    let stop_clone = stop_signal.clone();
    let listen = std::thread::spawn(move || {
        loop {
            // check the stop signal
            if stop_clone.0.load(Ordering::Acquire) {
                break;
            }

            // wait for the signal
            match listener.wait_timeout(2* CYCLE_TIME) {
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
    });

    // emitter and thread
    let emitter = signal.emitter()?;
    let stop_clone = stop_signal.clone();
    let emit = std::thread::spawn(move || {
        loop {
            // check the stop signal
            if stop_clone.0.load(Ordering::Acquire) {
                break;
            }

            // emit the signal
            emitter.emit();
            println!("Signal emitted");

            // wait for the cycle time
            std::thread::sleep(CYCLE_TIME);
        }
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
