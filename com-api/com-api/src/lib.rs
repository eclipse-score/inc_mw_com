// Copyright (c) 2025 Contributors to the Eclipse Foundation
//
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
//
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// <https://www.apache.org/licenses/LICENSE-2.0>
//
// SPDX-License-Identifier: Apache-2.0

#[cfg(not(any(feature = "iceoryx", feature = "lola")))]
compile_error!("You must enable at least one feature: `iceoryx` or `lola`!");

#[cfg(feature = "iceoryx")]
pub use com_api_runtime_iceoryx::RuntimeBuilderImpl;
#[cfg(feature = "lola")]
pub use com_api_runtime_lola::RuntimeBuilderImpl;

pub use com_api_concept::{
    Builder, Consumer, ConsumerBuilder, ConsumerDescriptor, InstanceSpecifier, Interface,
    OfferedProducer, Producer, ProducerBuilder, Reloc, Result, SampleContainer, SampleMaybeUninit,
    SampleMut, ServiceDiscovery, Subscriber, Subscription,
};
