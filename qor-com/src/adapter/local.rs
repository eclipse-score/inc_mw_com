// Copyright (c) 2025 Qorix GmbH
//
// This program and the accompanying materials are made available under the
// terms of the Apache License, Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0.
//
// SPDX-License-Identifier: Apache-2.0

use qor_core::prelude::*;

use crate::concepts::*;

use std::fmt::Debug;

#[cfg(feature = "signals_supported")]
mod signal;
#[cfg(feature = "signals_supported")]
pub use signal::*;

#[cfg(feature = "topics_supported")]
mod topic;
#[cfg(feature = "topics_supported")]
pub use topic::*;

#[cfg(feature = "rpcs_supported")]
mod rpc;
#[cfg(feature = "rpcs_supported")]
pub use rpc::*;

#[cfg(feature = "dynamic_adapter")]
use super::dynamic::{Dynamic, DynamicNew};

//
// The TransportAdapter implementation
//

#[derive(Debug)]
pub struct LocalStaticConfig {}

impl adapter::StaticConfigConcept<Local> for LocalStaticConfig {
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

impl adapter::AdapterConcept for Local {
    type StaticConfig = LocalStaticConfig;

    fn static_config(&self) -> &'static Self::StaticConfig {
        &Self::CONFIG
    }
}

impl adapter::TransportAdapterConcept for Local {
    #[cfg(feature = "signals_supported")]
    type SignalBuilder = LocalEventBuilder;

    #[cfg(feature = "topics_supported")]
    type TopicBuilder<T>
        = LocalTopicBuilder<T>
    where
        T: Debug + Send + TypeTag + Coherent + Reloc;

    #[cfg(feature = "rpcs_supported")]
    type RpcBuilder<F, Args, R>
        = LocalRpcBuilder<F, Args, R>
    where
        F: Fn(Args) -> R,
        Args: Copy + Send,
        R: Send + Copy + TypeTag + Coherent + Reloc;

    /// Create a new event on the local heap
    #[cfg(feature = "signals_supported")]
    fn signal_builder(&self, _label: Label) -> Self::SignalBuilder {
        LocalEventBuilder::new()
    }

    /// Create a new topic on the local heap
    #[cfg(feature = "topics_supported")]
    fn topic_builder<T>(&self, _label: Label) -> Self::TopicBuilder<T>
    where
        T: TypeTag + Coherent + Reloc + Send + Debug,
    {
        LocalTopicBuilder::new()
    }

    /// Create a new remote_procedure_call on the local heap
    #[cfg(feature = "rpcs_supported")]
    fn rpc_builder<F, Args, R>(&self, label: Label) -> Self::RpcBuilder<F, Args, R>
    where
        F: Fn(Args) -> R,
        Args: Copy + Send,
        R: Send + Copy + TypeTag + Coherent + Reloc,
    {
        LocalRpcBuilder::new(label)
    }
}

#[cfg(feature = "dynamic_adapter")]
impl IntoDynamic<Local> for Local {
    fn into_dynamic(self) -> Dynamic {
        Dynamic::new(self)
    }
}

#[cfg(test)]
mod test {
    use super::*;
    use crate::base::*;
    
    use std::{thread, time::Duration};
    use tracing::{info, span, Level};

    #[test]
    fn trace_init() {
        tracing_subscriber::fmt::init();
    }

    #[test]
    #[cfg(feature = "signals_supported")]
    fn test_local_event_mt() {
        // ADAPTER as static to avoid lifetime issues in the test thread.
        static ADAPTER: std::sync::LazyLock<Local> = std::sync::LazyLock::new(|| Local::new());

        let event = ADAPTER.signal_builder(Label::INVALID).build().unwrap();

        let emitter = event.emitter().unwrap();
        let handle_notifier = std::thread::spawn(move || {
            let span = span!(Level::INFO, "[Local] thread_proc_emit");
            let _guard = span.enter();

            info!("[Local] start emit");
            thread::sleep(Duration::from_millis(1000));
            info!("[Local] emitting notification");
            emitter.emit();
        });

        let collector = event.collector().unwrap();
        let handle_collector = std::thread::spawn(move || {
            let span = span!(Level::INFO, "[Local] thread_proc_collect");
            let _guard = span.enter();

            info!("[Local] start collect");
            let result = collector.wait().unwrap();
            info!("[Local] received notification");
            assert_eq!(result, true);
        });

        // wait for both threads
        handle_notifier.join().unwrap();
        handle_collector.join().unwrap();
        info!("[Local] event threads joined");
    }

