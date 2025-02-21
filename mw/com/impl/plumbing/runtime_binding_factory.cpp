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


#include "platform/aas/mw/com/impl/plumbing/runtime_binding_factory.h"

#include "platform/aas/mw/com/impl/configuration/lola_service_type_deployment.h"

#include "platform/aas/mw/com/impl/bindings/lola/runtime.h"
#include "platform/aas/mw/com/impl/bindings/lola/tracing/tracing_runtime.h"

#include <amp_overload.hpp>
#include <amp_utility.hpp>

#include <unordered_map>
#include <utility>

std::unordered_map<bmw::mw::com::impl::BindingType, std::unique_ptr<bmw::mw::com::impl::IRuntimeBinding>>
bmw::mw::com::impl::RuntimeBindingFactory::CreateBindingRuntimes(
    Configuration& configuration,
    concurrency::Executor& long_running_threads,
    const amp::optional<tracing::TracingFilterConfig>& tracing_filter_config)
{
    std::unordered_map<BindingType, std::unique_ptr<IRuntimeBinding>> result{};

    // Iterate through all the service types we have to find out, which technical bindings are used.
    for (auto it = configuration.GetServiceTypes().cbegin(); it != configuration.GetServiceTypes().cend(); ++it)
    {
        auto service_type = *it;

        
        /* The lambda will be executed within this stack. Thus, all references are still valid */
        auto deployment_info_visitor = amp::overload(
            [&result, &configuration, &long_running_threads, &tracing_filter_config](const LolaServiceTypeDeployment&) {
                std::unique_ptr<lola::tracing::TracingRuntime> lola_tracing_runtime{nullptr};
                if (configuration.GetTracingConfiguration().IsTracingEnabled() && tracing_filter_config.has_value())
                {
                    const auto number_of_service_elements_with_trace_done_cb =
                        tracing_filter_config->GetNumberOfServiceElementsWithTraceDoneCB();
                    lola_tracing_runtime = std::make_unique<lola::tracing::TracingRuntime>(
                        number_of_service_elements_with_trace_done_cb, configuration);
                }

                auto lola_runtime = std::make_unique<bmw::mw::com::impl::lola::Runtime>(
                    configuration, long_running_threads, std::move(lola_tracing_runtime));
                const auto pair = result.emplace(std::make_pair(BindingType::kLoLa, std::move(lola_runtime)));
                
                AMP_ASSERT_PRD_MESSAGE(pair.second, "Failed to emplace lola runtime binding");
                
                return true;
            },
            [](const amp::blank&) { return false; });
        
        const bool runtime_created = amp::visit(deployment_info_visitor, service_type.second.binding_info_);

        // currently, (with only one binding support) we can immediately stop further search.
        if (runtime_created)
        {
            break;
        }
    }
    return result;
}
