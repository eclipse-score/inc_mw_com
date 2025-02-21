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


#include "platform/aas/mw/com/impl/runtime.h"

#include "platform/aas/lib/memory/shared/memory_resource_registry.h"
#include "platform/aas/mw/com/impl/configuration/config_parser.h"
#include "platform/aas/mw/com/impl/instance_specifier.h"
#include "platform/aas/mw/com/impl/plumbing/runtime_binding_factory.h"
#include "platform/aas/mw/com/impl/tracing/configuration/tracing_filter_config_parser.h"
#include "platform/aas/mw/com/impl/tracing/i_tracing_runtime_binding.h"
#include "platform/aas/mw/log/logging.h"
#include "platform/aas/mw/log/runtime.h"

#include <amp_assert.hpp>
#include <amp_utility.hpp>

#include <iostream>
#include <string>
#include <utility>

namespace
{
constexpr auto default_manifest_path = "./etc/mw_com_config.json";
constexpr auto ServiceInstanceManifestOption = "-service_instance_manifest";

inline void warn_double_init()
{
    bmw::mw::log::LogWarn("lola") << "bmw::mw::com::impl::Runtime is already initialized! Redundant call to a "
                                     "Runtime::Initialize() overload within production code needs to be checked.";
}

inline void error_double_init()
{
    bmw::mw::log::LogError("lola")
        << "bmw::mw::com::impl::Runtime is already initialized and locked! Redundant call to a "
           "Runtime::Initialize() overload without effect within production code needs to be checked.";
}

/// \brief Forces initialization of all static dependencies our static Runtime has.
/// \details To avoid a static destruction order fiasco, where we access objects, which are located in other static
///          contexts,  from our Runtime static context, when those other static contexts have already been destroyed,
///          we "touch" those other static contexts (make sure that they get initialized) BEFORE our own static Runtime
///          context gets initialized. This way, we make sure, that those other static contexts, where we depend on, are
///          outliving our static context and we are always save to access it.
///          Currently we see two "static dependencies" we do have:
///          - logging: mw::log has some static context and we use this logging facility everywhere in our mw::com code.
///          - MemoryResourceRegistry of lib/memory/shared: This is also a static singleton and all our
///          proxies/skeletons
///            depend on it, as e.g. in their destructors we are unregistering memory-resources from
///            MemoryResourceRegistry. mw::com/LoLa supports/allows, that proxy/skeleton instances are residing in the
///            static context of our impl::Runtime! (We do only forbid via AoU, that users put proxies/skeletons in
///            their own static context).
///            Creating proxies/skeletons in our static Runtime context might implicitly happen (is allowed), when a
///            user creates e.g. a proxy within a FidService-callback. This callback is then handed over to the
///            StartFindService-API and stored in our service discovery, which is part of our static Runtime context!
///            So this callback will be executed in our static Runtime context and we have to make sure, that
///            MemoryResourceRegistry is available.
void TouchStaticDependencies()
{
    amp::ignore = bmw::mw::log::detail::Runtime::GetRecorder();
    amp::ignore = bmw::memory::shared::MemoryResourceRegistry::getInstance();
}

}  // namespace

