// Copyright (c) 2025 Qorix GmbH
//
// This program and the accompanying materials are made available under the
// terms of the Apache License, Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0.
//
// SPDX-License-Identifier: Apache-2.0
use crate::json::JsonValue;

/// Configuration structure
#[derive(Debug, Clone)]
pub struct Config {
    value: JsonValue,
}

impl Default for Config {
    fn default() -> Self {
        Config::new()
    }
}

impl Config {
    pub fn new() -> Config {
        Config {
            value: JsonValue::Null,
        }
    }

    pub fn get(&self) -> &JsonValue {
        &self.value
    }
}

impl From<JsonValue> for Config {
    fn from(value: JsonValue) -> Self {
        Config { value }
    }
}

impl core::ops::Index<&str> for Config {
    type Output = JsonValue;
    fn index(&self, index: &str) -> &Self::Output {
        &self.value[index]
    }
}

impl core::ops::Index<usize> for Config {
    type Output = JsonValue;
    fn index(&self, index: usize) -> &Self::Output {
        &self.value[index]
    }
}

#[cfg(test)]
mod tests {
    //use super::*;
}
