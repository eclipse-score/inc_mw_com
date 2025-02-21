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


#include "platform/aas/mw/com/impl/configuration/configuration.h"

#include "platform/aas/mw/com/impl/configuration/config_parser.h"

#include "platform/aas/lib/json/json_writer.h"

#include <amp_optional.hpp>
#include <amp_overload.hpp>

#include <gtest/gtest.h>
#include <utility>
#include <vector>

namespace bmw
{
namespace mw
{
namespace com
{
namespace impl
{
namespace
{

class ConfigurationFixture : public ::testing::Test
{
  public:
    void SetUp() override {}

    void TearDown() override {}

    void PrepareMinimalConfiguration()
    {
        auto service_type_deployment = ServiceTypeDeployment{amp::blank{}};
        auto lola = LolaServiceInstanceDeployment{LolaServiceInstanceId{1U}};
        auto serviceIdentifier = make_ServiceIdentifierType("/bla/blub/one", 1U, 2U);
        auto instance_specifier_result = InstanceSpecifier::Create("/bla/blub/instance_specifier");
        ASSERT_TRUE(instance_specifier_result.has_value());
        auto service_instance_deployment = ServiceInstanceDeployment{serviceIdentifier,
                                                                     ServiceInstanceDeployment::BindingInformation{},
                                                                     QualityType::kASIL_QM,
                                                                     std::move(instance_specifier_result).value()};

        InstanceSpecifier portName{InstanceSpecifier::Create(port_name_).value()};
        Configuration::ServiceTypeDeployments type_deployments{};
        type_deployments.insert({serviceIdentifier, service_type_deployment});
        Configuration::ServiceInstanceDeployments instance_deployments{};
        instance_deployments.emplace(portName, service_instance_deployment);

        unit_.emplace(std::move(type_deployments),
                      std::move(instance_deployments),
                      GlobalConfiguration{},
                      TracingConfiguration{});
    }