namespace bmw
{
namespace mw
{
namespace com
{
namespace impl
{

using TracingFilterConfig = tracing::TracingFilterConfig;

IRuntime* Runtime::mock_ = nullptr;
amp::optional<Configuration> Runtime::initialization_config_{};
bool Runtime::runtime_initialization_locked_{false};

std::mutex bmw::mw::com::impl::Runtime::mutex_{};

void Runtime::Initialize() noexcept
{
    std::lock_guard<std::mutex> lock{mutex_};
    if (runtime_initialization_locked_)
    {
        error_double_init();
        return;
    }
    if (initialization_config_.has_value())
    {
        warn_double_init();
    }
    auto config = configuration::Parse(default_manifest_path);
    StoreConfiguration(std::move(config));
}

void Runtime::Initialize(const std::string& buffer)
{
    std::lock_guard<std::mutex> lock{mutex_};
    if (runtime_initialization_locked_)
    {
        error_double_init();
        return;
    }
    if (initialization_config_.has_value())
    {
        warn_double_init();
    }

    const bmw::json::JsonParser json_parser_obj;
    auto json = json_parser_obj.FromBuffer(buffer);
    if (!json.has_value())
    {
        std::cerr << "Error Parsing JSON" << json.error() << std::endl;
        
        /* Terminate call tolerated.See Assumptions of Use in mw/com/design/README.md*/
        std::terminate();
        
    }
    auto config = configuration::Parse(std::move(json).value());
    StoreConfiguration(std::move(config));
}

void Runtime::Initialize(const amp::span<const bmw::StringLiteral> arguments)
{
    std::lock_guard<std::mutex> lock{mutex_};
    if (runtime_initialization_locked_)
    {
        error_double_init();
        return;
    }

    if (initialization_config_.has_value())
    {
        warn_double_init();
    }

    const auto num_args = arguments.size();
    bmw::StringLiteral manifestFilePath = nullptr;
    for (std::int32_t argNum = 0; argNum < num_args; argNum++)
    {
        const std::string inputServiceInstanceManifestOption{amp::at(arguments, argNum)};
        if (inputServiceInstanceManifestOption == ServiceInstanceManifestOption)
        {
            if (argNum + 1 < num_args)
            {
                manifestFilePath = amp::at(arguments, argNum + 1);
            }
            break;
        }
    }

    if (manifestFilePath == nullptr)
    {
        manifestFilePath = default_manifest_path;
    }

    auto config = configuration::Parse(manifestFilePath);
    StoreConfiguration(std::move(config));
}

auto Runtime::getInstance() -> IRuntime&
{
    if (mock_ != nullptr)
    {
        return *mock_;
    }
    else
    {
        return getInstanceInternal();
    }
}

amp::optional<TracingFilterConfig> ParseTraceConfig(const Configuration& configuration)
{
    if (!configuration.GetTracingConfiguration().IsTracingEnabled())
    {
        return {};
    }
    const auto trace_filter_config_path = configuration.GetTracingConfiguration().GetTracingFilterConfigPath();
    bmw::Result<TracingFilterConfig> tracing_config_result = tracing::Parse(trace_filter_config_path, configuration);
    if (!tracing_config_result.has_value())
    {
        bmw::mw::log::LogError("lola") << "Parsing tracing config failed with error: " << tracing_config_result.error();
        return {};
    }
    return std::move(tracing_config_result).value();
}

Runtime& Runtime::getInstanceInternal()
{
    TouchStaticDependencies();
    static Runtime instance{([]() -> std::pair<Configuration, amp::optional<TracingFilterConfig>> {
        std::lock_guard<std::mutex> lock{mutex_};
        runtime_initialization_locked_ = true;
        if (!initialization_config_.has_value())
        {
            auto configuration = configuration::Parse(default_manifest_path);
            auto tracing_config = ParseTraceConfig(configuration);
            return std::make_pair(std::move(configuration), std::move(tracing_config));
        }
        else
        {
            auto tracing_config = ParseTraceConfig(initialization_config_.value());
            auto configuration_pair =
                std::make_pair(std::move(initialization_config_.value()), std::move(tracing_config));
            initialization_config_ = {};
            return configuration_pair;
        }
    })()};
    return instance;
}

Runtime::Runtime(std::pair<Configuration&&, amp::optional<TracingFilterConfig>&&> configs)
    : IRuntime{},
      configuration_{std::move(std::get<0>(configs))},
      tracing_filter_configuration_{std::move(std::get<1>(configs))},
      tracing_runtime_{nullptr},
      service_discovery_{*this},
      long_running_threads_{}
{
    runtime_bindings_ = RuntimeBindingFactory::CreateBindingRuntimes(
        configuration_, long_running_threads_, tracing_filter_configuration_);
    if (configuration_.GetTracingConfiguration().IsTracingEnabled())
    {
        if (tracing_filter_configuration_.has_value())
        {
            std::unordered_map<BindingType, tracing::ITracingRuntimeBinding*> tracing_runtime_bindings{};
            for (auto& runtime_binding : runtime_bindings_)
            {
                AMP_ASSERT_PRD_MESSAGE(runtime_binding.second->GetTracingRuntime() != nullptr,
                                       "Binding specific runtime has no tracing runtime although tracing is enabled!");
                tracing_runtime_bindings.emplace(runtime_binding.first, runtime_binding.second->GetTracingRuntime());
            }
            tracing_runtime_ = std::make_unique<tracing::TracingRuntime>(std::move(tracing_runtime_bindings));
        }
    }
}

Runtime::~Runtime() noexcept
{
    mw::log::LogDebug("lola") << "Starting destrcution of mw::com runtime";
}

std::vector<InstanceIdentifier> Runtime::resolve(const InstanceSpecifier& specifier) const
{
    std::vector<InstanceIdentifier> result;
    const auto instanceSearch = configuration_.GetServiceInstances().find(specifier);
    if (instanceSearch != configuration_.GetServiceInstances().end())
    {
        // @todo: Right now we don't support multi-binding, if we do, we need to have some kind of loop
        const auto type_deployment = configuration_.GetServiceTypes().find(instanceSearch->second.service_);
        if (type_deployment != configuration_.GetServiceTypes().cend())
        {
            result.push_back(make_InstanceIdentifier(instanceSearch->second, type_deployment->second));
        }
        else
        {
            // @todo error handling
        }
    }

    return result;
}

auto Runtime::GetBindingRuntime(const BindingType binding) const -> IRuntimeBinding*
{
    auto search = runtime_bindings_.find(binding);
    if (search != runtime_bindings_.cend())
    {
        return search->second.get();
    }
    return nullptr;
}

auto Runtime::GetServiceDiscovery() noexcept -> IServiceDiscovery&
{
    // The signature of this function is part of the detailled design.
    // 
    return service_discovery_;
}

auto Runtime::GetTracingFilterConfig() const noexcept -> const tracing::ITracingFilterConfig*
{
    if (!tracing_filter_configuration_.has_value())
    {
        return nullptr;
    }
    return &tracing_filter_configuration_.value();
}

tracing::ITracingRuntime* Runtime::GetTracingRuntime() const noexcept
{
    return tracing_runtime_.get();
}

void Runtime::InjectMock(IRuntime* const mock) noexcept
{
    mock_ = mock;
}

void Runtime::StoreConfiguration(Configuration config) noexcept
{
    amp::ignore = initialization_config_.emplace(std::move(config));
    InstanceIdentifier::SetConfiguration(&(*initialization_config_));
}

}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
