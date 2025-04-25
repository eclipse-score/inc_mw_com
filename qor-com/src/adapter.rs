// Copyright (c) 2025 Qorix GmbH
//
// This program and the accompanying materials are made available under the
// terms of the Apache License, Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0.
//
// SPDX-License-Identifier: Apache-2.0

/// The local in-process transport adapter
pub mod local;
pub use local::Local as LocalAdapter;

#[cfg(feature = "dynamic_adapter")]
pub mod dynamic;
#[cfg(feature = "dynamic_adapter")]
pub use dynamic::Dynamic as DynamicAdapter;