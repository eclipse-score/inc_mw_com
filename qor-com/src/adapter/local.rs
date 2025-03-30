// Copyright (c) 2025 Qorix GmbH
//
// This program and the accompanying materials are made available under the
// terms of the Apache License, Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0.
//
// SPDX-License-Identifier: Apache-2.0

use crate::base::*;
use qor_core::prelude::*;

use std::fmt::Debug;

mod signal;
pub use signal::*;

mod event;
pub use event::*;

// mod remote_procedure;
// pub use remote_procedure::*;

//
// The TransportAdapter implementation
//

#[derive(Debug)]
pub struct LocalStaticConfig {}

impl StaticConfig<Local> for LocalStaticConfig {
    #[inline(always)]
    fn name() -> &'static str {
        "LocalAdapter"
    }

    #[inline(always)]
    fn vendor() -> &'static str {
        "Qorix GmbH"
    }

    #[inline(always)]
    fn version() -> Version {
        Version::from(env!("CARGO_PKG_VERSION"))
    }
}

#[derive(Debug)]
pub struct Local {}

impl Local {
    const CONFIG: LocalStaticConfig = LocalStaticConfig {};

    #[inline(always)]
    pub fn new() -> Self {
        Self {}
    }
}

impl Adapter for Local {
    type StaticConfig = LocalStaticConfig;

    fn static_config<'a>(&self) -> &'a Self::StaticConfig {
        &Self::CONFIG
    }
}

impl TransportAdapter for Local {
    type SignalBuilder = LocalSignalBuilder;
    type EventBuilder<T>
        = LocalEventBuilder<T>
    where
        T: Debug + Send + TypeTag + Coherent + Reloc;
    // type RemoteProcedureBuilder<Args, R>
    //     = LocalRemoteProcedureBuilder<Args, R>
    // where
    //     Args: ParameterPack,
    //     R: ReturnValue;

    /// Create a new event on the local heap
    fn signal(&self, _label: Label) -> Self::SignalBuilder {
        LocalSignalBuilder::new()
    }

    /// Create a new topic on the local heap
    fn event<T>(&self, _label: Label) -> Self::EventBuilder<T>
    where
        T: TypeTag + Coherent + Reloc + Send + Debug,
    {
        LocalEventBuilder::new()
    }

    // Create a new remote_procedure_call on the local heap
    // fn remote_procedure<Args, R>(&self, label: Label) -> Self::RemoteProcedureBuilder<Args, R>
    // where
    //     Args: ParameterPack,
    //     R: ReturnValue,
    // {
    //     LocalRemoteProcedureBuilder::new(label)
    // }
}

#[cfg(test)]
mod test {
    use std::{thread, time::Duration};
    use tracing::{info, span, Level};

    use super::*;

    #[test]
    fn trace_init() {
        tracing_subscriber::fmt::init();
    }

    #[test]
    fn test_heap_signal_threading() {
        // ADAPTER as static to avoid lifetime issues in the test thread.
        static ADAPTER: std::sync::LazyLock<Local> =
            std::sync::LazyLock::new(|| Local::new());

        let signal = ADAPTER.signal(Label::INVALID).build().unwrap();

        let emitter = signal.emitter().unwrap();
        let handle_notifier = std::thread::spawn(move || {
            let span = span!(Level::INFO, "thread_proc_notify");
            let _guard = span.enter();

            info!("start notify");
            thread::sleep(Duration::from_millis(1000));
            info!("emitting notification");
            emitter.emit();
        });

        let listener = signal.listener().unwrap();
        let handle_listener = std::thread::spawn(move || {
            let span = span!(Level::INFO, "thread_proc_listen");
            let _guard = span.enter();

            info!("start listen");
            let result = listener.wait().unwrap();
            info!("received notification");
            assert_eq!(result, true);
        });

        // wait for both threads
        handle_notifier.join().unwrap();
        handle_listener.join().unwrap();
        info!("event threads joined");
    }

    #[test]
    fn test_heap_event() {
        let adapter = Local::new();
        let event = adapter
            .event::<u32>(Label::INVALID)
            .with_queue_depth(4)
            .build()
            .unwrap();

        let publisher = event.publisher().unwrap();
        let subscriber = event.subscriber().unwrap();

        let sample = publisher.loan(42).unwrap();
        sample.send().unwrap();

        let sample = subscriber.try_receive().unwrap();
        assert_eq!(sample.is_some(), true);
        let sample = sample.unwrap();
        assert_eq!(*sample, 42);
    }

    #[derive(Debug)]
    struct MyData {
        _a: u32,
        b: u32,
    }

    impl TypeTag for MyData {
        const TYPE_TAG: Tag = Tag::new(*b"MyData__");
    }
    impl Coherent for MyData {}
    unsafe impl Reloc for MyData {}

    type Payload = MyData;

    fn thread_proc_publish(event: impl Event<Local, Payload>) {
        let span = span!(Level::INFO, "thread_proc_publish");
        let _guard = span.enter();

        info!("start publish");
        thread::sleep(Duration::from_millis(1000));

        info!("sending sample");
        let publisher = event.publisher().unwrap();

        for i in 0..4 {
            let sample = publisher.loan(Payload { _a: 42, b: i }).unwrap();
            sample.send().unwrap();
        }
    }

    fn thread_proc_subscribe(event: impl Event<Local, Payload>) -> ComResult<u32> {
        let span = span!(Level::INFO, "thread_proc_subscribe");
        let _guard = span.enter();

        info!("start subscribe");
        let subscriber = event.subscriber().unwrap();

        info!("waiting for samples");
        let mut sum = 0;
        for _ in 0..4 {
            let sample = subscriber.receive().unwrap();
            info!("received sample {:?}", *sample);
            sum += sample.b;
        }
        Ok(sum)
    }

    #[test]
    fn test_heap_topic_threading() {
        // ADAPTER as static to avoid lifetime issues in the test thread.
        static ADAPTER: std::sync::LazyLock<Local> =
            std::sync::LazyLock::new(|| Local::new());

        let event = ADAPTER
            .event::<Payload>(Label::INVALID)
            .with_queue_depth(4)
            .build()
            .unwrap();

        let event_clone = event.clone();
        let handle_publisher = std::thread::spawn(move || thread_proc_publish(event_clone));

        let event_clone = event.clone();
        let handle_subscriber = std::thread::spawn(move || thread_proc_subscribe(event_clone));

        // wait for both threads
        handle_publisher.join().unwrap();
        let result = handle_subscriber.join().unwrap();
        info!("topic threads joined");

        assert_eq!(result.is_ok(), true);
        let result = result.unwrap();
        assert_eq!(result, 0 + 1 + 2 + 3);
    }
}
