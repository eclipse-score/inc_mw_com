/********************************************************************************
* Copyright (c) 2025 Contributors to the Eclipse Foundation
*
* See the NOTICE file(s) distributed with this work for additional
* information regarding copyright ownership.
*
* This program and the accompanying materials are made available under the
* terms of the Apache License Version 2.0 which is available at
* https://www.apache.org/licenses/LICENSE-2.0
*
* SPDX-License-Identifier: Apache-2.0
********************************************************************************/


#include "platform/aas/mw/com/impl/tracing/configuration/tracing_filter_config_parser.h"

#include "platform/aas/mw/com/impl/configuration/config_parser.h"

#include "gmock/gmock.h"
#include <gtest/gtest.h>

#include <memory>

namespace bmw
{
namespace mw
{
namespace com
{
namespace impl
{
namespace tracing
{
namespace
{

using bmw::json::operator""_json;

const char* small_mw_com_config_ok = R"(
{
"serviceTypes": [
  {
    "serviceTypeName": "/bmw/ncar/services/TirePressureService",
    "version": {
      "major": 12,
      "minor": 34
    },
    "bindings": [
      {
        "binding": "SHM",
        "serviceId": 1234,
        "events": [
          {
            "eventName": "CurrentPressureFrontLeft",
            "eventId": 20
          }
        ],
        "fields": [
          {
            "fieldName": "CurrentTemperatureFrontLeft",
            "fieldId": 30
          }
        ]
      }
    ]
  }
],
"serviceInstances": [
  {
    "instanceSpecifier": "abc/abc/TirePressurePort",
    "serviceTypeName": "/bmw/ncar/services/TirePressureService",
    "version": {
      "major": 12,
      "minor": 34
    },
    "instances": [
      {
        "instanceId": 1234,
        "asil-level": "QM",
        "binding": "SHM",
        "shm-size": 10000,
        "events": [
          {
            "eventName": "CurrentPressureFrontLeft",
            "numberOfSampleSlots": 50,
            "maxSubscribers": 5,
            "enableIpcTracing": true
          }
        ],
        "fields": [
          {
            "fieldName": "CurrentTemperatureFrontLeft",
            "numberOfSampleSlots": 60,
            "maxSubscribers": 6,
            "enableIpcTracing": true
          }
        ],
      }
    ]
  }
],
"tracing": {
  "enable": true,
  "applicationInstanceID": "ara_com_example",
}
}
)";

class TraceConfigParserFixture : public ::testing::Test
{
  public:
    void SetUp() override
    {
        const bmw::json::JsonParser json_parser;
        auto json_result = json_parser.FromBuffer(small_mw_com_config_ok);
        EXPECT_TRUE(json_result.has_value());
        config_ =
            std::make_unique<Configuration>(bmw::mw::com::impl::configuration::Parse(std::move(json_result).value()));
    }

  protected:
    void expectAllEventTracePoints(const TracingFilterConfig& tracing_filter_config,
                                   const char* event_name,
                                   bool enabled)
    {
        const char* instance_specifier{"abc/abc/TirePressurePort"};
        const tracing::ProxyEventTracePointType expected_enabled_proxy_event_trace_points[] = {
            tracing::ProxyEventTracePointType::SUBSCRIBE,
            tracing::ProxyEventTracePointType::UNSUBSCRIBE,
            tracing::ProxyEventTracePointType::SUBSCRIBE_STATE_CHANGE,
            tracing::ProxyEventTracePointType::SET_SUBSCRIPTION_STATE_CHANGE_HANDLER,
            tracing::ProxyEventTracePointType::UNSET_SUBSCRIPTION_STATE_CHANGE_HANDLER,
            tracing::ProxyEventTracePointType::SUBSCRIPTION_STATE_CHANGE_HANDLER_CALLBACK,
            tracing::ProxyEventTracePointType::SET_RECEIVE_HANDLER,
            tracing::ProxyEventTracePointType::UNSET_RECEIVE_HANDLER,
            tracing::ProxyEventTracePointType::RECEIVE_HANDLER_CALLBACK,
            tracing::ProxyEventTracePointType::GET_NEW_SAMPLES,
            tracing::ProxyEventTracePointType::GET_NEW_SAMPLES_CALLBACK};
        const tracing::SkeletonEventTracePointType expected_enabled_skeleton_event_trace_points[] = {
            tracing::SkeletonEventTracePointType::SEND, tracing::SkeletonEventTracePointType::SEND_WITH_ALLOCATE};

        for (auto trace_point : expected_enabled_proxy_event_trace_points)
        {
            EXPECT_EQ(tracing_filter_config.IsTracePointEnabled(
                          service_type_name_, event_name, instance_specifier, trace_point),
                      enabled);
        }
        for (auto trace_point : expected_enabled_skeleton_event_trace_points)
        {
            EXPECT_EQ(tracing_filter_config.IsTracePointEnabled(
                          service_type_name_, event_name, instance_specifier, trace_point),
                      enabled);
        }
    }

