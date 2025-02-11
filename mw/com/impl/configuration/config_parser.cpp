// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#include "platform/aas/mw/com/impl/configuration/config_parser.h"

#include "platform/aas/mw/com/impl/configuration/lola_service_instance_deployment.h"
#include "platform/aas/mw/com/impl/configuration/quality_type.h"
#include "platform/aas/mw/com/impl/configuration/service_type_deployment.h"
#include "platform/aas/mw/com/impl/configuration/tracing_configuration.h"
#include "platform/aas/mw/com/impl/instance_specifier.h"
#include "platform/aas/mw/com/impl/tracing/configuration/service_element_type.h"

#include "platform/aas/lib/json/json_parser.h"
#include "platform/aas/mw/log/logging.h"

#include <amp_string_view.hpp>

#include <cstdlib>
#include <exception>
#include <set>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <unordered_map>
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
namespace configuration
{
namespace
{

using std::string_view_literals::operator""sv;

constexpr auto ServiceInstancesKey = "serviceInstances"sv;
constexpr auto InstanceSpecifierKey = "instanceSpecifier"sv;
constexpr auto ServiceTypeNameKey = "serviceTypeName"sv;
constexpr auto VersionKey = "version"sv;
constexpr auto MajorVersionKey = "major"sv;
constexpr auto MinorVersionKey = "minor"sv;
constexpr auto DeploymentInstancesKey = "instances"sv;
constexpr auto BindingKey = "binding"sv;
constexpr auto BindingsKey = "bindings"sv;
constexpr auto AsilKey = "asil-level"sv;
constexpr auto ServiceIdKey = "serviceId"sv;
constexpr auto InstanceIdKey = "instanceId"sv;
constexpr auto ServiceTypesKey = "serviceTypes"sv;
constexpr auto EventsKey = "events"sv;
constexpr auto EventNameKey = "eventName"sv;
constexpr auto EventIdKey = "eventId"sv;
constexpr auto FieldsKey = "fields"sv;
constexpr auto FieldNameKey = "fieldName"sv;
constexpr auto FieldIdKey = "fieldId"sv;
constexpr auto EventNumberOfSampleSlotsKey = "numberOfSampleSlots"sv;
constexpr auto EventMaxSamplesKey = "maxSamples"sv;
constexpr auto EventMaxSubscribersKey = "maxSubscribers"sv;
constexpr auto EventEnforceMaxSamplesKey = "enforceMaxSamples"sv;
constexpr auto EventMaxConcurrentAllocationsKey = "maxConcurrentAllocations"sv;
constexpr auto FieldNumberOfSampleSlotsKey = "numberOfSampleSlots"sv;
constexpr auto FieldMaxSubscribersKey = "maxSubscribers"sv;
constexpr auto FieldEnforceMaxSamplesKey = "enforceMaxSamples"sv;
constexpr auto FieldMaxConcurrentAllocationsKey = "maxConcurrentAllocations"sv;
constexpr auto LolaShmSizeKey = "shm-size"sv;
constexpr auto GlobalPropertiesKey = "global"sv;
constexpr auto AllowedConsumerKey = "allowedConsumer"sv;
constexpr auto AllowedProviderKey = "allowedProvider"sv;
constexpr auto QueueSizeKey = "queue-size"sv;
constexpr auto ShmSizeCalcModeKey = "shm-size-calc-mode"sv;
constexpr auto TracingPropertiesKey = "tracing"sv;
constexpr auto TracingEnabledKey = "enable"sv;
constexpr auto TracingApplicationInstanceIDKey = "applicationInstanceID"sv;
constexpr auto TracingTraceFilterConfigPathKey = "traceFilterConfigPath"sv;
constexpr auto TracingServiceElementEnabledKey = "enableIpcTracing"sv;
constexpr auto PermissionChecksKey = "permission-checks"sv;

constexpr auto SomeIpBinding = "SOME/IP"sv;
constexpr auto ShmBinding = "SHM"sv;
constexpr auto ShmSizeCalcModeSimulation = "SIMULATION"sv;
constexpr auto ShmSizeCalcModeEstimation = "ESTIMATION"sv;

constexpr auto TracingEnabledDefaultValue = false;
constexpr auto TracingTraceFilterConfigPathDefaultValue{"./etc/mw_com_trace_filter.json"sv};
constexpr auto StrictPermission{"strict"sv};
constexpr auto FilePermissionsOnEmpty{"file-permissions-on-empty"sv};

void error_if_found(const bmw::json::Object::const_iterator& iterator_to_element, const bmw::json::Object& json_obj)
{

    if (iterator_to_element != json_obj.end())
    {
        mw::log::LogError() << static_cast<std::string_view>("Parsing an element ")
                            << (iterator_to_element->first.GetAsStringView())
                            << static_cast<std::string_view>(" which is not currently supported.")
                            << static_cast<std::string_view>(
                                   " Remove this element from the configuration. Aborting!\n");

        std::abort();
    }
}

auto ParseInstanceSpecifier(const bmw::json::Any& json) -> InstanceSpecifier
{
    const auto& instanceSpecifierJson = json.As<bmw::json::Object>().value().get().find(InstanceSpecifierKey.data());
    if (instanceSpecifierJson != json.As<bmw::json::Object>().value().get().cend())
    {
        const auto string_view = instanceSpecifierJson->second.As<std::string>()->get();
        const auto instance_specifier_result = InstanceSpecifier::Create(string_view);
        if (!instance_specifier_result.has_value())
        {
            bmw::mw::log::LogFatal("lola") << "Invalid InstanceSpecifier.";
            std::terminate(); /* Terminate call tolerated.See Assumptions of Use in mw/com/design/README.md*/
        }
        return instance_specifier_result.value();
    }

    bmw::mw::log::LogFatal("lola") << "No instance specifier provided. Required argument.";
    std::terminate(); /* Terminate call tolerated.See Assumptions of Use in mw/com/design/README.md*/
}

auto ParseServiceTypeName(const bmw::json::Any& json) -> const std::string&
{
    const auto& serviceTypeName = json.As<bmw::json::Object>().value().get().find(ServiceTypeNameKey.data());
    if (serviceTypeName != json.As<bmw::json::Object>().value().get().cend())
    {
        return serviceTypeName->second.As<std::string>().value().get();
    }

    bmw::mw::log::LogFatal("lola") << "No service type name provided. Required argument.";
    
    /* Terminate call tolerated.See Assumptions of Use in mw/com/design/README.md*/
    std::terminate();
    
}

auto ParseVersion(const bmw::json::Any& json) -> std::pair<std::uint32_t, std::uint32_t>
{
    const auto& version = json.As<bmw::json::Object>().value().get().find(VersionKey.data());
    if (version != json.As<bmw::json::Object>().value().get().cend())
    {
        const auto& version_object = version->second.As<bmw::json::Object>().value().get();
        const auto major_version_number = version_object.find(MajorVersionKey.data());
        const auto minor_version_number = version_object.find(MinorVersionKey.data());

        if ((major_version_number != version_object.cend()) && (minor_version_number != version_object.cend()))
        {
            return std::pair<std::uint32_t, std::uint32_t>{major_version_number->second.As<std::uint32_t>().value(),
                                                           minor_version_number->second.As<std::uint32_t>().value()};
        }
    }

    bmw::mw::log::LogFatal("lola") << "No Version provided. Required argument.";
    
    /* Terminate call tolerated.See Assumptions of Use in mw/com/design/README.md*/
    std::terminate();
    
}

auto ParseServiceTypeIdentifier(const bmw::json::Any& json) -> ServiceIdentifierType
{
    const auto& name = ParseServiceTypeName(json);
    const auto& version = ParseVersion(json);

    return make_ServiceIdentifierType(name.data(), version.first, version.second);
}

auto ParseAsilLevel(const bmw::json::Any& json) -> amp::optional<QualityType>
{
    const auto& quality = json.As<bmw::json::Object>().value().get().find(AsilKey.data());
    if (quality != json.As<bmw::json::Object>().value().get().cend())
    {
        const auto& qualityValue = quality->second.As<std::string>().value().get();

        if (qualityValue == "QM")
        {
            return QualityType::kASIL_QM;
        }

        if (qualityValue == "B")
        {
            return QualityType::kASIL_B;
        }

        return QualityType::kInvalid;
    }

    return amp::nullopt;
}

auto ParseShmSizeCalcMode(const bmw::json::Any& json) -> amp::optional<ShmSizeCalculationMode>
{
    const auto& shm_size_calc_mode = json.As<bmw::json::Object>().value().get().find(ShmSizeCalcModeKey.data());
    if (shm_size_calc_mode != json.As<bmw::json::Object>().value().get().cend())
    {
        const auto& shm_size_calc_mode_value = shm_size_calc_mode->second.As<std::string>().value().get();

        if (shm_size_calc_mode_value == ShmSizeCalcModeEstimation)
        {
            return ShmSizeCalculationMode::kEstimation;
        }
        else if (shm_size_calc_mode_value == ShmSizeCalcModeSimulation)
        {
            return ShmSizeCalculationMode::kSimulation;
        }
        else
        {
            bmw::mw::log::LogError("lola")
                << "Unknown value " << shm_size_calc_mode_value << " in key " << ShmSizeCalcModeKey;
            
            /* Terminate call tolerated.See Assumptions of Use in mw/com/design/README.md*/
            std::terminate();
            
        }
    }

    return amp::nullopt;
}

auto ParseAllowedUser(const bmw::json::Any& json, std::string_view key) noexcept
    -> std::unordered_map<QualityType, std::vector<uid_t>>
{
    std::unordered_map<QualityType, std::vector<uid_t>> user_map{};
    auto allowed_user = json.As<bmw::json::Object>().value().get().find(key.data());

    if (allowed_user != json.As<bmw::json::Object>().value().get().cend())
    {
        for (const auto& user : allowed_user->second.As<bmw::json::Object>().value().get())
        {
            std::vector<uid_t> user_ids{};
            for (const auto& user_id : user.second.As<bmw::json::List>().value().get())
            {
                user_ids.push_back(user_id.As<uid_t>().value());
            }

            if (user.first == "QM")
            {
                user_map[QualityType::kASIL_QM] = std::move(user_ids);
            }
            else if (user.first == "B")
            {
                user_map[QualityType::kASIL_B] = std::move(user_ids);
            }
            else
            {
                bmw::mw::log::LogError("lola")
                    << "Unknown quality type in " << key << " " << user.first.GetAsStringView();
                /* Terminate call tolerated.See Assumptions of Use in mw/com/design/README.md*/
                std::terminate();
            }
        }
    }

    return user_map;
}

auto ParseAllowedConsumer(const bmw::json::Any& json) noexcept -> std::unordered_map<QualityType, std::vector<uid_t>>
{
    return ParseAllowedUser(json, AllowedConsumerKey.data());
}

auto ParseAllowedProvider(const bmw::json::Any& json) noexcept -> std::unordered_map<QualityType, std::vector<uid_t>>
{
    return ParseAllowedUser(json, AllowedProviderKey.data());
}

class ServiceElementInstanceDeploymentParser
{
  public:
    explicit ServiceElementInstanceDeploymentParser(const bmw::json::Object& json_object) : json_object_{json_object} {}

