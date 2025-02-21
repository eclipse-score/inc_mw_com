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

#include "platform/aas/mw/com/impl/tracing/trace_error.h"

#include "platform/aas/lib/json/json_parser.h"
#include "platform/aas/mw/log/logging.h"

#include <amp_overload.hpp>
#include <amp_string_view.hpp>
#include <string_view>

#include <exception>
#include <set>
#include <string>
#include <type_traits>
#include <utility>

namespace bmw::mw::com::impl::tracing
{
namespace
{

constexpr auto kServicesKey = static_cast<std::string_view>("services");
constexpr auto kShortnamePathKey = static_cast<std::string_view>("shortname_path");
constexpr auto kEventsKey = static_cast<std::string_view>("events");
constexpr auto kFieldsKey = static_cast<std::string_view>("fields");
constexpr auto kMethodsKey = static_cast<std::string_view>("methods");
constexpr auto kShortnameKey = static_cast<std::string_view>("shortname");
constexpr auto kNotifierKey = static_cast<std::string_view>("notifier");
constexpr auto kGetterKey = static_cast<std::string_view>("getter");
constexpr auto kSetterKey = static_cast<std::string_view>("setter");

/// \brief List of json property names from the tracing filter config json file which are not currently implemented.
constexpr std::array service_element_notifier_filter_properties_not_implemented_array = {
    static_cast<std::string_view>("trace_subscribe_received"),
    static_cast<std::string_view>("trace_unsubscribe_received")};

/// \brief mapping of json property names from the tracing filter config json file to the corresponding
/// ProxyEventTracePointType enum.
constexpr std::array filter_property_proxy_event_mappings = {
    std::pair<const std::string_view, ProxyEventTracePointType>{"trace_subscribe_send",
                                                                ProxyEventTracePointType::SUBSCRIBE},
    std::pair<const std::string_view, ProxyEventTracePointType>{"trace_unsubscribe_send",
                                                                ProxyEventTracePointType::UNSUBSCRIBE},
    std::pair<const std::string_view, ProxyEventTracePointType>{"trace_subscription_state_changed",
                                                                ProxyEventTracePointType::SUBSCRIBE_STATE_CHANGE},
    std::pair<const std::string_view, ProxyEventTracePointType>{
        "trace_subscription_state_change_handler_registered",
        ProxyEventTracePointType::SET_SUBSCRIPTION_STATE_CHANGE_HANDLER},
    std::pair<const std::string_view, ProxyEventTracePointType>{
        "trace_subscription_state_change_handler_deregistered",
        ProxyEventTracePointType::UNSET_SUBSCRIPTION_STATE_CHANGE_HANDLER},
    std::pair<const std::string_view, ProxyEventTracePointType>{
        "trace_subscription_state_change_handler_callback",
        ProxyEventTracePointType::SUBSCRIPTION_STATE_CHANGE_HANDLER_CALLBACK},
    std::pair<const std::string_view, ProxyEventTracePointType>{"trace_get_new_samples",
                                                                ProxyEventTracePointType::GET_NEW_SAMPLES},
    std::pair<const std::string_view, ProxyEventTracePointType>{"trace_get_new_samples_callback",
                                                                ProxyEventTracePointType::GET_NEW_SAMPLES_CALLBACK},
    std::pair<const std::string_view, ProxyEventTracePointType>{"trace_receive_handler_registered",
                                                                ProxyEventTracePointType::SET_RECEIVE_HANDLER},
    std::pair<const std::string_view, ProxyEventTracePointType>{"trace_receive_handler_deregistered",
                                                                ProxyEventTracePointType::UNSET_RECEIVE_HANDLER},
    std::pair<const std::string_view, ProxyEventTracePointType>{"trace_receive_handler_callback",
                                                                ProxyEventTracePointType::RECEIVE_HANDLER_CALLBACK},
};

/// \brief mapping of json property names from the tracing filter config json file to the corresponding
/// SkeletonEventTracePointType enum.
constexpr std::array filter_property_skeleton_event_mappings = {
    std::pair<const std::string_view, SkeletonEventTracePointType>{"trace_send_allocate",
                                                                   SkeletonEventTracePointType::SEND_WITH_ALLOCATE},
    std::pair<const std::string_view, SkeletonEventTracePointType>{"trace_send", SkeletonEventTracePointType::SEND},
};

/// \brief mapping of json property names from the tracing filter config json file to the corresponding
/// ProxyFieldTracePointType enum.
constexpr std::array filter_property_proxy_field_notifier_mappings = {
    std::pair<const std::string_view, ProxyFieldTracePointType>{"trace_subscribe_send",
                                                                ProxyFieldTracePointType::SUBSCRIBE},
    std::pair<const std::string_view, ProxyFieldTracePointType>{"trace_unsubscribe_send",
                                                                ProxyFieldTracePointType::UNSUBSCRIBE},
    std::pair<const std::string_view, ProxyFieldTracePointType>{"trace_subscription_state_changed",
                                                                ProxyFieldTracePointType::SUBSCRIBE_STATE_CHANGE},
    std::pair<const std::string_view, ProxyFieldTracePointType>{
        "trace_subscription_state_change_handler_registered",
        ProxyFieldTracePointType::SET_SUBSCRIPTION_STATE_CHANGE_HANDLER},
    std::pair<const std::string_view, ProxyFieldTracePointType>{
        "trace_subscription_state_change_handler_deregistered",
        ProxyFieldTracePointType::UNSET_SUBSCRIPTION_STATE_CHANGE_HANDLER},
    std::pair<const std::string_view, ProxyFieldTracePointType>{
        "trace_subscription_state_change_handler_callback",
        ProxyFieldTracePointType::SUBSCRIPTION_STATE_CHANGE_HANDLER_CALLBACK},
    std::pair<const std::string_view, ProxyFieldTracePointType>{"trace_get_new_samples",
                                                                ProxyFieldTracePointType::GET_NEW_SAMPLES},
    std::pair<const std::string_view, ProxyFieldTracePointType>{"trace_get_new_samples_callback",
                                                                ProxyFieldTracePointType::GET_NEW_SAMPLES_CALLBACK},
    std::pair<const std::string_view, ProxyFieldTracePointType>{"trace_receive_handler_registered",
                                                                ProxyFieldTracePointType::SET_RECEIVE_HANDLER},
    std::pair<const std::string_view, ProxyFieldTracePointType>{"trace_receive_handler_deregistered",
                                                                ProxyFieldTracePointType::UNSET_RECEIVE_HANDLER},
    std::pair<const std::string_view, ProxyFieldTracePointType>{"trace_receive_handler_callback",
                                                                ProxyFieldTracePointType::RECEIVE_HANDLER_CALLBACK},
};

/// \brief mapping of json property names of getter object from the tracing filter config json file to the corresponding
/// ProxyFieldTracePointType enum.
constexpr std::array filter_property_proxy_field_getter_mappings = {
    std::pair<const std::string_view, ProxyFieldTracePointType>{"trace_request_send", ProxyFieldTracePointType::GET},
    std::pair<const std::string_view, ProxyFieldTracePointType>{"trace_response_received",
                                                                ProxyFieldTracePointType::GET_RESULT},
};

/// \brief mapping of json property names of setter object from the tracing filter config json file to the corresponding
/// ProxyFieldTracePointType enum.
constexpr std::array filter_property_proxy_field_setter_mappings = {
    std::pair<const std::string_view, ProxyFieldTracePointType>{"trace_request_send", ProxyFieldTracePointType::SET},
    std::pair<const std::string_view, ProxyFieldTracePointType>{"trace_response_received",
                                                                ProxyFieldTracePointType::SET_RESULT},
};

/// \brief mapping of json property names of notifier object from the tracing filter config json file to the
/// corresponding SkeletonFieldTracePointType enum.
constexpr std::array filter_property_skeleton_field_notifier_mappings = {
    // we duplicate "trace_update" currently to also add a trace point for lola specific UPDATE_WITH_ALLOCATE
    // @todo: define a schema extension for "update_with allocate".
    std::pair<const std::string_view, SkeletonFieldTracePointType>{"trace_update",
                                                                   SkeletonFieldTracePointType::UPDATE_WITH_ALLOCATE},
    std::pair<const std::string_view, SkeletonFieldTracePointType>{"trace_update", SkeletonFieldTracePointType::UPDATE},
};

/// \brief mapping of json property names of getter object from the tracing filter config json file to the corresponding
/// SkeletonFieldTracePointType enum.
constexpr std::array filter_property_skeleton_field_getter_mappings = {
    std::pair<const std::string_view, SkeletonFieldTracePointType>{"trace_request_received",
                                                                   SkeletonFieldTracePointType::GET_CALL},
    std::pair<const std::string_view, SkeletonFieldTracePointType>{"trace_response_send",
                                                                   SkeletonFieldTracePointType::GET_CALL_RESULT},
};

/// \brief mapping of json property names of setter object from the tracing filter config json file to the corresponding
/// SkeletonFieldTracePointType enum.
constexpr std::array filter_property_skeleton_field_setter_mappings = {
    std::pair<const std::string_view, SkeletonFieldTracePointType>{"trace_request_received",
                                                                   SkeletonFieldTracePointType::SET_CALL},
    std::pair<const std::string_view, SkeletonFieldTracePointType>{"trace_response_send",
                                                                   SkeletonFieldTracePointType::SET_CALL_RESULT},
};

/// \brief Checks the optional bool property with the given name in the given json object. If it doesn't exist, it
///        returns false, otherwise it returns the bool value it finds.
/// \param json json object to check for bool prop.
/// \param bool_property_name name of bool prop
/// \return value of property or false if it doesn't exist.
bool IsOptionalBoolPropertyEnabled(const bmw::json::Object& json, const std::string_view bool_property_name)
{
    const auto& bool_prop_object = json.find(bool_property_name);
    if (bool_prop_object != json.cend())
    {
        return bool_prop_object->second.As<bool>().value();
    }
    return false;
}

/// \brief Returns the configured instances (within our mw_com_config.json) of the given service type
/// \param configuration configuration, where to do the lookup
/// \param service_type identification of the service type (which is an AUTOSAR short-name-path representation)
/// \return set of string_views reflecting an InstanceSpecifier. Those string_views reference into strings held by
///         our single/global Configuration object. Their lifetime is the same as the LoLa runtime!
std::set<amp::string_view> GetInstancesOfServiceType(const Configuration& configuration, amp::string_view service_type)
{
    std::set<amp::string_view> result{};
    for (const auto& service_instance_element : configuration.GetServiceInstances())
    {
        if (service_instance_element.second.service_.ToString() == service_type)
        {
            result.insert(service_instance_element.first.ToString());
        }
    }
    return result;
}

/// \brief Returns a set of element names, used within the given service_type. The names in the set are string_views
///        pointing to strings owned by members of Configuration. So the life-time of those string-views is bound to
///        the life-time of our single/global Configuration object held within the Runtime.
/// \param service_type service type from which to get element names
/// \param element_type element type of which to get names
/// \param configuration configuration object to get consulted for the search/lookup
/// \return a set of string_views denoting the service element names.
std::set<amp::string_view> GetElementNamesOfServiceType(const amp::string_view service_type,
                                                        ServiceElementType element_type,
                                                        const Configuration& configuration)
{
    std::set<amp::string_view> result{};
    auto service_type_deployment_visitor = amp::overload(
        [&result, element_type](const LolaServiceTypeDeployment& lola_service_deployment) {
            if (element_type == ServiceElementType::EVENT)
            {
                for (const auto& event : lola_service_deployment.events_)
                {
                    result.insert(event.first);
                }
            }
            else if (element_type == ServiceElementType::FIELD)
            {
                for (const auto& field : lola_service_deployment.fields_)
                {
                    result.insert(field.first);
                }
            }
            else
            {
                bmw::mw::log::LogFatal("lola")
                    << "GetElementNamesOfServiceType called with unsupported ServiceElementType: " << element_type;
                std::terminate();
            }
        },
        [](const amp::blank&) { return; });

    for (const auto& service_type_deployment : configuration.GetServiceTypes())
    {
        const ServiceIdentifierTypeView current_service_type_view{service_type_deployment.first};
        if (current_service_type_view.getInternalTypeName() == service_type)
        {
            amp::visit(service_type_deployment_visitor, service_type_deployment.second.binding_info_);
        }
    }
    return result;
}

///
/// \tparam TP Trace Point Type, one of ProxyEventTracePointType/ProxyFieldTracePointType or
///            SkeletonEventTracePointType/SkeletonFieldTracePointType
/// \param json json object containing the bool property
/// \param bool_prop_name name of the bool property in the json to be checked
/// \param service_type service type identified by its short-name-path for which the trace point shall be added
/// \param service_element_name name of the service within service_type, for which the trace point shall be added.
/// \param instance_id instance id of service-type for which the trace point shall be added.
/// \param trace_point_type detailed trace point type
/// \param filter_config filter_config config, where the addition shall be done.
template <typename TP>
void AddTracePoint(const bmw::json::Object& json,
                   const std::string_view bool_prop_name,
                   amp::string_view service_type,
                   amp::string_view service_element_name,
                   ITracingFilterConfig::InstanceSpecifierView instance_id,
                   TP trace_point_type,
                   TracingFilterConfig& filter_config)
{
    if (IsOptionalBoolPropertyEnabled(json, bool_prop_name))
    {
        filter_config.AddTracePoint(service_type, service_element_name, instance_id, trace_point_type);
    }
}

void ParseEvent(const bmw::json::Any& json,
                amp::string_view service_type,
                const std::set<amp::string_view>& event_names,
                const Configuration& configuration,
                const std::set<amp::string_view>& instance_specifiers,
                TracingFilterConfig& filter_config) noexcept
{
    const auto& object = json.As<bmw::json::Object>().value().get();
    const auto& shortname = object.find(kShortnameKey);
    if (shortname == object.cend())
    {
        bmw::mw::log::LogError("lola")
            << "Trace Filter Configuration: shortname property missing for event in service: " << service_type
            << ". Skipping this event";
    }

    const auto& event_name = shortname->second.As<std::string>().value().get();
    // check if event exists at all on our side. If not silently ignore it according to:
    // [8] Trace Filter Config reference to non-existing trace-point
    if (event_names.count(event_name) == 0)
    {
        return;
    }

    const tracing::ServiceElementIdentifierView service_element_identifier{
        service_type, event_name, ServiceElementType::EVENT};

    for (const auto& instance : instance_specifiers)
    {
        if (configuration.GetTracingConfiguration().IsServiceElementTracingEnabled(service_element_identifier,
                                                                                   instance))
        {
            // trace points for the proxy side
            for (auto& event_prop_mapping : filter_property_proxy_event_mappings)
            {
                AddTracePoint(object,
                              event_prop_mapping.first,
                              service_type,
                              event_name,
                              instance,
                              event_prop_mapping.second,
                              filter_config);
            }
            // trace points for the skeleton side
            for (auto& event_prop_mapping : filter_property_skeleton_event_mappings)
            {
                AddTracePoint(object,
                              event_prop_mapping.first,
                              service_type,
                              event_name,
                              instance,
                              event_prop_mapping.second,
                              filter_config);
            }
            // trace points that are not currently implemented. To be removed in 
            for (const auto& not_implemented_property_name :
                 service_element_notifier_filter_properties_not_implemented_array)
            {
                if (IsOptionalBoolPropertyEnabled(object, not_implemented_property_name))
                {
                    ::bmw::mw::log::LogWarn("lola")
                        << "Event Tracing point:" << not_implemented_property_name
                        << "is currently unsupported. Will be added in . Disabling trace point.";
                }
            }
        }
        else
        {
            ::bmw::mw::log::LogWarn("lola")
                << "Tracing for " << service_element_identifier << " with instance " << instance
                << " has been disabled in mw_com_config but is present in trace filter config file!";
        }
    }
}

void ParseEvents(const bmw::json::Any& json,
                 amp::string_view service_short_name_path,
                 const Configuration& configuration,
                 const std::set<amp::string_view>& instance_specifiers,
                 TracingFilterConfig& filter_config) noexcept
{
    const auto& object = json.As<bmw::json::Object>().value().get();
    const auto& events = object.find(kEventsKey);
    if (events == object.cend())
    {
        // service with no events is fine/ok.
        return;
    }
    else
    {
        const std::set<amp::string_view>& event_names =
            GetElementNamesOfServiceType(service_short_name_path, ServiceElementType::EVENT, configuration);

        for (const auto& event : events->second.As<bmw::json::List>().value().get())
        {
            ParseEvent(event, service_short_name_path, event_names, configuration, instance_specifiers, filter_config);
        }
    }
}

/// \brief In case of fields, the bool props for the various trace-points aren't flat under the field-object, but spread
///        in sub-objects. This helper cares for this case to remove code duplication.
/// \tparam Mapping property-name-to-trace-point-type mapping arrays
/// \param json_object enclosing json object (currently a field-object)
/// \param sub_object_name name of sub-object to be parsed
/// \param service_type service type in which context adding takes place
/// \param service_element_name name of service element (in this case field name)
/// \param instance_id instance
/// \param property_name_trace_point_mappings json-property-name-to-trace-point-type mapping
/// \param filter_config filter config, where the trace-points shall be added.
template <typename Mapping>
void AddTracePointsFromSubObject(const bmw::json::Object& json_object,
                                 const std::string_view sub_object_name,
                                 amp::string_view service_type,
                                 amp::string_view service_element_name,
                                 ITracingFilterConfig::InstanceSpecifierView instance_id,
                                 Mapping& property_name_trace_point_mappings,
                                 TracingFilterConfig& filter_config)
{
    const auto& block = json_object.find(sub_object_name);
    if (block != json_object.cend())
    {
        auto& block_object = block->second.As<bmw::json::Object>().value().get();
        // trace points for the proxy side
        for (auto& prop_mapping : property_name_trace_point_mappings)
        {
            AddTracePoint(block_object,
                          prop_mapping.first,
                          service_type,
                          service_element_name,
                          instance_id,
                          prop_mapping.second,
                          filter_config);
        }
    }
}

/// \todo Helper function that can be removed when support for these tracing points is added in 
void WarnNotImplementedTracePointsFromSubObject(const bmw::json::Object& json_object,
                                                const std::string_view sub_object_name)
{
    const auto& block = json_object.find(sub_object_name);
    if (block != json_object.cend())
    {
        for (auto& not_implemented_property_name : service_element_notifier_filter_properties_not_implemented_array)
        {
            auto& block_object = block->second.As<bmw::json::Object>().value().get();
            if (IsOptionalBoolPropertyEnabled(block_object, not_implemented_property_name))
            {
                ::bmw::mw::log::LogWarn("lola")
                    << "Field Tracing point:" << not_implemented_property_name
                    << "is currently unsupported. Will be added in . Disabling trace point.";
            }
        }
    }
}

void ParseField(const bmw::json::Any& json,
                amp::string_view service_type,
                const std::set<amp::string_view>& field_names,
                const Configuration& configuration,
                const std::set<amp::string_view>& instance_specifiers,
                TracingFilterConfig& filter_config) noexcept
{
    const auto& object = json.As<bmw::json::Object>().value().get();
    const auto& shortname = object.find(kShortnameKey);
    if (shortname == object.cend())
    {
        bmw::mw::log::LogError("lola")
            << "Trace Filter Configuration: shortname property missing for field in service: " << service_type
            << ". Skipping this field";
    }

    const auto& field_name = shortname->second.As<std::string>().value().get();
    // check if field exists at all on our side. If not silently ignore it according to:
    // [8] Trace Filter Config reference to non existing trace-point
    if (field_names.count(field_name) == 0)
    {
        return;
    }

    const tracing::ServiceElementIdentifierView service_element_identifier{
        service_type, field_name, ServiceElementType::FIELD};

    // check, whether service element exists locally

    for (const auto& instance : instance_specifiers)
    {
        if (configuration.GetTracingConfiguration().IsServiceElementTracingEnabled(service_element_identifier,
                                                                                   instance))
        {
            AddTracePointsFromSubObject(object,
                                        kNotifierKey,
                                        service_type,
                                        field_name,
                                        instance,
                                        filter_property_proxy_field_notifier_mappings,
                                        filter_config);
            AddTracePointsFromSubObject(object,
                                        kNotifierKey,
                                        service_type,
                                        field_name,
                                        instance,
                                        filter_property_skeleton_field_notifier_mappings,
                                        filter_config);
            AddTracePointsFromSubObject(object,
                                        kGetterKey,
                                        service_type,
                                        field_name,
                                        instance,
                                        filter_property_proxy_field_getter_mappings,
                                        filter_config);
            AddTracePointsFromSubObject(object,
                                        kGetterKey,
                                        service_type,
                                        field_name,
                                        instance,
                                        filter_property_skeleton_field_getter_mappings,
                                        filter_config);
            AddTracePointsFromSubObject(object,
                                        kSetterKey,
                                        service_type,
                                        field_name,
                                        instance,
                                        filter_property_proxy_field_setter_mappings,
                                        filter_config);
            AddTracePointsFromSubObject(object,
                                        kSetterKey,
                                        service_type,
                                        field_name,
                                        instance,
                                        filter_property_skeleton_field_setter_mappings,
                                        filter_config);
            WarnNotImplementedTracePointsFromSubObject(object, kNotifierKey);
        }
        else
        {
            ::bmw::mw::log::LogWarn("lola")
                << "Tracing for " << service_element_identifier << " with instance " << instance
                << " has been disabled in mw_com_config but is present in trace filter config file!";
        }
    }
}

void ParseFields(const bmw::json::Any& json,
                 amp::string_view service_short_name_path,
                 const Configuration& configuration,
                 const std::set<amp::string_view>& instance_specifiers,
                 TracingFilterConfig& filter_config) noexcept
{
    const auto& object = json.As<bmw::json::Object>().value().get();
    const auto& fields = object.find(kFieldsKey);
    if (fields == object.cend())
    {
        // service with no fields is fine/ok.
        return;
    }
    else
    {
        const std::set<amp::string_view>& field_names =
            GetElementNamesOfServiceType(service_short_name_path, ServiceElementType::FIELD, configuration);

        for (const auto& field : fields->second.As<bmw::json::List>().value().get())
        {
            ParseField(field, service_short_name_path, field_names, configuration, instance_specifiers, filter_config);
        }
    }
}

void ParseMethods(const bmw::json::Any& json,
                  amp::string_view /*service_short_name_path*/,
                  const Configuration& /*configuration*/,
                  const std::set<amp::string_view>& /*instance_specifiers*/,
                  TracingFilterConfig& /*filter_config*/) noexcept
{
    const auto& object = json.As<bmw::json::Object>().value().get();
    const auto& methods = object.find(kMethodsKey);
    if (methods == object.cend())
    {
        // service with no methods is fine/ok.
        return;
    }
    else
    {
        // @todo: Implement ParseMethod, when we support methods in LoLa.
        /* for (const auto& method : methods->second.As<bmw::json::List>().value().get())
        {

            break;
        }*/
    }
}

void ParseService(const bmw::json::Any& json,
                  const std::set<amp::string_view>& configured_service_types,
                  const Configuration& configuration,
                  TracingFilterConfig& filter_config) noexcept
{
    const auto& object = json.As<bmw::json::Object>().value().get();
    const auto& shortname_path = object.find(kShortnamePathKey);
    if (shortname_path == object.cend())
    {
        bmw::mw::log::LogError("lola") << "Trace Filter Configuration: shortname_path property missing for service!";
        return;
    }

    const auto& shortname_path_string = shortname_path->second.As<std::string>().value().get();
    if (configured_service_types.count(shortname_path_string) > 0)
    {
        // determine the configured service-instances of the given service-type
        const auto instance_specifiers = GetInstancesOfServiceType(configuration, shortname_path_string);

        ParseEvents(json, shortname_path_string, configuration, instance_specifiers, filter_config);
        ParseFields(json, shortname_path_string, configuration, instance_specifiers, filter_config);
        ParseMethods(json, shortname_path_string, configuration, instance_specifiers, filter_config);
    }
}

bmw::Result<TracingFilterConfig> ParseServices(const bmw::json::Any& json, const Configuration& configuration) noexcept
{
    TracingFilterConfig tracing_filter_config{};
    const auto& object = json.As<bmw::json::Object>().value().get();
    const auto& services = object.find(kServicesKey);
    if (services == object.cend())
    {
        // even if it is "weird" having a filter-config without any service in it ... it is valid/ok
        return tracing_filter_config;
    }

    // which service types are configured locally in mw::com/LoLa?
    std::set<amp::string_view> configured_service_types{};
    for (const auto& map_entry : configuration.GetServiceTypes())
    {
        configured_service_types.insert(map_entry.first.ToString());
    }

    for (const auto& service : services->second.As<bmw::json::List>().value().get())
    {
        ParseService(service, configured_service_types, configuration, tracing_filter_config);
    }
    return tracing_filter_config;
}

}  // namespace

bmw::Result<TracingFilterConfig> Parse(const amp::string_view path, const Configuration& configuration) noexcept
{
    const bmw::json::JsonParser json_parser_obj{};
    // Reason for banning is AoU of vaJson library about integrity of provided path.
    // This AoU is forwarded as AoU of Lola. See 
    // NOLINTNEXTLINE(bmw-banned-function): The user has to guarantee the integrity of the path
    auto json_result = json_parser_obj.FromFile(path);
    if (!json_result.has_value())
    {
        ::bmw::mw::log::LogFatal("lola") << "Parsing trace filter config file" << path
                                         << "failed with error:" << json_result.error().Message() << ": "
                                         << json_result.error().UserMessage() << " . Terminating.";
        return MakeUnexpected(TraceErrorCode::JsonConfigParseError, json_result.error().UserMessage().data());
    }
    return Parse(std::move(json_result).value(), configuration);
}

bmw::Result<TracingFilterConfig> Parse(bmw::json::Any json, const Configuration& configuration) noexcept
{
    return ParseServices(json, configuration);
}

}  // namespace bmw::mw::com::impl::tracing