    void expectAllFieldTracePoints(const TracingFilterConfig& tracing_filter_config,
                                   const char* field_name,
                                   bool enabled)
    {
        const char* instance_specifier{"abc/abc/TirePressurePort"};
        const tracing::ProxyFieldTracePointType expected_enabled_proxy_field_trace_points[] = {
            tracing::ProxyFieldTracePointType::SUBSCRIBE,
            tracing::ProxyFieldTracePointType::UNSUBSCRIBE,
            tracing::ProxyFieldTracePointType::SUBSCRIBE_STATE_CHANGE,
            tracing::ProxyFieldTracePointType::SET_SUBSCRIPTION_STATE_CHANGE_HANDLER,
            tracing::ProxyFieldTracePointType::UNSET_SUBSCRIPTION_STATE_CHANGE_HANDLER,
            tracing::ProxyFieldTracePointType::SUBSCRIPTION_STATE_CHANGE_HANDLER_CALLBACK,
            tracing::ProxyFieldTracePointType::SET_RECEIVE_HANDLER,
            tracing::ProxyFieldTracePointType::UNSET_RECEIVE_HANDLER,
            tracing::ProxyFieldTracePointType::RECEIVE_HANDLER_CALLBACK,
            tracing::ProxyFieldTracePointType::GET_NEW_SAMPLES,
            tracing::ProxyFieldTracePointType::GET_NEW_SAMPLES_CALLBACK,
            tracing::ProxyFieldTracePointType::GET,
            tracing::ProxyFieldTracePointType::GET_RESULT,
            tracing::ProxyFieldTracePointType::SET,
            tracing::ProxyFieldTracePointType::SET_RESULT};
        const tracing::SkeletonFieldTracePointType expected_enabled_skeleton_field_trace_points[] = {
            tracing::SkeletonFieldTracePointType::UPDATE, tracing::SkeletonFieldTracePointType::UPDATE_WITH_ALLOCATE};

        for (auto trace_point : expected_enabled_proxy_field_trace_points)
        {
            EXPECT_EQ(tracing_filter_config.IsTracePointEnabled(
                          service_type_name_, field_name, instance_specifier, trace_point),
                      enabled);
        }
        for (auto trace_point : expected_enabled_skeleton_field_trace_points)
        {
            EXPECT_EQ(tracing_filter_config.IsTracePointEnabled(
                          service_type_name_, field_name, instance_specifier, trace_point),
                      enabled);
        }
    }