    std::string GetName(const bmw::json::Object::const_iterator name) const noexcept
    {
        if (name == json_object_.cend())
        {
            bmw::mw::log::LogFatal("lola") << "No Event/Field-Name provided. Required attribute";
            
            /* Terminate call tolerated.See Assumptions of Use in mw/com/design/README.md*/
            std::terminate();
            
        }
        return name->second.As<std::string>().value().get();
    }

    void CheckContainsEvent(const bmw::json::Object::const_iterator name, const LolaServiceInstanceDeployment& service)
    {
        if (service.ContainsEvent(name->second.As<std::string>().value().get()))
        {
            bmw::mw::log::LogFatal("lola") << "Event Name Duplicated. Not allowed";
            
            /* Terminate call tolerated.See Assumptions of Use in mw/com/design/README.md*/
            std::terminate();
            
        }
    }

    void CheckContainsField(const bmw::json::Object::const_iterator name, const LolaServiceInstanceDeployment& service)
    {
        if (service.ContainsField(name->second.As<std::string>().value().get()))
        {
            bmw::mw::log::LogFatal("lola") << "Field Name Duplicated. Not allowed";
            
            /* Terminate call tolerated.See Assumptions of Use in mw/com/design/README.md*/
            std::terminate();
            
        }
    }

    template <typename Deployment>
    void FillNumberOfSampleSlots(const bmw::json::Object::const_iterator number_of_sample_slots, Deployment& deployment)
    {
        if (number_of_sample_slots != json_object_.cend())
        {
            deployment.SetNumberOfSampleSlots(number_of_sample_slots->second.As<std::uint16_t>().value());
        }
    }

