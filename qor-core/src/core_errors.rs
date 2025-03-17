// Copyright (c) 2025 Qorix GmbH
//
// This program and the accompanying materials are made available under the
// terms of the Apache License, Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0.
//
// SPDX-License-Identifier: Apache-2.0

use super::ErrorCode;

#[allow(dead_code)]
pub const OK: ErrorCode = 0x00000000;

pub const CORE: ErrorCode = 0x10000000;
pub const OS: ErrorCode = 0x10100000;
pub const MEM: ErrorCode = 0x10200000;
pub const RTO: ErrorCode = 0x10300000;
pub const COM: ErrorCode = 0x10400000;
pub const NEXUS: ErrorCode = 0x10500000;

#[allow(dead_code)] // System Services are outside this lib
pub const SYS: ErrorCode = 0x20000000;

#[allow(dead_code)] // User Applications are outside this lib
pub const USER: ErrorCode = 0x80000000;

#[allow(dead_code)] // provision for new function results
/// Function is not implemented yet
pub const NOT_IMPLEMENTED: ErrorCode = CORE + 1;
pub const DIVISION_BY_ZERO: ErrorCode = CORE + 10;
pub const ARITHMETIC_UNDERFLOW: ErrorCode = CORE + 11;
pub const ARITHMETIC_OVERFLOW: ErrorCode = CORE + 12;

/// The operation ended with a timeout
pub const TIMEOUT: ErrorCode = CORE + 20;

/// The requested feature is not supported.
pub const UNSUPPORTED_FEATURE: ErrorCode = CORE + 100;

/// Id passed is not valid.
pub const INVALID_ID: ErrorCode = CORE + 1000;

/// Tag passed is not valid.
pub const INVALID_TAG: ErrorCode = CORE + 1001;

/// A required lock could not be acquired.
pub const LOCK_ERROR: ErrorCode = CORE + 1100;

/// The collection is full.
pub const COLLECTION_FULL: ErrorCode = CORE + 1200;

/// The collection is empty.
pub const COLLECTION_EMPTY: ErrorCode = CORE + 1201;

/// Error during parsing of a string or stream.
pub const PARSE_ERROR: ErrorCode = CORE + 2000;