    std::unique_ptr<Configuration> config_;
    static constexpr auto service_type_name_ = "/bmw/ncar/services/TirePressureService";
};

TEST_F(TraceConfigParserFixture, FilterConfigOK)
{
    RecordProperty("Verifies", "9, 5, 8");
    RecordProperty("Description",
                   "Checks whether format of Trace Filter Config is correctly parsed (9) and the event "
                   "(5) and field (8) specific <enableIpcTracing> properties. ");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("Priority", "1");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    // Given a tracing filter configuration, which enables tracing for all trace-point-types of the
    // service elements configured, which are "ipcTracing enabled"  in the underlying mw_com_config.json
    // (TraceConfigParserFixture::config_)

    auto filter_config_json = R"(
{
  "services": [
    {
      "shortname_path": "/bmw/ncar/services/TirePressureService",
      "events": [
        {
          "shortname": "CurrentPressureFrontLeft",
          "trace_subscribe_send": true,
          "trace_subscribe_received": true,
          "trace_unsubscribe_send": true,
          "trace_unsubscribe_received": true,
          "trace_subscription_state_changed": true,
          "trace_subscription_state_change_handler_registered": true,
          "trace_subscription_state_change_handler_deregistered": true,
          "trace_subscription_state_change_handler_callback": true,
          "trace_send": true,
          "trace_send_allocate": true,
          "trace_get_new_samples": true,
          "trace_get_new_samples_callback": true,
          "trace_receive_handler_registered": true,
          "trace_receive_handler_deregistered": true,
          "trace_receive_handler_callback": true
        },
      ],
      "fields": [
        {
          "shortname": "CurrentTemperatureFrontLeft",
          "notifier": {
            "trace_subscribe_send": true,
            "trace_subscribe_received": true,
            "trace_unsubscribe_send": true,
            "trace_unsubscribe_received": true,
            "trace_subscription_state_changed": true,
            "trace_subscription_state_change_handler_registered": true,
            "trace_subscription_state_change_handler_deregistered": true,
            "trace_subscription_state_change_handler_callback": true,
            "trace_update": true,
            "trace_get_new_samples": true,
            "trace_get_new_samples_callback": true,
            "trace_receive_handler_registered": true,
            "trace_receive_handler_deregistered": true,
            "trace_receive_handler_callback": true
          },
          "getter": {
            "trace_request_send": true,
            "trace_request_received": true,
            "trace_response_send": true,
            "trace_response_received": true,
            "trace_get_handler_registered": true,
            "trace_get_handler_completed": true
          },
          "setter": {
            "trace_request_send": true,
            "trace_request_received": true,
            "trace_response_send": true,
            "trace_response_received": true,
            "trace_set_handler_registered": true,
            "trace_set_handler_completed": true
          }
        }
      ]
    }
  ]
}
)"_json;

    // when parsing the given tracing filter config
    auto result = Parse(std::move(filter_config_json), *config_);
    // expect, that there is no error
    EXPECT_TRUE(result.has_value());

    // and expect, that the trace-points enabled in the json file are all reflected as true/enabled in the returned
    // TracingFilterConfig
    TracingFilterConfig tracing_filter_config = std::move(result).value();
    expectAllEventTracePoints(tracing_filter_config, "CurrentPressureFrontLeft", true);
    expectAllFieldTracePoints(tracing_filter_config, "CurrentTemperatureFrontLeft", true);
}

TEST_F(TraceConfigParserFixture, FilterConfigOKFromFile)
{
    RecordProperty("Verifies", "4, 9, 5, 8");
    RecordProperty("Description",
                   "Checks whether format of Trace Filter Config is correctly parsed (9) from file "
                   "(4) and the event (5) and field (8) specific <enableIpcTracing> "
                   "properties. ");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("Priority", "1");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    // given a LoLa/mw::com config with an event Event_1 for which <enableIpcTracing> has been enabled.
    auto config_event_trace_enabled = R"(
{
"serviceTypes": [
  {
    "serviceTypeName": "/bmw/ncar/services/TirePressureService",
    "version": {
      "major": 12,
      "minor": 34
    },
    "bindings": [
      {
        "binding": "SHM",
        "serviceId": 1234,
        "events": [
          {
            "eventName": "Event_1",
            "eventId": 20
          }
        ]
      }
    ]
  }
],
"serviceInstances": [
  {
    "instanceSpecifier": "abc/abc/TirePressurePort",
    "serviceTypeName": "/bmw/ncar/services/TirePressureService",
    "version": {
      "major": 12,
      "minor": 34
    },
    "instances": [
      {
        "instanceId": 1234,
        "asil-level": "QM",
        "binding": "SHM",
        "shm-size": 10000,
        "events": [
          {
            "eventName": "Event_1",
            "numberOfSampleSlots": 50,
            "maxSubscribers": 5,
            "enableIpcTracing": true
          }
        ],
      }
    ]
  }
],
"tracing": {
  "enable": true,
  "applicationInstanceID": "ara_com_example",
}
}
)"_json;

    auto config = configuration::Parse(std::move(config_event_trace_enabled));

    // Given a tracing filter configuration under a given path, which enables tracing for all trace-point-types of the
    // Event_1

    // when parsing the given tracing filter config
    auto result =
        Parse("platform/aas/mw/com/impl/tracing/configuration/example/comtrace_filter_config_small.json", config);
    // expect, that there is no error
    ASSERT_TRUE(result.has_value());

