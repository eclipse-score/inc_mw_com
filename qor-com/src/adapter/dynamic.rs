// Copyright (c) 2025 Qorix GmbH
//
// This program and the accompanying materials are made available under the
// terms of the Apache License, Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0.
//
// SPDX-License-Identifier: Apache-2.0

use crate::base::*;
use qor_core::prelude::*;

use std::{fmt::Debug, sync::Arc};

#[cfg(feature = "signals_supported")]
mod signal;
#[cfg(feature = "signals_supported")]
pub use signal::*;

#[cfg(feature = "signals_supported")]
mod event;
#[cfg(feature = "signals_supported")]
pub use event::*;

#[cfg(feature = "rpcs_supported")]
mod remote_procedure;
#[cfg(feature = "rpcs_supported")]
pub use remote_procedure::*;

//
// The TransportAdapter implementation
//

#[derive(Debug)]
pub struct DynamicStaticConfig {}

impl StaticConfig<Dynamic> for DynamicStaticConfig {
    #[inline(always)]
    fn name() -> &'static str {
        "DynamicAdapter"
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

/// The dynamic transport adapter selector
#[derive(Debug)]
pub enum AdapterSelector {
    Local(super::local::Local),
}

/// Allow the transport specific creation of the dynamic transport adapter
///
/// `Dynamic` will implement this for every supported transport adapter.
/// The supported adapter should implement the `IntoDynamic` trait to allow the conversion.
pub trait DynamicNew<T: TransportAdapter> {
    /// Create a new dynamic transport adapter
    fn new(inner: T) -> Dynamic;
}

/// A dynamic transport adapter for late binding. To guarantee type safety in Communication, most types have distinct generic arguments.
/// Hence type erasure is not possible because this would require either dyn objects or dynamic typing.
/// The support for async operations prevents the use of dyn objects as those require Futures with compile-time sizes.
/// Dynamic typing is not possible in Rust.
///
/// We therefore choose an explicit dynamic binding approach. This utilizes enumerations to represent the different adapters.
/// Unfortunately, this requires us to know the supported adapters a priori at compile time.
///
#[derive(Debug)]
pub struct Dynamic {
    inner: AdapterSelector,
}

impl DynamicNew<super::local::Local> for Dynamic {
    /// Create a new dynamic transport adapter
    fn new(inner: super::local::Local) -> Dynamic {
        Dynamic {
            inner: AdapterSelector::Local(inner),
        }
    }
}

impl Dynamic {
    const CONFIG: DynamicStaticConfig = DynamicStaticConfig {};

    /// Get read access to the inner adapter
    #[inline(always)]
    pub fn adapter(&self) -> &AdapterSelector {
        &self.inner
    }
}

impl Adapter for Dynamic {
    type StaticConfig = DynamicStaticConfig;

    fn static_config(&self) -> &'static Self::StaticConfig {
        &Self::CONFIG
    }
}

impl TransportAdapter for Dynamic {
    #[cfg(feature = "signals_supported")]
    type SignalBuilder = DynamicSignalBuilder;

    #[cfg(feature = "signals_supported")]
    type EventBuilder<T>
        = DynamicEventBuilder<T>
    where
        T: Debug + Send + TypeTag + Coherent + Reloc;

    #[cfg(feature = "rpcs_supported")]
    type RemoteProcedureBuilder<Args, R>
        = DynamicRemoteProcedureBuilder<Args, R>
    where
        Args: Copy + Send,
        R: Send + Copy + TypeTag + Coherent + Reloc;

    /// Create a new event on the local heap
    #[cfg(feature = "signals_supported")]
    fn signal(&self, label: Label) -> Self::SignalBuilder {
        match &self.inner {
            AdapterSelector::Local(inner) => {
                let builder = inner.signal(label);
                DynamicSignalBuilder::new(SignalBuilderSelector::Local(builder))
            }
        }
    }

    /// Create a new topic on the local heap
    #[cfg(feature = "signals_supported")]   
    fn event<T>(&self, label: Label) -> Self::EventBuilder<T>
    where
        T: TypeTag + Coherent + Reloc + Send + Debug,
    {
        match &self.inner {
            AdapterSelector::Local(inner) => {
                let builder = inner.event::<T>(label);
                DynamicEventBuilder::new(EventBuilderSelector::Local(builder))
            }
        }
    }

