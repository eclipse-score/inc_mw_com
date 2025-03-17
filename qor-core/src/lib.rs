// Copyright (c) 2025 Qorix GmbH
//
// This program and the accompanying materials are made available under the
// terms of the Apache License, Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0.
//
// SPDX-License-Identifier: Apache-2.0

use std::fmt::Display;

pub mod id;

pub mod types;

pub mod component;
pub mod config;
pub mod hash;
pub mod crypto;
pub mod json;

/// The core preample publicly re-exports the most common symbols
pub mod prelude {
    pub use super::*;
    pub use component::Component;
    pub use config::Config;
    pub use id::{Id, Tag, Label};
    pub use types::*;
    pub use crypto;
}

/// Error code
/// The qor system error codes are structures error codes:
///
/// | Byte 7  | Byte 6 | Byte 5     | Byte 4   | Bytes 3..0   |
/// | ------- | ------ | ---------- | -------- | ------------ |
/// | Library | Module | Sub-module | Reserved | Error Number |
///
/// ### Library
///
/// | value | description |
/// | -- | -- |
/// | 0x00 xxxxxx xxxxxxxx | General error codes |
/// | 0x10 xxxxxx xxxxxxxx | Foundation libraries |
/// | 0x20 xxxxxx xxxxxxxx | System service libraries |
/// | 0x80 xxxxxx xxxxxxxx | User library |
///  
/// ### Foundation library modules
///
/// | value | description |
/// | -- | -- |
/// | 0x10 00 xx xxxxxxxx | Core |
/// | 0x10 10 xx xxxxxxxx | Os |
/// | 0x10 20 xx xxxxxxxx | Mem |
/// | 0x10 30 xx xxxxxxxx | Runtime |
/// | 0x10 40 xx xxxxxxxx | Com |
/// | 0x10 50 xx xxxxxxxx | Nexus |
///
pub type ErrorCode = u64;

pub mod core_errors;

/// Helper for storing static and dynamic error texts
#[derive(Debug, PartialEq)]
enum ErrorText {
    None,
    Static(&'static str),
    Dynamic(String),
}

impl Display for ErrorText {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(
            f,
            "{}",
            match self {
                Self::None => "n/a",
                Self::Static(str) => str,
                Self::Dynamic(str) => str.as_str(),
            }
        )
    }
}

/// Base class for errors
#[derive(Debug, PartialEq)]
pub struct Error {
    code: ErrorCode,
    text: ErrorText,
}

impl Error {
    #[allow(dead_code)] // provision for new function results
    pub const FUNCTION_NOT_IMPLEMENTED: Self =
        Self::const_new(core_errors::NOT_IMPLEMENTED, "Function not implemented.");

    pub const FEATURE_NOT_SUPPORTED: Self =
        Self::const_new(core_errors::UNSUPPORTED_FEATURE, "Feature not supported.");

    /// Create a new error code from a dynamic string
    pub fn new(code: ErrorCode, text: String) -> Self {
        Error {
            code,
            text: ErrorText::Dynamic(text),
        }
    }

    /// Create a new error code from a `&'static str`
    pub const fn const_new(code: ErrorCode, text: &'static str) -> Self {
        Error {
            code,
            text: ErrorText::Static(text),
        }
    }

    /// Create a new error code without an error text
    pub const fn from_code(code: ErrorCode) -> Self {
        Error {
            code,
            text: ErrorText::None,
        }
    }

    /// Get the code of an error
    pub fn code(&self) -> ErrorCode {
        self.code
    }

    /// Get the error text
    pub fn text(&self) -> &str {
        match &self.text {
            ErrorText::None => "",
            ErrorText::Static(s) => s,
            ErrorText::Dynamic(s) => s.as_str(),
        }
    }
}

impl std::fmt::Display for Error {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "{}: {}", self.code(), self.text)
    }
}

impl From<ErrorCode> for Error {
    fn from(code: ErrorCode) -> Self {
        Self::from_code(code)
    }
}

impl std::error::Error for Error {}

pub type CoreResult<T> = std::result::Result<T, Error>;

/// Semantic Version
#[derive(Debug, Clone, Copy, PartialEq, PartialOrd)]
pub struct Version {
    version: u64,
}

impl Version {
    const INVALID: u64 = u64::MAX;

    pub const MAJOR_MAX: u16 = 1023;
    pub const MINOR_MAX: u16 = 1023;
    pub const PATCH_MAX: u16 = 1023;
    pub const BUILD_MAX: u32 = u32::MAX;

    pub const fn new(major: u16, minor: u16, patch: u16, build: u32) -> Self {
        assert!(major <= Version::MAJOR_MAX);
        assert!(minor <= Version::MINOR_MAX);
        assert!(patch <= Version::PATCH_MAX);

        Version {
            version: (major as u64) << 54
                | (minor as u64) << 44
                | (patch as u64) << 34
                | build as u64,
        }
    }

    pub const fn invalid() -> Self {
        Version {
            version: Version::INVALID,
        }
    }

    pub fn is_valid(&self) -> bool {
        self.version != Version::INVALID
    }

    pub fn major(&self) -> u16 {
        (self.version >> 54) as u16 & Version::MAJOR_MAX
    }

    pub fn minor(&self) -> u16 {
        (self.version >> 54) as u16 & Version::MAJOR_MAX
    }

    pub fn patch(&self) -> u16 {
        (self.version >> 54) as u16 & Version::MAJOR_MAX
    }

    pub fn build(&self) -> u32 {
        (self.version >> 54) as u32 & Version::BUILD_MAX
    }
}

impl From<&str> for Version {
    fn from(version: &str) -> Self {
        let parts: Vec<&str> = version.split('.').collect();
        if parts.len() > 4 {
            return Version::invalid();
        }

        let major = if parts.len() >= 1 { parts[0].parse::<u16>().unwrap_or(0) } else { 0 };
        let minor = if parts.len() >= 2 { parts[1].parse::<u16>().unwrap_or(1) } else { 1 };
        let patch = if parts.len() >= 3 { parts[2].parse::<u16>().unwrap_or(0) } else { 0 };
        let build = if parts.len() >= 4 { parts[3].parse::<u32>().unwrap_or(0) } else { 0 };

        Version::new(major, minor, patch, build)
    }
}

impl std::fmt::Display for Version {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(
            f,
            "{}.{}.{} {}",
            self.major(),
            self.minor(),
            self.patch(),
            self.build()
        )
    }
}