    amp::optional<Configuration> unit_{};
    const char port_name_[13]{"abc/def/port"};
};

bmw::Result<std::string> GetStringFromJson(const json::Object& json_object)
{
    json::JsonWriter json_writer{};
    return json_writer.ToBuffer(json_object);
}

/**
 * @brief TC to test construction via two maps and specific move construction.
 */
TEST_F(ConfigurationFixture, construct)
{
    // Given a Configuration instance created on a bare minimum configuration
    PrepareMinimalConfiguration();

    // move construct unit2 from unit
    Configuration unit2(std::move(unit_.value()));

    // verify that unit2 really contains still valid copies
    EXPECT_EQ(unit2.GetServiceTypes().size(), 1);
    EXPECT_EQ(unit2.GetServiceInstances().size(), 1);
    EXPECT_NE(unit2.GetServiceInstances().find(InstanceSpecifier::Create(port_name_).value()),
              unit2.GetServiceInstances().end());

    // verify default values of global section
    EXPECT_EQ(unit2.GetGlobalConfiguration().GetProcessAsilLevel(), QualityType::kASIL_QM);
    EXPECT_EQ(unit2.GetGlobalConfiguration().GetReceiverMessageQueueSize(QualityType::kASIL_QM),
              GlobalConfiguration::DEFAULT_MIN_NUM_MESSAGES_RX_QUEUE);
    EXPECT_EQ(unit2.GetGlobalConfiguration().GetReceiverMessageQueueSize(QualityType::kASIL_B),
              GlobalConfiguration::DEFAULT_MIN_NUM_MESSAGES_RX_QUEUE);
    EXPECT_EQ(unit2.GetGlobalConfiguration().GetSenderMessageQueueSize(),
              GlobalConfiguration::DEFAULT_MIN_NUM_MESSAGES_TX_QUEUE);
}

TEST_F(ConfigurationFixture, ConfigIsCorrectlyParsedFromFile)
{
    RecordProperty("ParentRequirement", "");
    RecordProperty(
        "Description",
        "All relevant configuration aspects shall be read from a JSON file and not be manipulated by the read logic.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    // When parsing a json configuration file
    const auto json_path{"platform/aas/mw/com/impl/configuration/example/ara_com_config.json"};
    auto config = configuration::Parse(json_path);

    // Then manually generated ServiceTypes data structures using data from config file
    const auto service_identifier_type = make_ServiceIdentifierType("/bmw/ncar/services/TirePressureService", 12, 34);

    const std::string service_type_name{"/bmw/ncar/services/TirePressureService"};
    const std::string service_event_name{"CurrentPressureFrontLeft"};
    const std::string service_field_name{"CurrentTemperatureFrontLeft"};
    const LolaServiceId service_id{1234};
    const LolaEventId lola_event_type{20};
    const LolaFieldId lola_field_type{30};
    const LolaServiceTypeDeployment::EventIdMapping service_events{{service_event_name, lola_event_type}};
    const LolaServiceTypeDeployment::FieldIdMapping service_fields{{service_field_name, lola_field_type}};
    const LolaServiceTypeDeployment manual_lola_service_type(service_id, service_events, service_fields);

    // Match ServiceTypes generated from json
    const auto* generated_lola_service_type =
        amp::get_if<LolaServiceTypeDeployment>(&config.GetServiceTypes().at(service_identifier_type).binding_info_);
    ASSERT_NE(generated_lola_service_type, nullptr);
    EXPECT_EQ(manual_lola_service_type.service_id_, generated_lola_service_type->service_id_);
    const auto& manual_service_events = manual_lola_service_type.events_;
    const auto& manual_service_fields = manual_lola_service_type.fields_;
    const auto& generated_service_events = generated_lola_service_type->events_;
    const auto& generated_service_fields = generated_lola_service_type->fields_;
    EXPECT_EQ(manual_service_events.size(), generated_service_events.size());
    EXPECT_EQ(manual_service_fields.size(), generated_service_fields.size());
    EXPECT_EQ(manual_service_events.at(service_event_name), generated_service_events.at(service_event_name));
    EXPECT_EQ(manual_service_fields.at(service_field_name), generated_service_fields.at(service_field_name));

    // And manually generated ServiceInstances using data from config file
    const auto instance_specifier_result = InstanceSpecifier::Create("abc/abc/TirePressurePort");
    ASSERT_TRUE(instance_specifier_result.has_value());

    const std::string instance_event_name{"CurrentPressureFrontLeft"};
    const std::string instance_field_name{"CurrentTemperatureFrontLeft"};
    const LolaServiceInstanceId instance_id{1234};
    const std::size_t shared_memory_size{10000};
    const std::uint16_t event_max_samples{50};
    const std::uint8_t event_max_subscribers{5};
    const std::uint16_t field_max_samples{60};
    const std::uint8_t field_max_subscribers{6};
    const std::vector<uid_t> allowed_consumer_ids_qm{42, 43};
    const std::vector<uid_t> allowed_consumer_ids_b{54, 55};
    const std::vector<uid_t> allowed_provider_ids_qm{15};
    const std::vector<uid_t> allowed_provider_ids_b{15};

    const LolaEventInstanceDeployment lola_event_instance{event_max_samples, event_max_subscribers};
    const LolaFieldInstanceDeployment lola_field_instance{field_max_samples, field_max_subscribers};
    const LolaServiceInstanceDeployment::EventInstanceMapping instance_events{
        {instance_event_name, lola_event_instance}};
    const LolaServiceInstanceDeployment::FieldInstanceMapping instance_fields{
        {instance_field_name, lola_field_instance}};
    const std::unordered_map<QualityType, std::vector<uid_t>> allowed_consumers{
        {QualityType::kASIL_QM, allowed_consumer_ids_qm}, {QualityType::kASIL_B, allowed_consumer_ids_b}};
    const std::unordered_map<QualityType, std::vector<uid_t>> allowed_providers{
        {QualityType::kASIL_QM, allowed_provider_ids_qm}, {QualityType::kASIL_B, allowed_provider_ids_b}};

    LolaServiceInstanceDeployment binding_info(instance_id, instance_events, instance_fields);
    binding_info.allowed_consumer_ = allowed_consumers;
    binding_info.allowed_provider_ = allowed_providers;
    binding_info.shared_memory_size_ = shared_memory_size;
    const QualityType asil_level{QualityType::kASIL_B};

    ServiceInstanceDeployment manual_service_instance(
        service_identifier_type, binding_info, asil_level, instance_specifier_result.value());

    // Match ServiceInstances generated from json
    const ServiceInstanceDeployment& generated_service_instance =
        config.GetServiceInstances().at(instance_specifier_result.value());

    auto serialized_manual_service_instance = manual_service_instance.Serialize();
    auto serialized_manual_service_instance_string = GetStringFromJson(manual_service_instance.Serialize());
    ASSERT_TRUE(serialized_manual_service_instance_string.has_value());

    auto serialized_generated_service_instance = generated_service_instance.Serialize();
    auto serialized_generated_service_instance_string = GetStringFromJson(generated_service_instance.Serialize());
    ASSERT_TRUE(serialized_generated_service_instance_string.has_value());

    EXPECT_EQ(serialized_manual_service_instance_string.value(), serialized_generated_service_instance_string.value());

    auto manual_lola_service_instance_deployment =
        amp::get_if<LolaServiceInstanceDeployment>(&manual_service_instance.bindingInfo_);
    auto generated_lola_service_instance_deployment =
        amp::get_if<LolaServiceInstanceDeployment>(&generated_service_instance.bindingInfo_);
    ASSERT_NE(manual_lola_service_instance_deployment, nullptr);
    ASSERT_NE(generated_lola_service_instance_deployment, nullptr);
    EXPECT_EQ(manual_lola_service_instance_deployment->instance_id_,
              generated_lola_service_instance_deployment->instance_id_);
    EXPECT_EQ(manual_lola_service_instance_deployment->shared_memory_size_,
              generated_lola_service_instance_deployment->shared_memory_size_);
    EXPECT_EQ(manual_lola_service_instance_deployment->allowed_consumer_,
              generated_lola_service_instance_deployment->allowed_consumer_);
    EXPECT_EQ(manual_lola_service_instance_deployment->allowed_provider_,
              generated_lola_service_instance_deployment->allowed_provider_);
    EXPECT_EQ(manual_lola_service_instance_deployment->events_, generated_lola_service_instance_deployment->events_);
    EXPECT_EQ(manual_lola_service_instance_deployment->fields_, generated_lola_service_instance_deployment->fields_);
}

}  // namespace
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
