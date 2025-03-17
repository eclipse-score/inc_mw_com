// Copyright (c) 2025 Qorix GmbH
//
// This program and the accompanying materials are made available under the
// terms of the Apache License, Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0.
//
// SPDX-License-Identifier: Apache-2.0
pub use tinyjson::{
    JsonGenerateError, JsonGenerateResult, JsonGenerator, JsonParseError, JsonParseResult,
    JsonParser, JsonValue,
};

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn parse_json() {
        let json_str = r#"
        {
            "name": "John",
            "age": 30,
            "is_student": false,
            "courses": ["Math", "Science"],
            "address": {
                "city": "New York",
                "zip": "10001"
            }
        }
        "#;

        let value: JsonValue = json_str.parse::<JsonValue>().unwrap();
        let v = value["name"].get::<String>().unwrap();
        assert_eq!(v.as_str(), "John");

        let json_str = r#"
        {
            "version": "0.2.0",
            "configurations": [
                {
                    "type": "lldb",
                    "request": "launch",
                    "name": "Debug executable 'json'",
                    "cargo": {
                        "args": [
                            "build",
                            "--bin=json",
                            "--package=json"
                        ],
                        "filter": {
                            "name": "json",
                            "kind": "bin"
                        }
                    },
                    "args": [],
                    "cwd": "${workspaceFolder}"
                },
                {
                    "type": "lldb",
                    "request": "launch",
                    "name": "Debug unit tests in executable 'json'",
                    "cargo": {
                        "args": [
                            "test",
                            "--no-run",
                            "--bin=json",
                            "--package=json"
                        ],
                        "filter": {
                            "name": "json",
                            "kind": "bin"
                        }
                    },
                    "args": [null],
                    "cwd": "${workspaceFolder}"
                }
            ]
        }
        "#;

        let value: JsonValue = json_str.parse().unwrap();
        let v = &value["configurations"][1]["args"][0];
        assert_eq!(v.is_null(), true);
    }
}
