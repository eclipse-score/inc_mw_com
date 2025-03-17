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

fn main() -> Result<(), Box<dyn std::error::Error>> {
    static ADAPTER: std::sync::LazyLock<HeapAdapter> =
        std::sync::LazyLock::new(|| HeapAdapter::new());

    tracing_subscriber::fmt::init();

    let stop_signal = Arc::new((Mutex::new(false), Condvar::new()));

    let event = ADAPTER.event(Label::INVALID).build().unwrap();

    let notifier = event.notifier().unwrap();

    let stop_signal_clone = stop_signal.clone();
    let handle_notifier = std::thread::spawn(move || {
        let span = span!(Level::INFO, "thread_proc_notify");
        let _guard = span.enter();

        info!("start notify");
        for _i in 0..10 {
            thread::sleep(CYCLE_TIME);
            info!("sending notification");
            notifier.notify();
        }

        info!("notify stop");
        let mut lock = stop_signal_clone.0.lock().unwrap();
        *lock = true;
        stop_signal_clone.1.notify_one();
    });

    let listener = event.listener().unwrap();
    let handle_listener = std::thread::spawn(move || {
        let span = span!(Level::INFO, "thread_proc_listen");
        let _guard = span.enter();

        info!("start listen");

        while !*stop_signal.0.lock().unwrap() {
            if !listener.wait_timeout(2 * CYCLE_TIME).unwrap() {
                info!("timed out");
            } else {
                info!("received notification");
            }
        }

        info!("received stop signal");
    });

    // wait for both threads
    handle_notifier.join().unwrap();
    handle_listener.join().unwrap();
    info!("event threads joined");

    Ok(())
}
