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


#ifndef PLATFORM_AAS_MW_COM_IMPL_PLUMBING_RUNTIME_BINDING_FACTORY_H
#define PLATFORM_AAS_MW_COM_IMPL_PLUMBING_RUNTIME_BINDING_FACTORY_H

#include "platform/aas/mw/com/impl/configuration/configuration.h"
#include "platform/aas/mw/com/impl/i_runtime_binding.h"
#include "platform/aas/mw/com/impl/tracing/configuration/tracing_filter_config.h"

#include <amp_optional.hpp>

#include "platform/aas/lib/concurrency/executor.h"

#include <map>
#include <memory>
#include <unordered_map>

namespace bmw
{
namespace mw
{
namespace com
{
namespace impl
{

class RuntimeBindingFactory final
{
  public:
    static std::unordered_map<BindingType, std::unique_ptr<IRuntimeBinding>> CreateBindingRuntimes(
        Configuration& configuration,
        concurrency::Executor& long_running_threads,
        const amp::optional<tracing::TracingFilterConfig>& tracing_filter_config);
};

}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_PLUMBING_RUNTIME_BINDING_FACTORY_H