    /// Create a new remote_procedure_call on the local heap
    #[cfg(feature = "rpcs_supported")]   
    fn remote_procedure<Args, R>(&self, label: Label) -> Self::RemoteProcedureBuilder<Args, R>
    where
        Args: Copy + Send,
        R: Send + Copy + TypeTag + Coherent + Reloc,
    {
        DynamicRemoteProcedureBuilder::new(label)
    }
}

#[cfg(test)]
mod test {
    use std::{thread, time::Duration};
    use tracing::{info, span, Level};

    use super::*;
    use crate::adapter::dynamic::{Dynamic, DynamicNew};
    use crate::adapter::local::Local;


    #[test]
    #[cfg(feature = "signals_supported")]   
    fn test_dynamic_signal_threading() {
        // ADAPTER as static to avoid lifetime issues in the test thread.
        static ADAPTER: std::sync::LazyLock<Dynamic> =
            std::sync::LazyLock::new(|| Dynamic::new(Local::new()));

        let signal = ADAPTER.signal(Label::INVALID).build().unwrap();

        let emitter = signal.emitter().unwrap();
        let handle_notifier = std::thread::spawn(move || {
            let span = span!(Level::INFO, "[Dynamic] thread_proc_notify");
            let _guard = span.enter();

            info!("[Dynamic] start notify");
            thread::sleep(Duration::from_millis(1000));
            info!("[Dynamic] emitting notification");
            emitter.emit();
        });

        let listener = signal.collector().unwrap();
        let handle_listener = std::thread::spawn(move || {
            let span = span!(Level::INFO, "[Dynamic] thread_proc_listen");
            let _guard = span.enter();

            info!("[Dynamic] start listen");
            let result = listener.wait().unwrap();
            info!("[Dynamic] received notification");
            assert_eq!(result, true);
        });

        // wait for both threads
        handle_notifier.join().unwrap();
        handle_listener.join().unwrap();
        info!("[Dynamic] event threads joined");
    }

    #[test]
    #[cfg(feature = "signals_supported")]   
    fn test_dynamic_event() {
        let adapter = Dynamic::new(Local::new());
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

    #[cfg(feature = "signals_supported")]
    fn thread_proc_publish(event: impl Event<Dynamic, Payload>) {
        let span = span!(Level::INFO, "[Dynamic] thread_proc_publish");
        let _guard = span.enter();

        info!("[Dynamic] start publish");
        thread::sleep(Duration::from_millis(1000));

        info!("[Dynamic] sending sample");
        let publisher = event.publisher().unwrap();

        for i in 0..4 {
            let sample = publisher.loan(Payload { _a: 42, b: i }).unwrap();
            sample.send().unwrap();
        }
    }

    #[cfg(feature = "signals_supported")]
    fn thread_proc_subscribe(event: impl Event<Dynamic, Payload>) -> ComResult<u32> {
        let span = span!(Level::INFO, "[Dynamic] thread_proc_subscribe");
        let _guard = span.enter();

        info!("[Dynamic] start subscribe");
        let subscriber = event.subscriber().unwrap();

        info!("[Dynamic] waiting for samples");
        let mut sum = 0;
        for _ in 0..4 {
            let sample = subscriber.receive().unwrap();
            info!("[Dynamic] received sample {:?}", *sample);
            sum += sample.b;
        }
        Ok(sum)
    }

    #[test]
    #[cfg(feature = "signals_supported")]
    fn test_dynamic_event_threading() {
        // ADAPTER as static to avoid lifetime issues in the test thread.
        static ADAPTER: std::sync::LazyLock<Dynamic> =
            std::sync::LazyLock::new(|| Dynamic::new(Local::new()));

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
        info!("[Dynamic] topic threads joined");

        assert_eq!(result.is_ok(), true);
        let result = result.unwrap();
        assert_eq!(result, 0 + 1 + 2 + 3);
    }
}