    // and expect, that the trace-points enabled in the json file are all reflected as true/enabled in the returned
    // TracingFilterConfig
    TracingFilterConfig tracing_filter_config = std::move(result).value();
    expectAllEventTracePoints(tracing_filter_config, "Event_1", true);
}

TEST_F(TraceConfigParserFixture, FilterConfigJsonError)
{
    RecordProperty("Verifies", "3");
    RecordProperty("Description",
                   "Checks whether a broken json-format leads to an Error return. Note: We are NOT doing a real "
                   "schema-validation during runtime. We try to understand the content best-effort!");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("Priority", "1");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    // given a LoLa/mw::com config with an event Event_1 for which <enableIpcTracing> has been enabled.
    auto config_event_trace_enabled = R"(
{
"serviceTypes": [
  {
    "serviceTypeName": "/bmw/ncar/services/TirePressureService",
    "version": {
      "major": 12,
      "minor": 34
    },
    "bindings": [
      {
        "binding": "SHM",
        "serviceId": 1234,
        "events": [
          {
            "eventName": "Event_1",
            "eventId": 20
          }
        ]
      }
    ]
  }
],
"serviceInstances": [
  {
    "instanceSpecifier": "abc/abc/TirePressurePort",
    "serviceTypeName": "/bmw/ncar/services/TirePressureService",
    "version": {
      "major": 12,
      "minor": 34
    },
    "instances": [
      {
        "instanceId": 1234,
        "asil-level": "QM",
        "binding": "SHM",
        "shm-size": 10000,
        "events": [
          {
            "eventName": "Event_1",
            "numberOfSampleSlots": 50,
            "maxSubscribers": 5,
            "enableIpcTracing": true
          }
        ],
      }
    ]
  }
],
"tracing": {
  "enable": true,
  "applicationInstanceID": "ara_com_example",
}
}
)"_json;

    auto config = configuration::Parse(std::move(config_event_trace_enabled));

    // Given a tracing filter configuration under a given path, which is syntactically broken, so that already the
    // json parser refuses it.

    // when parsing the given tracing filter config
    auto result =
        Parse("platform/aas/mw/com/impl/tracing/configuration/example/comtrace_filter_config_broken.json", config);
    // expect, that there is an error
    EXPECT_FALSE(result.has_value());
}

TEST_F(TraceConfigParserFixture, IgnoreTracePointReferencingUnknownServiceType)
{
    RecordProperty("Verifies", "8");
    RecordProperty("Description",
                   "Checks whether references from tracing filter config to trace-points for service elements, which "
                   "do not exist in mw::com/LoLa are ignored.");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("Priority", "1");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    // Given a tracing filter configuration, which enables tracing for a service /bmw/ncar/services/UNKNOWN, which
    // doesn't exist in the underlying mw_com_config.json (TraceConfigParserFixture::config_)

    auto filter_config_json = R"(
{
  "services": [
    {
      "shortname_path": "/bmw/ncar/services/UNKNOWN",
      "events": [
        {
          "shortname": "CurrentPressureFrontLeft",
          "trace_subscribe_send": true,
        },
      ],
      "fields": [
        {
          "shortname": "CurrentTemperatureFrontLeft",
          "notifier": {
            "trace_subscribe_send": true,
          },
        }
      ]
    }
  ]
}
)"_json;

    // when parsing the given tracing filter config
    auto result = Parse(std::move(filter_config_json), *config_);
    // expect, that there is no error
    EXPECT_TRUE(result.has_value());

    // and expect, that no trace-points have been enabled for mw::com/LoLa supported trace-points
    TracingFilterConfig tracing_filter_config = std::move(result).value();
    expectAllEventTracePoints(tracing_filter_config, "CurrentPressureFrontLeft", false);
    expectAllFieldTracePoints(tracing_filter_config, "CurrentTemperatureFrontLeft", false);
}

TEST_F(TraceConfigParserFixture, IgnoreTracePointReferencingUnknownEventField)
{
    RecordProperty("Verifies", "8");
    RecordProperty("Description",
                   "Checks whether references from tracing filter config to trace-points for service elements, "
                   "which do not exist in mw::com/LoLa are ignored.");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("Priority", "1");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    // Given a tracing filter configuration, which enables tracing for an unknown event and unknown field in a known
    // service /bmw/ncar/services/TirePressureService, which doesn't exist in the underlying mw_com_config.json
    // (TraceConfigParserFixture::config_)

    auto filter_config_json = R"(
{
  "services": [
    {
      "shortname_path": "/bmw/ncar/services/TirePressureService",
      "events": [
        {
          "shortname": "UnknownEvent",
          "trace_subscribe_send": true,
        },
      ],
      "fields": [
        {
          "shortname": "UnknownField",
          "notifier": {
            "trace_subscribe_send": true,
          },
        }
      ]
    }
  ]
}
)"_json;

