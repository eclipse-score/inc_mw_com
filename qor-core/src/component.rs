// Copyright (c) 2025 Qorix GmbH
//
// This program and the accompanying materials are made available under the
// terms of the Apache License, Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0.
//
// SPDX-License-Identifier: Apache-2.0
use super::CoreResult;

use std::fmt;

pub mod errors {
    use crate::core_errors::CORE;
    use crate::ErrorCode;
    pub const COMPONENT_INVALID_STATE: ErrorCode = CORE + 100;
}

/// State of the component
#[derive(Debug, Clone, Copy, PartialEq)]
pub enum State {
    Created,
    Initialized,
    Ready,
    Starting,
    Running,
    Terminating,
    Terminated,
}

impl fmt::Display for State {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            State::Created => write!(f, "Created"),
            State::Initialized => write!(f, "Initialized"),
            State::Ready => write!(f, "Ready"),
            State::Starting => write!(f, "Starting"),
            State::Running => write!(f, "Running"),
            State::Terminating => write!(f, "Terminating"),
            State::Terminated => write!(f, "Terminated"),
        }
    }
}

/// A component is an element with a simple life cycle
pub trait Component {
    /// Initialize the component
    /// After this the component is ready for setup
    fn init(&mut self) -> CoreResult<State>;

    /// Setup the component
    /// The component will continue initialization
    fn setup(&mut self, config: &super::config::Config) -> CoreResult<State>;
    fn start_up(&mut self) -> CoreResult<State>;
    fn run(&mut self) -> CoreResult<State>;
    fn shut_down(&mut self) -> CoreResult<State>;

    fn class_name() -> &'static str;
    fn instance_name(&self) -> &str;

    fn state(&self) -> State;
}
