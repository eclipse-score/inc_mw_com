use com_api::{Interface, Runtime};
use com_api_sample_runtime::InstanceSpecifier;
use std::marker::PhantomData;

pub struct InstanceBuilder<I: Interface> {
    _interface: PhantomData<I>,
    _instance_specifier: InstanceSpecifier,
}

impl<I: Interface> InstanceBuilder<I> {
    pub fn new<R: Runtime>(_runtime: &R, instance_specifier: InstanceSpecifier) -> Self {
        Self {
            _interface: PhantomData,
            _instance_specifier: instance_specifier,
        }
    }
}

impl com_api::Instance<com_api_sample_runtime::RuntimeImpl>
    for InstanceBuilder<com_api_sample_interface::AutoInterface>
{
    type Interface = com_api_sample_interface::AutoInterface;

    type ProducerBuilder = AutoProducerBuilder;
    type ConsumerBuilder = AutoConsumerBuilder;

    fn producer(&self) -> Self::ProducerBuilder {
        AutoProducerBuilder {}
    }

    fn consumer(&self) -> Self::ConsumerBuilder {
        AutoConsumerBuilder {}
    }
}

pub struct AutoProducerBuilder {}

impl com_api::Builder for AutoProducerBuilder {
    type Output = AutoProducer;

    fn build(self) -> com_api::Result<Self::Output> {
        todo!()
    }
}

impl com_api::ProducerBuilder<com_api_sample_runtime::RuntimeImpl> for AutoProducerBuilder {}

pub struct AutoConsumerBuilder {}

impl com_api::Builder for AutoConsumerBuilder {
    type Output = AutoConsumer;

    fn build(self) -> com_api::Result<Self::Output> {
        todo!()
    }
}

impl com_api::ConsumerBuilder<com_api_sample_runtime::RuntimeImpl> for AutoConsumerBuilder {}

pub struct AutoProducer {
    pub linkes_rad: com_api_sample_runtime::Publisher<com_api_sample_interface::Rad>,
    pub auspuff: com_api_sample_runtime::Publisher<com_api_sample_interface::Auspuff>,
}

pub struct AutoConsumer {
    pub linkes_rad: com_api_sample_runtime::SubscriberImpl<com_api_sample_interface::Rad>,
    pub auspuff: com_api_sample_runtime::SubscriberImpl<com_api_sample_interface::Auspuff>,
}