    // when parsing the given tracing filter config
    auto result = Parse(std::move(filter_config_json), *config_);
    // expect, that there is no error
    EXPECT_TRUE(result.has_value());

    // and expect, that no trace-points have been enabled for mw::com/LoLa supported trace-points
    TracingFilterConfig tracing_filter_config = std::move(result).value();
    expectAllEventTracePoints(tracing_filter_config, "CurrentPressureFrontLeft", false);
    expectAllFieldTracePoints(tracing_filter_config, "CurrentTemperatureFrontLeft", false);
}

/// \brief This test verifies, that a specific trace-point, which has been activated/enabled in the trace-filter-config
///        for an event/field, for which tracing has been disabled in the mw::com/LoLa config, will not lead to
///        corresponding enabling in the returned TracingFilterConfig AND that a warning message is logged.
/// \attention the verification of the warning message expects, that the message is logged to stdout! The implementation
///            writes the warning message via mw::log. We expect, that in the context of the unit test there is no
///            configuration for mw::log existing, which leads to stdout output! Whenever this changes, this test has to
///            be adapted!
TEST_F(TraceConfigParserFixture, IgnoreTracePointForDisabledEventWithWarning)
{
    RecordProperty("Verifies", "4, 5, 7");
    RecordProperty("Description",
                   "Checks whether an activated trace-point in the tracing filter config for a service element, "
                   "for which tracing has been disabled explicitly (5) or implicitly (7) in "
                   "mw::com/LoLa are ignored, but lead to a WARN log message.");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("Priority", "1");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    // given a LoLa/mw::com config with an event for which <enableIpcTracing> has been disabled/set to false.
    auto config_event_trace_disabled = R"(
{
"serviceTypes": [
  {
    "serviceTypeName": "/bmw/ncar/services/TirePressureService",
    "version": {
      "major": 12,
      "minor": 34
    },
    "bindings": [
      {
        "binding": "SHM",
        "serviceId": 1234,
        "events": [
          {
            "eventName": "CurrentPressureFrontLeft",
            "eventId": 20
          },
          {
            "eventName": "CurrentPressureFrontRight",
            "eventId": 21
          }
        ]
      }
    ]
  }
],
"serviceInstances": [
  {
    "instanceSpecifier": "abc/abc/TirePressurePort",
    "serviceTypeName": "/bmw/ncar/services/TirePressureService",
    "version": {
      "major": 12,
      "minor": 34
    },
    "instances": [
      {
        "instanceId": 1234,
        "asil-level": "QM",
        "binding": "SHM",
        "shm-size": 10000,
        "events": [
          {
            "eventName": "CurrentPressureFrontLeft",
            "numberOfSampleSlots": 50,
            "maxSubscribers": 5,
            "enableIpcTracing": false
          },
          {
            "eventName": "CurrentPressureFrontRight",
            "numberOfSampleSlots": 50,
            "maxSubscribers": 5,
          }
        ],
      }
    ]
  }
],
"tracing": {
  "enable": true,
  "applicationInstanceID": "ara_com_example",
}
}
)"_json;

    auto config = configuration::Parse(std::move(config_event_trace_disabled));

    // and given a tracing filter config, which enables a trace-point for events, where <enableIpcTracing> has been
    // disabled/set to false
    auto filter_config_json = R"(
{
  "services": [
    {
      "shortname_path": "/bmw/ncar/services/TirePressureService",
      "events": [
        {
          "shortname": "CurrentPressureFrontLeft",
          "trace_subscribe_send": true,
        },
        {
          "shortname": "CurrentPressureFrontRight",
          "trace_subscribe_send": true,
        },
      ]
    }
  ]
}
)"_json;

    // capture stdout output during Parse() call.
    testing::internal::CaptureStdout();
    // when parsing the given tracing filter config
    auto result = Parse(std::move(filter_config_json), config);
    // stop capture and get captured data.
    std::string log_output = testing::internal::GetCapturedStdout();
    // expect, that there is no error
    EXPECT_TRUE(result.has_value());

    // and expect, that no trace-points have been enabled for mw::com/LoLa supported trace-points
    TracingFilterConfig tracing_filter_config = std::move(result).value();
    expectAllEventTracePoints(tracing_filter_config, "CurrentPressureFrontLeft", false);
    expectAllEventTracePoints(tracing_filter_config, "CurrentPressureFrontRight", false);

    const char log_warn_snippet[] = "log warn";
    const char text_snippet[] = "has been disabled in mw_com_config but";

    // and expect, that the output contains two warning messages (mw::log)
    auto first_offset = log_output.find(log_warn_snippet);
    EXPECT_TRUE(first_offset != log_output.npos);
    EXPECT_TRUE(log_output.find(log_warn_snippet, first_offset) != log_output.npos);
    // and the following snippet, which is part of the warn message
    first_offset = log_output.find(text_snippet);
    EXPECT_TRUE(first_offset != log_output.npos);
    EXPECT_TRUE(log_output.find(text_snippet, first_offset) != log_output.npos);
}