    template <typename Deployment>
    void FillMaxSubscribers(const bmw::json::Object::const_iterator max_subscribers, Deployment& deployment)
    {
        if (max_subscribers != json_object_.cend())
        {
            deployment.max_subscribers_ = max_subscribers->second.As<std::uint8_t>().value();
        }
    }

    template <typename Deployment>
    void FillMaxConcurrentAllocations(const bmw::json::Object::const_iterator max_concurrent_allocations,
                                      Deployment& deployment)
    {
        if (max_concurrent_allocations != json_object_.cend())
        {
            deployment.max_concurrent_allocations_ = max_concurrent_allocations->second.As<std::uint8_t>().value();
        }
    }

    template <typename Deployment>
    void FillEnforceMaxSamples(const bmw::json::Object::const_iterator enforce_max_samples, Deployment& deployment)
    {
        if (enforce_max_samples != json_object_.cend())
        {
            deployment.enforce_max_samples_ = enforce_max_samples->second.As<bool>().value();
        }
    }

  private:
    const bmw::json::Object& json_object_;
};

auto ParseLolaEventInstanceDeployment(const bmw::json::Any& json, LolaServiceInstanceDeployment& service) noexcept
    -> void
{
    const auto& events = json.As<bmw::json::Object>().value().get().find(EventsKey.data());
    if (events == json.As<bmw::json::Object>().value().get().cend())
    {
        return;
    }

    for (const auto& event : events->second.As<bmw::json::List>().value().get())
    {
        const auto& event_object = event.As<bmw::json::Object>().value().get();
        const auto& event_name = event_object.find(EventNameKey.data());
        const auto& max_samples = event_object.find(EventMaxSamplesKey.data());
        const auto& number_of_sample_slots = event_object.find(EventNumberOfSampleSlotsKey.data());
        const auto& max_subscribers = event_object.find(EventMaxSubscribersKey.data());
        const auto& enforce_max_samples = event_object.find(EventEnforceMaxSamplesKey.data());
        const auto& max_concurrent_allocations = event_object.find(EventMaxConcurrentAllocationsKey.data());

        error_if_found(max_concurrent_allocations, event_object);

        ServiceElementInstanceDeploymentParser deployment_parser{event_object};

        auto event_name_value = deployment_parser.GetName(event_name);
        deployment_parser.CheckContainsEvent(event_name, service);

        LolaEventInstanceDeployment event_deployment{};

        // deprecation check "for max_samples"
        if (max_samples != event_object.cend())
        {
            if (number_of_sample_slots != event_object.cend())
            {
                bmw::mw::log::LogFatal("lola") << "<maxSamples> and <numberOfSampleSlots> provided for event "
                                               << event_name_value << ". This is invalid!";
                
                /* Terminate call tolerated.See Assumptions of Use in mw/com/design/README.md*/
                std::terminate();
                
            }
            bmw::mw::log::LogWarn("lola") << "<maxSamples> property for event is DEPRECATED!";
            deployment_parser.FillNumberOfSampleSlots(max_samples, event_deployment);
        }
        deployment_parser.FillNumberOfSampleSlots(number_of_sample_slots, event_deployment);
        deployment_parser.FillMaxSubscribers(max_subscribers, event_deployment);
        deployment_parser.FillMaxConcurrentAllocations(max_concurrent_allocations, event_deployment);
        deployment_parser.FillEnforceMaxSamples(enforce_max_samples, event_deployment);
        const auto emplace_result = service.events_.emplace(std::piecewise_construct,
                                                            std::forward_as_tuple(std::move(event_name_value)),
                                                            std::forward_as_tuple(std::move(event_deployment)));
        AMP_ASSERT_PRD_MESSAGE(emplace_result.second, "Could not emplace element in map");
    }
}

auto ParseLolaFieldInstanceDeployment(const bmw::json::Any& json, LolaServiceInstanceDeployment& service) noexcept
    -> void
{
    const auto& fields = json.As<bmw::json::Object>().value().get().find(FieldsKey.data());
    if (fields == json.As<bmw::json::Object>().value().get().cend())
    {
        return;
    }

    for (const auto& field : fields->second.As<bmw::json::List>().value().get())
    {
        const auto& field_object = field.As<bmw::json::Object>().value().get();
        const auto& field_name = field_object.find(FieldNameKey.data());
        const auto& number_of_sample_slots = field_object.find(FieldNumberOfSampleSlotsKey.data());
        const auto& max_subscribers = field_object.find(FieldMaxSubscribersKey.data());
        const auto& enforce_max_samples = field_object.find(FieldEnforceMaxSamplesKey.data());
        const auto& max_concurrent_allocations = field_object.find(FieldMaxConcurrentAllocationsKey.data());

        error_if_found(max_concurrent_allocations, field_object);

        ServiceElementInstanceDeploymentParser deployment_parser{field_object};

        auto field_name_value = deployment_parser.GetName(field_name);
        deployment_parser.CheckContainsField(field_name, service);

        LolaFieldInstanceDeployment field_deployment{};

        deployment_parser.FillNumberOfSampleSlots(number_of_sample_slots, field_deployment);
        deployment_parser.FillMaxSubscribers(max_subscribers, field_deployment);
        deployment_parser.FillMaxConcurrentAllocations(max_concurrent_allocations, field_deployment);
        deployment_parser.FillEnforceMaxSamples(enforce_max_samples, field_deployment);

        const auto emplace_result = service.fields_.emplace(std::piecewise_construct,
                                                            std::forward_as_tuple(std::move(field_name_value)),
                                                            std::forward_as_tuple(std::move(field_deployment)));
        AMP_ASSERT_PRD_MESSAGE(emplace_result.second, "Could not emplace element in map");
    }
}

auto ParseEventTracingEnabled(const bmw::json::Any& json,
                              TracingConfiguration& tracing_configuration,
                              const amp::string_view service_type_name_view,
                              const InstanceSpecifier instance_specifier) noexcept -> void
{
    const auto& events = json.As<bmw::json::Object>().value().get().find(EventsKey.data());
    if (events == json.As<bmw::json::Object>().value().get().cend())
    {
        return;
    }

    for (const auto& event : events->second.As<bmw::json::List>().value().get())
    {
        const auto& event_object = event.As<bmw::json::Object>().value().get();
        const auto event_name = event_object.find(EventNameKey.data());
        AMP_ASSERT_PRD(event_name != event_object.end());

        const auto event_name_value = event_name->second.As<std::string>().value().get();

        const auto& enable_tracing = event_object.find(TracingServiceElementEnabledKey.data());
        if (enable_tracing != event_object.end())
        {
            const auto enable_tracing_value = enable_tracing->second.As<bool>().value();
            if (enable_tracing_value)
            {
                std::string service_type_name{service_type_name_view.data(), service_type_name_view.size()};
                tracing::ServiceElementIdentifier service_element_identifier{
                    std::move(service_type_name), event_name_value, tracing::ServiceElementType::EVENT};
                tracing_configuration.SetServiceElementTracingEnabled(service_element_identifier, instance_specifier);
            }
        }
    }
}

auto ParseFieldTracingEnabled(const bmw::json::Any& json,
                              TracingConfiguration& tracing_configuration,
                              const amp::string_view service_type_name_view,
                              const InstanceSpecifier instance_specifier) noexcept -> void
{
    const auto& fields = json.As<bmw::json::Object>().value().get().find(FieldsKey.data());
    if (fields == json.As<bmw::json::Object>().value().get().cend())
    {
        return;
    }

    for (const auto& field : fields->second.As<bmw::json::List>().value().get())
    {
        const auto& field_object = field.As<bmw::json::Object>().value().get();
        const auto field_name = field_object.find(FieldNameKey.data());
        AMP_ASSERT_PRD(field_name != field_object.end());

        const auto field_name_value = field_name->second.As<std::string>().value().get();

        const auto& enable_tracing = field_object.find(TracingServiceElementEnabledKey.data());
        if (enable_tracing != field_object.end())
        {
            const auto enable_tracing_value = enable_tracing->second.As<bool>().value();
            if (enable_tracing_value)
            {
                std::string service_type_name{service_type_name_view.data(), service_type_name_view.size()};
                tracing::ServiceElementIdentifier service_element_identifier{
                    std::move(service_type_name), field_name_value, tracing::ServiceElementType::FIELD};
                tracing_configuration.SetServiceElementTracingEnabled(service_element_identifier, instance_specifier);
            }
        }
    }
}

auto ParsePermissionChecks(const bmw::json::Any& deployment_instance) -> std::string_view
{
    const auto permission_checks =
        deployment_instance.As<bmw::json::Object>().value().get().find(PermissionChecksKey.data());
    if (permission_checks != deployment_instance.As<bmw::json::Object>().value().get().cend())
    {
        const auto& perm_result = permission_checks->second.As<std::string>().value().get();
        if (perm_result != FilePermissionsOnEmpty && perm_result != StrictPermission)
        {
            bmw::mw::log::LogFatal("lola") << "Unknown permission" << perm_result << "in permission-checks attribute";
            
            /* Terminate call tolerated.See Assumptions of Use in mw/com/design/README.md*/
            std::terminate();
            
        }
        return perm_result;
    }

    return FilePermissionsOnEmpty;
}

auto ParseLolaServiceInstanceDeployment(const bmw::json::Any& json) -> LolaServiceInstanceDeployment
{
    LolaServiceInstanceDeployment service{};
    const auto& found_shm_size = json.As<bmw::json::Object>().value().get().find(LolaShmSizeKey.data());
    if (found_shm_size != json.As<bmw::json::Object>().value().get().cend())
    {
        service.shared_memory_size_ = found_shm_size->second.As<std::uint64_t>().value();
    }

    const auto& instance_id = json.As<bmw::json::Object>().value().get().find(InstanceIdKey.data());
    if (instance_id != json.As<bmw::json::Object>().value().get().cend())
    {
        service.instance_id_ =
            LolaServiceInstanceId{instance_id->second.As<LolaServiceInstanceId::InstanceId>().value()};
    }

    ParseLolaEventInstanceDeployment(json, service);
    ParseLolaFieldInstanceDeployment(json, service);

    service.strict_permissions_ = ParsePermissionChecks(json) == StrictPermission;

    service.allowed_consumer_ = ParseAllowedConsumer(json);
    service.allowed_provider_ = ParseAllowedProvider(json);

    return service;
}

auto ParseServiceInstanceDeployments(const bmw::json::Any& json,
                                     TracingConfiguration& tracing_configuration,
                                     const ServiceIdentifierType& service,
                                     const InstanceSpecifier& instance_specifier)
    -> std::vector<ServiceInstanceDeployment>
{
    const auto& deploymentInstances = json.As<bmw::json::Object>().value().get().find(DeploymentInstancesKey.data());
    if (deploymentInstances == json.As<bmw::json::Object>().value().get().cend())
    {
        bmw::mw::log::LogFatal("lola") << "No deployment instances provided. Required argument.";
        
        /* Terminate call tolerated.See Assumptions of Use in mw/com/design/README.md*/
        std::terminate();
        
    }

    std::vector<ServiceInstanceDeployment> deployments{};

    for (const auto& deploymentInstance : deploymentInstances->second.As<bmw::json::List>().value().get())
    {
        const auto asil_level = ParseAsilLevel(deploymentInstance);
        if ((asil_level.has_value() == false) || (asil_level.value() == QualityType::kInvalid))
        {
            bmw::mw::log::LogFatal("lola") << "Invalid or no ASIL-Level provided. Required argument.";
            
            /* Terminate call tolerated.See Assumptions of Use in mw/com/design/README.md*/
            std::terminate();
            
        }
        const auto binding = deploymentInstance.As<bmw::json::Object>().value().get().find(BindingKey.data());
        if (binding != deploymentInstance.As<bmw::json::Object>().value().get().cend())
        {
            const auto& bindingValue = binding->second.As<std::string>().value().get();
            if (bindingValue == SomeIpBinding)
            {
                bmw::mw::log::LogFatal("lola") << "Provided SOME/IP binding, which can not be parsed.";
                std::terminate();
            }
            else if (bindingValue == ShmBinding)
            {
                // Return Value not needed in this context
                amp::ignore = deployments.emplace_back(service,
                                                       ParseLolaServiceInstanceDeployment(deploymentInstance),
                                                       asil_level.value(),
                                                       instance_specifier);
            }
            else
            {
                bmw::mw::log::LogFatal("lola") << "No unknown binding provided. Required argument.";
                
                /* Terminate call tolerated.See Assumptions of Use in mw/com/design/README.md*/
                std::terminate();
                
            }

            if (tracing_configuration.IsTracingEnabled())
            {
                ParseEventTracingEnabled(
                    deploymentInstance, tracing_configuration, service.ToString(), instance_specifier);
                ParseFieldTracingEnabled(
                    deploymentInstance, tracing_configuration, service.ToString(), instance_specifier);
            }
        }
        else
        {
            bmw::mw::log::LogFatal("lola") << "No binding provided. Required argument.";
            
            /* Terminate call tolerated.See Assumptions of Use in mw/com/design/README.md*/
            std::terminate();
            
        }
    }
    return deployments; 
}

auto ParseServiceInstances(const bmw::json::Any& json, TracingConfiguration& tracing_configuration) noexcept
    -> Configuration::ServiceInstanceDeployments
{
    const auto& object = json.As<bmw::json::Object>().value().get();
    const auto& servicesInstances = object.find(ServiceInstancesKey.data());
    if (servicesInstances == object.cend())
    {
        bmw::mw::log::LogFatal("lola") << "No service instances provided. Required argument.";
        
        /* Terminate call tolerated.See Assumptions of Use in mw/com/design/README.md*/
        std::terminate();
        
    }
    Configuration::ServiceInstanceDeployments service_instance_deployments{};
    for (const auto& serviceInstance : servicesInstances->second.As<bmw::json::List>().value().get())
    {
        auto instanceSpecifier = ParseInstanceSpecifier(serviceInstance);

        auto service_identifier = ParseServiceTypeIdentifier(serviceInstance);

        auto instance_deployments = ParseServiceInstanceDeployments(
            serviceInstance, tracing_configuration, service_identifier, instanceSpecifier);
        if (instance_deployments.size() != std::size_t{1U})
        {
            
            bmw::mw::log::LogFatal("lola") << "More or less then one deployment for " << service_identifier.ToString()
                                           << ". Multi-Binding support right now not supported";
            
            
            /* Terminate call tolerated.See Assumptions of Use in mw/com/design/README.md*/
            std::terminate();
            
        }

        auto emplaceRes = service_instance_deployments.emplace(std::piecewise_construct,
                                                               std::forward_as_tuple(instanceSpecifier),
                                                               std::forward_as_tuple(instance_deployments.at(0)));
        if (emplaceRes.second == false)
        {
            bmw::mw::log::LogFatal("lola") << "Unexpected error, when inserting service instance deployments.";
            
            /* Terminate call tolerated.See Assumptions of Use in mw/com/design/README.md*/
            std::terminate();
            
        }
    }
    return service_instance_deployments;
}

auto ParseLolaEventTypeDeployments(const bmw::json::Any& json, LolaServiceTypeDeployment& service) noexcept -> bool
{
    const auto& events = json.As<bmw::json::Object>().value().get().find(EventsKey.data());
    if (events == json.As<bmw::json::Object>().value().get().cend())
    {
        return false;
    }
    for (const auto& event : events->second.As<bmw::json::List>().value().get())
    {
        const auto& event_object = event.As<bmw::json::Object>().value().get();
        const auto& event_name = event_object.find(EventNameKey.data());
        const auto& event_id = event_object.find(EventIdKey.data());

        if ((event_name != event_object.cend()) && (event_id != event_object.cend()))
        {
            const auto result =
                service.events_.emplace(std::piecewise_construct,
                                        std::forward_as_tuple(event_name->second.As<std::string>().value().get()),
                                        std::forward_as_tuple(event_id->second.As<std::uint16_t>().value()));

            if (result.second != true)
            {
                bmw::mw::log::LogFatal("lola") << "An event was configured twice.";
                
                /* Terminate call tolerated.See Assumptions of Use in mw/com/design/README.md*/
                std::terminate();
                
            }
        }
        else
        {
            bmw::mw::log::LogFatal("lola") << "Either no Event-Name or no Event-Id provided";
            
            /* Terminate call tolerated.See Assumptions of Use in mw/com/design/README.md*/
            std::terminate();
            
        }
    }
    return true;
}

auto ParseLolaFieldTypeDeployments(const bmw::json::Any& json, LolaServiceTypeDeployment& service) noexcept -> bool
{
    const auto& fields = json.As<bmw::json::Object>().value().get().find(FieldsKey.data());
    if (fields == json.As<bmw::json::Object>().value().get().cend())
    {
        return false;
    }

    for (const auto& field : fields->second.As<bmw::json::List>().value().get())
    {
        const auto& field_object = field.As<bmw::json::Object>().value().get();
        const auto& field_name = field_object.find(FieldNameKey.data());
        const auto& field_id = field_object.find(FieldIdKey.data());

        if ((field_name != field_object.cend()) && (field_id != field_object.cend()))
        {
            const auto result =
                service.fields_.emplace(std::piecewise_construct,
                                        std::forward_as_tuple(field_name->second.As<std::string>().value().get()),
                                        std::forward_as_tuple(field_id->second.As<std::uint16_t>().value()));

            if (result.second != true)
            {
                bmw::mw::log::LogFatal("lola") << "A field was configured twice.";
                
                /* Terminate call tolerated.See Assumptions of Use in mw/com/design/README.md*/
                std::terminate();
                
            }
        }
        else
        {
            bmw::mw::log::LogFatal("lola") << "Either no Field-Name or no Field-Id provided";
            
            /* Terminate call tolerated.See Assumptions of Use in mw/com/design/README.md*/
            std::terminate();
            
        }
    }
    return true;
}

auto AreEventAndFieldIdsUnique(const LolaServiceTypeDeployment& lola_service_type_deployment) noexcept -> bool
{
    const auto& events = lola_service_type_deployment.events_;
    const auto& fields = lola_service_type_deployment.fields_;

    static_assert(std::is_same<LolaEventId, LolaFieldId>::value,
                  "EventId and FieldId should have the same underlying type.");
    std::set<LolaEventId> ids{};

    for (const auto& event : events)
    {
        const auto result = ids.insert(event.second);
        if (!result.second)
        {
            return false;
        }
    }

    for (const auto& field : fields)
    {
        const auto result = ids.insert(field.second);
        if (!result.second)
        {
            return false;
        }
    }
    return true;
}

auto ParseLoLaServiceTypeDeployments(const bmw::json::Any& json) noexcept -> LolaServiceTypeDeployment
{
    const auto& service_id = json.As<bmw::json::Object>().value().get().find(ServiceIdKey.data());
    if (service_id != json.As<bmw::json::Object>().value().get().cend())
    {
        
        LolaServiceTypeDeployment lola{service_id->second.As<std::uint16_t>().value()};
        
        const bool events_exist = ParseLolaEventTypeDeployments(json, lola);
        const bool fields_exist = ParseLolaFieldTypeDeployments(json, lola);
        if (!events_exist && !fields_exist)
        {
            bmw::mw::log::LogFatal("lola") << "Configuration should contain at least one event or field.";
            
            /* Terminate call tolerated.See Assumptions of Use in mw/com/design/README.md*/
            std::terminate();
            
        }
        if (!AreEventAndFieldIdsUnique(lola))
        {
            bmw::mw::log::LogFatal("lola") << "Configuration cannot contain duplicate eventId or fieldIds.";
            
            /* Terminate call tolerated.See Assumptions of Use in mw/com/design/README.md*/
            std::terminate();
            
        }
        return lola;
    }
    else
    {
        bmw::mw::log::LogFatal("lola") << "No Service Id Provided. Required argument.";
        
        /* Terminate call tolerated.See Assumptions of Use in mw/com/design/README.md*/
        std::terminate();
        
    }
}

auto ParseServiceTypeDeployment(const bmw::json::Any& json) noexcept -> ServiceTypeDeployment
{
    const auto& bindings = json.As<bmw::json::Object>().value().get().find(BindingsKey.data());
    for (const auto& binding : bindings->second.As<bmw::json::List>().value().get())
    {
        auto binding_type = binding.As<bmw::json::Object>().value().get().find(BindingKey.data());
        if (binding_type != binding.As<bmw::json::Object>().value().get().cend())
        {
            const auto& value = binding_type->second.As<std::string>().value().get();
            if (value == ShmBinding)
            {
                LolaServiceTypeDeployment lola_deployment = ParseLoLaServiceTypeDeployments(binding);
                return ServiceTypeDeployment{lola_deployment};
            }
            else if (value == SomeIpBinding)
            {
                // we skip this, because we don't support SOME/IP right now.
            }
            else
            {
                bmw::mw::log::LogFatal("lola") << "No unknown binding provided. Required argument.";
                
                /* Terminate call tolerated.See Assumptions of Use in mw/com/design/README.md*/
                std::terminate();
                
            }
        }
        else
        {
            bmw::mw::log::LogFatal("lola") << "No binding provided. Required argument.";
            
            /* Terminate call tolerated.See Assumptions of Use in mw/com/design/README.md*/
            std::terminate();
            
        }
    }
    return ServiceTypeDeployment{amp::blank{}};
}

auto ParseServiceTypes(const bmw::json::Any& json) noexcept -> Configuration::ServiceTypeDeployments
{
    const auto& service_types = json.As<bmw::json::Object>().value().get().find(ServiceTypesKey.data());
    if (service_types == json.As<bmw::json::Object>().value().get().cend())
    {
        bmw::mw::log::LogFatal("lola") << "No service type deployments provided. Terminating";
        
        /* Terminate call tolerated.See Assumptions of Use in mw/com/design/README.md*/
        std::terminate();
        
    }

    Configuration::ServiceTypeDeployments service_type_deployments{};
    for (const auto& service_type : service_types->second.As<bmw::json::List>().value().get())
    {
        const auto service_identifier = ParseServiceTypeIdentifier(service_type);

        const auto service_deployment = ParseServiceTypeDeployment(service_type);
        const auto inserted = service_type_deployments.emplace(std::piecewise_construct,
                                                               std::forward_as_tuple(service_identifier),
                                                               std::forward_as_tuple(service_deployment));

        if (inserted.second != true)
        {
            bmw::mw::log::LogFatal("lola") << "Service Type was deployed twice";
            
            /* Terminate call tolerated.See Assumptions of Use in mw/com/design/README.md*/
            std::terminate();
            
        }
    }
    return service_type_deployments;
}

auto ParseReceiverQueueSize(const bmw::json::Any& global_config, const QualityType quality_type)
    -> amp::optional<std::int32_t>
{
    const auto& global_config_map = global_config.As<json::Object>().value().get();
    const auto& queue_size = global_config_map.find(QueueSizeKey.data());
    if (queue_size != global_config_map.cend())
    {
        std::string_view queue_type_str{};
        switch (quality_type)
        {
            case QualityType::kASIL_QM:
                queue_type_str = "QM-receiver";
                break;
            case QualityType::kASIL_B:
                queue_type_str = "B-receiver";
                break;
            case QualityType::kInvalid:  // LCOV_EXCL_LINE defensive programming
            default:  // LCOV_EXCL_LINE defensive programming Bug: We only must hand over QM or B here.
                
                /* Terminate call tolerated.See Assumptions of Use in mw/com/design/README.md*/
                std::terminate();  // LCOV_EXCL_LINE defensive programming
                
                break;
        }

        const auto& queue_size_map = queue_size->second.As<json::Object>().value().get();
        const auto& asil_queue_size = queue_size_map.find(queue_type_str.data());
        if (asil_queue_size != queue_size_map.cend())
        {
            return asil_queue_size->second.As<std::int32_t>().value();
        }
        else
        {
            return amp::nullopt;
        }
    }
    else
    {
        return amp::nullopt;
    }
}

auto ParseSenderQueueSize(const bmw::json::Any& global_config) -> amp::optional<std::int32_t>
{
    const auto& global_config_map = global_config.As<json::Object>().value().get();
    const auto& queue_size = global_config_map.find(QueueSizeKey.data());
    if (queue_size != global_config_map.cend())
    {
        const auto& queue_size_map = queue_size->second.As<json::Object>().value().get();
        const auto& asil_tx_queue_size = queue_size_map.find("B-sender");
        if (asil_tx_queue_size != queue_size_map.cend())
        {
            return asil_tx_queue_size->second.As<std::int32_t>().value();
        }
        else
        {
            return amp::nullopt;
        }
    }
    else
    {
        return amp::nullopt;
    }
}

auto ParseGlobalProperties(const bmw::json::Any& json) noexcept -> GlobalConfiguration
{
    GlobalConfiguration global_configuration{};

    const auto& top_level_object = json.As<bmw::json::Object>().value().get();
    const auto& process_properties = top_level_object.find(GlobalPropertiesKey.data());
    if (process_properties != top_level_object.cend())
    {
        const auto asil_level{ParseAsilLevel(process_properties->second)};
        if (asil_level.has_value() == false)
        {
            // set default (ASIL-QM)
            global_configuration.SetProcessAsilLevel(QualityType::kASIL_QM);
        }
        else
        {
            switch (asil_level.value())
            {
                case QualityType::kInvalid:
                    ::bmw::mw::log::LogFatal("lola") << "Invalid ASIL in global/asil-level, terminating.";
                    
                    /* Terminate call tolerated.See Assumptions of Use in mw/com/design/README.md*/
                    std::terminate();
                    
                    break;
                case QualityType::kASIL_QM:
                case QualityType::kASIL_B:
                    global_configuration.SetProcessAsilLevel(asil_level.value());
                    break;
                // LCOV_EXCL_START defensive programming
                default:
                    ::bmw::mw::log::LogFatal("lola") << "Unexpected QualityType, terminating";
                    
                    /* Terminate call tolerated.See Assumptions of Use in mw/com/design/README.md*/
                    std::terminate();
                    
                    break;
                    // LCOV_EXCL_STOP
            }
        }

        const amp::optional<std::int32_t> qm_rx_message_size{
            ParseReceiverQueueSize(process_properties->second, QualityType::kASIL_QM)};
        if (qm_rx_message_size.has_value())
        {
            global_configuration.SetReceiverMessageQueueSize(QualityType::kASIL_QM, qm_rx_message_size.value());
        }

        const amp::optional<std::int32_t> b_rx_message_size{
            ParseReceiverQueueSize(process_properties->second, QualityType::kASIL_B)};
        if (b_rx_message_size.has_value())
        {
            global_configuration.SetReceiverMessageQueueSize(QualityType::kASIL_B, b_rx_message_size.value());
        }

        const amp::optional<std::int32_t> b_tx_message_size{ParseSenderQueueSize(process_properties->second)};
        if (b_tx_message_size.has_value())
        {
            global_configuration.SetSenderMessageQueueSize(b_tx_message_size.value());
        }

        const amp::optional<ShmSizeCalculationMode> shm_size_calc_mode{
            ParseShmSizeCalcMode(process_properties->second)};
        if (shm_size_calc_mode.has_value())
        {
            global_configuration.SetShmSizeCalcMode(shm_size_calc_mode.value());
        }
    }
    else
    {
        global_configuration.SetProcessAsilLevel(QualityType::kASIL_QM);
    }
    return global_configuration;
}

auto ParseTracingEnabled(const bmw::json::Any& tracing_config) -> bool
{
    const auto& tracing_config_map = tracing_config.As<json::Object>().value().get();
    const auto& tracing_enabled = tracing_config_map.find(TracingEnabledKey.data());
    if (tracing_enabled != tracing_config_map.cend())
    {
        return tracing_enabled->second.As<bool>().value();
    }
    else
    {
        return TracingEnabledDefaultValue;
    }
}

auto ParseTracingApplicationInstanceId(const bmw::json::Any& tracing_config) -> const std::string&
{
    const auto& tracing_config_map = tracing_config.As<json::Object>().value().get();
    const auto& tracing_application_instance_id = tracing_config_map.find(TracingApplicationInstanceIDKey.data());
    if (tracing_application_instance_id != tracing_config_map.cend())
    {
        return tracing_application_instance_id->second.As<std::string>().value().get();
    }
    else
    {
        bmw::mw::log::LogFatal("lola") << "Could not find" << TracingApplicationInstanceIDKey
                                       << "in json file which is a required attribute.";
        std::terminate();
    }
}

auto ParseTracingTraceFilterConfigPath(const bmw::json::Any& tracing_config) -> std::string_view
{
    const auto& tracing_config_map = tracing_config.As<json::Object>().value().get();
    const auto& tracing_filter_config_path = tracing_config_map.find(TracingTraceFilterConfigPathKey.data());
    if (tracing_filter_config_path != tracing_config_map.cend())
    {
        return tracing_filter_config_path->second.As<std::string>().value().get();
    }
    else
    {
        return TracingTraceFilterConfigPathDefaultValue;
    }
}

auto ParseTracingProperties(const bmw::json::Any& json) noexcept -> TracingConfiguration
{
    TracingConfiguration tracing_configuration{};
    const auto& top_level_object = json.As<bmw::json::Object>().value().get();
    const auto& tracing_properties = top_level_object.find(TracingPropertiesKey.data());
    if (tracing_properties != top_level_object.cend())
    {
        const auto tracing_enabled = ParseTracingEnabled(tracing_properties->second);
        tracing_configuration.SetTracingEnabled(tracing_enabled);

        const auto tracing_application_instance_id = ParseTracingApplicationInstanceId(tracing_properties->second);
        tracing_configuration.SetApplicationInstanceID(tracing_application_instance_id);

        auto tracing_filter_config_path{ParseTracingTraceFilterConfigPath(tracing_properties->second)};
        tracing_configuration.SetTracingTraceFilterConfigPath(
            std::string{tracing_filter_config_path.data(), tracing_filter_config_path.size()});
    }
    return tracing_configuration;
}

void CrosscheckAsilLevels(const Configuration& config)
{
    for (const auto& service_instance : config.GetServiceInstances())
    {
        if ((service_instance.second.asilLevel_ == QualityType::kASIL_B) &&
            (config.GetGlobalConfiguration().GetProcessAsilLevel() != QualityType::kASIL_B))
        {
            ::bmw::mw::log::LogFatal("lola")
                << "Service instance has a higher ASIL than the process. This is invalid, terminating";
            
            /* Terminate call tolerated.See Assumptions of Use in mw/com/design/README.md*/
            std::terminate();
            
        }
    }
}

/**
 * \brief Checks, whether for all (binding) types used in service instances, there is also a corresponding type
 *        in service types.
 * @param config configuration to be crosschecked.
 */
void CrosscheckServiceInstancesToTypes(const Configuration& config)
{
    for (const auto& service_instance : config.GetServiceInstances())
    {
        const auto foundServiceType = config.GetServiceTypes().find(service_instance.second.service_);
        if (foundServiceType == config.GetServiceTypes().cend())
        {
            ::bmw::mw::log::LogFatal("lola")
                << "Service instance " << service_instance.first << "refers to a service type ("
                << service_instance.second.service_.ToString()
                << "), which is not configured. This is invalid, terminating";
            std::terminate(); /* Terminate call tolerated.See Assumptions of Use in mw/com/design/README.md*/
        }
        // check, that binding in service type and service instance are equal. Since currently ServiceTypeDeployment
        // only supports LolaServiceTypeDeployment, everything else than LolaServiceInstanceDeployment is an error.
        if (!amp::holds_alternative<LolaServiceInstanceDeployment>(service_instance.second.bindingInfo_))
        {
            ::bmw::mw::log::LogFatal("lola") << "Service instance " << service_instance.first
                                             << "refers to an not yet supported binding. This is invalid, terminating";
            std::terminate(); /* Terminate call tolerated.See Assumptions of Use in mw/com/design/README.md*/
        }
        // check, that for each service-element-name in the instance deployment, there exists a corresponding
        // service-element-name in the type deployment
        const auto& serviceTypeDeployment = amp::get<LolaServiceTypeDeployment>(foundServiceType->second.binding_info_);
        const auto& serviceInstanceDeployment =
            amp::get<LolaServiceInstanceDeployment>(service_instance.second.bindingInfo_);
        for (const auto& eventInstanceElement : serviceInstanceDeployment.events_)
        {
            const auto search = serviceTypeDeployment.events_.find(eventInstanceElement.first);
            if (search == serviceTypeDeployment.events_.cend())
            {
                ::bmw::mw::log::LogFatal("lola")
                    << "Service instance " << service_instance.first << "event" << eventInstanceElement.first
                    << "refers to an event, which doesn't exist in the referenced service type ("
                    << service_instance.second.service_.ToString() << "). This is invalid, terminating";
                std::terminate(); /* Terminate call tolerated.See Assumptions of Use in mw/com/design/README.md*/
            }
        }
        for (const auto& fieldInstanceElement : serviceInstanceDeployment.fields_)
        {
            const auto search = serviceTypeDeployment.fields_.find(fieldInstanceElement.first);
            if (search == serviceTypeDeployment.fields_.cend())
            {
                ::bmw::mw::log::LogFatal("lola")
                    << "Service instance " << service_instance.first << "field" << fieldInstanceElement.first
                    << "refers to a field, which doesn't exist in the referenced service type ("
                    << service_instance.second.service_.ToString() << "). This is invalid, terminating";
                std::terminate(); /* Terminate call tolerated.See Assumptions of Use in mw/com/design/README.md*/
            }
        }
    }
}

}  // namespace
}  // namespace configuration
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

