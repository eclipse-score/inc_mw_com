//! This is the "generated" code for an interface that looks like this (pseudo-IDL):
//!
//! ```poor-mans-idl
//! interface Auto {
//!     linkes_rad: Event<Rad>,
//!     auspuff: Event<Auspuff>,
//!     set_blinker_zustand: FnMut(blinkmodus: BlinkModus) -> Result<bool>,
//! }
//! ```

use crate::*;

pub(super) struct Rad {}
unsafe impl Reloc for Rad {}

pub(super) struct Auspuff {}
unsafe impl Reloc for Auspuff {}

pub(super) struct AutoInterface {}

/// Generic
impl Interface for AutoInterface {}

// Specific code for the "sample" runtime
// TODO: VIOLATES ORPHAN RULES, IMPOSSIBLE OUTSIDE CRATE!!11
impl Instance<sample_impl::RuntimeImpl> for sample_impl::SampleInstanceImpl<AutoInterface> {
    type ProducerBuilder = AutoProducerBuilder;
    type ConsumerBuilder = AutoConsumerBuilder;

    fn producer(&self) -> Self::ProducerBuilder {
        AutoProducerBuilder {}
    }

    fn consumer(&self) -> Self::ConsumerBuilder {
        AutoConsumerBuilder {}
    }
}

pub(super) struct AutoProducerBuilder {}

impl Builder for AutoProducerBuilder {
    type Output = AutoProducer;

    fn build(self) -> Result<Self::Output> {
        todo!()
    }
}

impl ProducerBuilder<sample_impl::RuntimeImpl> for AutoProducerBuilder {}

pub(super) struct AutoConsumerBuilder {}

impl Builder for AutoConsumerBuilder {
    type Output = AutoConsumer;

    fn build(self) -> Result<Self::Output> {
        todo!()
    }
}

impl ConsumerBuilder<sample_impl::RuntimeImpl> for AutoConsumerBuilder {}

pub(super) struct AutoProducer {
    pub linkes_rad: sample_impl::Publisher<Rad>,
    pub auspuff: sample_impl::Publisher<Auspuff>,
}

pub(super) struct AutoConsumer {
    pub linkes_rad: sample_impl::SubscriberImpl<Rad>,
    pub auspuff: sample_impl::SubscriberImpl<Auspuff>,
}