/// \brief Test resembles test "IgnoreTracePointForDisabledFieldWithWarning", but with fields instead of events.
/// \attention See test "IgnoreTracePointForDisabledFieldWithWarning"
TEST_F(TraceConfigParserFixture, IgnoreTracePointForDisabledFieldWithWarning)
{
    RecordProperty("Verifies", "4, 8, 5");
    RecordProperty("Description",
                   "Checks whether an activated trace-point in the tracing filter config for a service element, "
                   "for which tracing has been disabled explicitly (8) or implicitly (5) in "
                   "mw::com/LoLa are ignored, but lead to a WARN log message.");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("Priority", "1");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    // given a LoLa/mw::com config with an event for which <enableIpcTracing> has been disabled/set to false.
    auto config_event_trace_disabled = R"(
{
"serviceTypes": [
  {
    "serviceTypeName": "/bmw/ncar/services/TirePressureService",
    "version": {
      "major": 12,
      "minor": 34
    },
    "bindings": [
      {
        "binding": "SHM",
        "serviceId": 1234,
        "fields": [
           {
             "fieldName": "CurrentTemperatureFrontLeft",
             "fieldId": 30
           },
           {
             "fieldName": "CurrentTemperatureFrontRight",
             "fieldId": 31
           }
        ]
      }
    ]
  }
],
"serviceInstances": [
  {
    "instanceSpecifier": "abc/abc/TirePressurePort",
    "serviceTypeName": "/bmw/ncar/services/TirePressureService",
    "version": {
      "major": 12,
      "minor": 34
    },
    "instances": [
      {
        "instanceId": 1234,
        "asil-level": "QM",
        "binding": "SHM",
        "shm-size": 10000,
        "fields": [
          {
            "fieldName": "CurrentTemperatureFrontLeft",
            "numberOfSampleSlots": 50,
            "maxSubscribers": 5,
            "enableIpcTracing": false
          },
          {
            "fieldName": "CurrentTemperatureFrontRight",
            "numberOfSampleSlots": 50,
            "maxSubscribers": 5,
          }
        ],
      }
    ]
  }
],
"tracing": {
  "enable": true,
  "applicationInstanceID": "ara_com_example",
}
}
)"_json;

    auto config = configuration::Parse(std::move(config_event_trace_disabled));

    // and given a tracing filter config, which enables a trace-point for fields, where <enableIpcTracing> has been
    // disabled/set to false
    auto filter_config_json = R"(
{
  "services": [
    {
      "shortname_path": "/bmw/ncar/services/TirePressureService",
      "fields": [
        {
          "shortname": "CurrentTemperatureFrontLeft",
          "notifier": {
             "trace_subscribe_send": true,
          }
        },
        {
          "shortname": "CurrentTemperatureFrontRight",
          "notifier": {
             "trace_subscribe_send": true,
          }
        },
      ]
    }
  ]
}
)"_json;

    // capture stdout output during Parse() call.
    testing::internal::CaptureStdout();
    // when parsing the given tracing filter config
    auto result = Parse(std::move(filter_config_json), config);
    // stop capture and get captured data.
    std::string log_output = testing::internal::GetCapturedStdout();
    // expect, that there is no error
    EXPECT_TRUE(result.has_value());

    // and expect, that no trace-points have been enabled for mw::com/LoLa supported trace-points
    TracingFilterConfig tracing_filter_config = std::move(result).value();
    expectAllFieldTracePoints(tracing_filter_config, "CurrentTemperatureFrontLeft", false);
    expectAllFieldTracePoints(tracing_filter_config, "CurrentTemperatureFrontRight", false);

    const char log_warn_snippet[] = "log warn";
    const char text_snippet[] = "has been disabled in mw_com_config but";

    // and expect, that the output contains two warning messages (mw::log)
    auto first_offset = log_output.find(log_warn_snippet);
    EXPECT_TRUE(first_offset != log_output.npos);
    EXPECT_TRUE(log_output.find(log_warn_snippet, first_offset) != log_output.npos);
    // and the following snippet, which is part of the warn message
    first_offset = log_output.find(text_snippet);
    EXPECT_TRUE(first_offset != log_output.npos);
    EXPECT_TRUE(log_output.find(text_snippet, first_offset) != log_output.npos);
}