/**
 * \brief Parses json configuration under given path and returns a Configuration object on success.
 */


/* Function is called with different type, thus different function is called. */
auto bmw::mw::com::impl::configuration::Parse(const amp::string_view path) noexcept -> Configuration
{
    const bmw::json::JsonParser json_parser_obj;
    // Reason for banning is AoU of vaJson library about integrity of provided path.
    // This AoU is forwarded as AoU of Lola. See 
    // NOLINTNEXTLINE(bmw-banned-function): The user has to guarantee the integrity of the path
    auto json_result = json_parser_obj.FromFile(path);
    if (!json_result.has_value())
    {
        ::bmw::mw::log::LogFatal("lola") << "Parsing config file" << path
                                         << "failed with error:" << json_result.error().Message() << ": "
                                         << json_result.error().UserMessage() << " . Terminating.";
        std::terminate();
    }
    return Parse(std::move(json_result).value());
}



auto bmw::mw::com::impl::configuration::Parse(bmw::json::Any json) noexcept -> Configuration
{
    auto tracing_configuration = ParseTracingProperties(json);
    auto service_type_deployments = ParseServiceTypes(json);
    auto service_instance_deployments = ParseServiceInstances(json, tracing_configuration);
    auto global_configuration = ParseGlobalProperties(json);

    Configuration configuration{std::move(service_type_deployments),
                                std::move(service_instance_deployments),
                                std::move(global_configuration),
                                std::move(tracing_configuration)};

    CrosscheckAsilLevels(configuration);
    CrosscheckServiceInstancesToTypes(configuration);

    return configuration;
}
