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

pub use com_api_runtime_lola::LolaRuntimeImpl;
pub use com_api_runtime_lola::RuntimeBuilderImpl as LolaRuntimeBuilderImpl;
pub use com_api_runtime_mock::MockRuntimeImpl;
pub use com_api_runtime_mock::RuntimeBuilderImpl as MockRuntimeBuilderImpl;

pub use com_api_concept::{
    Builder, Consumer, ConsumerBuilder, ConsumerDescriptor, Error, FindServiceSpecifier,
    InstanceSpecifier, Interface, OfferedProducer, PlacementDefault, Producer, ProducerBuilder,
    Publisher, Reloc, Result, Runtime, SampleContainer, SampleMaybeUninit, SampleMut,
    ServiceDiscovery, Subscriber, Subscription,
};
