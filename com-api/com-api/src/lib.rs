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

//! This crate provides the COM API, which is a common interface for different implementations
//! of the COM API, e.g., for different IPC backends.
//! The actual implementations are provided by the `com-api-runtime-mock` and `com-api-runtime-lola` crates.
//! The user must enable one of these features to use the COM API.

#[cfg(not(any(feature = "mock", feature = "lola")))]
compile_error!("You must enable at least one feature: `mock` or `lola`!");

#[cfg(feature = "lola")]
pub use com_api_runtime_lola::LolaAdapter;
#[cfg(feature = "lola")]
pub use com_api_runtime_lola::LolaAdapterBuilder;
#[cfg(feature = "mock")]
pub use com_api_runtime_mock::MockAdapter;
#[cfg(feature = "mock")]
pub use com_api_runtime_mock::MockAdapterBuilder;

pub use com_api_concept::{
    BuilderConcept, ConsumerBuilderConcept, ConsumerConcept, ConsumerDescriptorConcept,
    InstanceSpecifier, InterfaceConcept, OfferedProducerConcept, ProducerBuilderConcept,
    ProducerConcept, Reloc, Result, SampleConcept, SampleContainer, SampleMaybeUninitConcept,
    SampleMutConcept, ServiceDiscoveryConcept, SubscriberConcept, SubscriptionConcept,
};