    #[test]
    #[cfg(feature = "signals_supported")]
    fn test_local_topic() {
        let adapter = Local::new();
        let topic = adapter
            .topic_builder::<u32>(Label::INVALID)
            .with_queue_depth(4)
            .build()
            .unwrap();

        let publisher = topic.publisher().unwrap();
        let subscriber = topic.subscriber().unwrap();

        let sample = publisher.loan(42).unwrap();
        sample.publish().unwrap();

        let sample = subscriber.try_receive().unwrap();
        assert_eq!(sample.is_some(), true);
        let sample = sample.unwrap();
        assert_eq!(*sample, 42);
    }

    #[derive(Debug)]
    #[cfg(feature = "topics_supported")]
    struct MyData {
        _a: u32,
        b: u32,
    }

    #[cfg(feature = "topics_supported")]
    impl TypeTag for MyData {
        const TYPE_TAG: Tag = Tag::new(*b"MyData__");
    }
    #[cfg(feature = "topics_supported")]
    impl Coherent for MyData {}
    #[cfg(feature = "topics_supported")]
    unsafe impl Reloc for MyData {}

    #[cfg(feature = "topics_supported")]
    type Payload = MyData;

    #[cfg(feature = "topics_supported")]
    fn thread_proc_publish(event: crate::types::Topic<Local, Payload>) {
        let span = span!(Level::INFO, "[Local] thread_proc_publish");
        let _guard = span.enter();

        info!("[Local] start publish");
        thread::sleep(Duration::from_millis(1000));

        info!("[Local] sending sample");
        let publisher = event.publisher().unwrap();

        for i in 0..4 {
            let sample = publisher.loan(Payload { _a: 42, b: i }).unwrap();
            sample.publish().unwrap();
        }
    }

    #[cfg(feature = "topics_supported")]
    fn thread_proc_subscribe(event: crate::types::Topic<Local, Payload>) -> ComResult<u32> {
        let span = span!(Level::INFO, "[Local] thread_proc_subscribe");
        let _guard = span.enter();

        info!("[Local] start subscribe");
        let subscriber = event.subscriber().unwrap();

        info!("[Local] waiting for samples");
        let mut sum = 0;
        for _ in 0..4 {
            let sample = subscriber.receive().unwrap();
            info!("[Local] received sample {:?}", *sample);
            sum += sample.b;
        }
        Ok(sum)
    }

    #[test]
    #[cfg(feature = "topics_supported")]
    fn test_local_topic_mt() {
        // ADAPTER as static to avoid lifetime issues in the test thread.
        static ADAPTER: std::sync::LazyLock<Local> = std::sync::LazyLock::new(|| Local::new());

        let topic = ADAPTER
            .topic_builder::<Payload>(Label::INVALID)
            .with_queue_depth(4)
            .build()
            .unwrap();

        let topic_clone = topic.clone();
        let handle_publisher = std::thread::spawn(move || thread_proc_publish(topic_clone));

        let topic_clone = topic.clone();
        let handle_subscriber = std::thread::spawn(move || thread_proc_subscribe(topic_clone));

        // wait for both threads
        handle_publisher.join().unwrap();
        let result = handle_subscriber.join().unwrap();
        info!("[Local] topic threads joined");

        assert_eq!(result.is_ok(), true);
        let result = result.unwrap();
        assert_eq!(result, 0 + 1 + 2 + 3);
    }

    #[test]
    fn test_local_rpc() {
        static ADAPTER: std::sync::LazyLock<Local> = std::sync::LazyLock::new(|| Local::new());

        // Create a new RPC for a function signature fn(u32)-> bool
        let rpc = ADAPTER
            .rpc_builder::<fn(u32) -> bool, _, _>(Label::INVALID)
            .build()
            .unwrap();

        // The client for the RPC
        let invoker = rpc.invoker().unwrap();

        // The service of the RPC
        let invoked = rpc.invoked().unwrap();

        // Client: Invoke with argument
        let request = invoker.loan(42).unwrap();
        let pending = request.execute().unwrap();

        // Service: Process the invokation
        invoked.receive_and_execute(|value| value > 42).unwrap();

        // Client: Wait for the result
        let response = pending.receive().unwrap();
        let result = *response;

        println!("Result: {:?}", result);
    }
}