/// \todo Test that can be removed when support for these tracing points is added in 
TEST_F(TraceConfigParserFixture, IgnoreTracePointForTemporarilyDisabledTracePointsWithWarning)
{

    // Given a tracing filter configuration, which enables tracing for all trace-point-types of the
    // service elements configured, which are "ipcTracing enabled"  in the underlying mw_com_config.json
    // (TraceConfigParserFixture::config_)

    auto filter_config_json = R"(
{
  "services": [
    {
      "shortname_path": "/bmw/ncar/services/TirePressureService",
      "events": [
        {
          "shortname": "CurrentPressureFrontLeft",
          "trace_subscribe_received": true,
          "trace_unsubscribe_received": true,
        },
      ],
      "fields": [
        {
          "shortname": "CurrentTemperatureFrontLeft",
          "notifier": {
            "trace_subscribe_received": true,
            "trace_unsubscribe_received": true,
          }
        }
      ]
    }
  ]
}
)"_json;

    // capture stdout output during Parse() call.
    testing::internal::CaptureStdout();
    // when parsing the given tracing filter config
    auto result = Parse(std::move(filter_config_json), *config_);

    // stop capture and get captured data.
    std::string log_output = testing::internal::GetCapturedStdout();
    // expect, that there is no error
    EXPECT_TRUE(result.has_value());

    // and expect, that no trace-points have been enabled for mw::com/LoLa supported trace-points
    TracingFilterConfig tracing_filter_config = std::move(result).value();
    expectAllFieldTracePoints(tracing_filter_config, "CurrentTemperatureFrontLeft", false);
    expectAllFieldTracePoints(tracing_filter_config, "CurrentTemperatureFrontRight", false);

    const char log_warn_snippet[] = "log warn";
    const char text_snippet_0[] = "Event Tracing point: trace_subscribe_received is currently unsupported";
    const char text_snippet_1[] = "Event Tracing point: trace_unsubscribe_received is currently unsupported";
    const char text_snippet_2[] = "Field Tracing point: trace_subscribe_received is currently unsupported";
    const char text_snippet_3[] = "Field Tracing point: trace_unsubscribe_received is currently unsupported";

    auto check_test_snipped =
        [&log_output, &log_warn_snippet](
            const auto text_snippet) {  // and expect, that the output contains two warning messages (mw::log)
            auto first_offset = log_output.find(log_warn_snippet);
            EXPECT_TRUE(first_offset != log_output.npos);
            EXPECT_TRUE(log_output.find(log_warn_snippet, first_offset) != log_output.npos);
            // and the following snippet, which is part of the warn message
            first_offset = log_output.find(text_snippet);
            EXPECT_TRUE(first_offset != log_output.npos);
            EXPECT_TRUE(log_output.find(text_snippet, first_offset) != log_output.npos);
        };
    check_test_snipped(text_snippet_0);
    check_test_snipped(text_snippet_1);
    check_test_snipped(text_snippet_2);
    check_test_snipped(text_snippet_3);
}

}  // namespace
}  // namespace tracing
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
