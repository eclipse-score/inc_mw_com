// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#include "platform/aas/mw/com/impl/bindings/lola/runtime.h"

#include "platform/aas/lib/memory/shared/offset_ptr.h"
#include "platform/aas/lib/os/unistd.h"
#include "platform/aas/mw/log/logging.h"

#include <amp_assert.hpp>
#include <amp_utility.hpp>

#include <exception>
#include <set>
#include <unordered_map>
#include <vector>

namespace bmw
{
namespace mw
{
namespace com
{
namespace impl
{
namespace lola
{


Runtime::Runtime(const Configuration& config,
                 concurrency::Executor& long_running_threads,
                 std::unique_ptr<lola::tracing::TracingRuntime> lola_tracing_runtime)
    : IRuntime{},
      configuration_{config},
      long_running_threads_{long_running_threads},
      lola_message_passing_control_{Runtime::HasAsilBSupport(),
                                    config.GetGlobalConfiguration().GetSenderMessageQueueSize()},
      lola_messaging_{lola_message_passing_control_,
                      Runtime::GetMessagePassingCfg(QualityType::kASIL_QM),
                      Runtime::HasAsilBSupport()
                          ? amp::optional<MessagePassingFacade::AsilSpecificCfg>{Runtime::GetMessagePassingCfg(
                                QualityType::kASIL_B)}
                          : amp::nullopt},
      service_discovery_client_{long_running_threads_},
      tracing_runtime_{std::move(lola_tracing_runtime)},
      rollback_data_{},
      pid_{os::Unistd::instance().getpid()},
      uid_{os::Unistd::instance().getuid()}


{
    // At this stage we know/can decide, whether we are an ASIL-B or ASIL-QM application. OffsetPtr bounds-checking
    // is costly and is only done in case we are an ASIL-B app!
    bmw::memory::shared::EnableOffsetPtrBoundsChecking(Runtime::HasAsilBSupport());
}

BindingType Runtime::GetBindingType() const noexcept
{
    return BindingType::kLoLa;
}


IMessagePassingService& Runtime::GetLolaMessaging()
{
    return lola_messaging_;
}


bool Runtime::HasAsilBSupport() const noexcept
{
    return configuration_.GetGlobalConfiguration().GetProcessAsilLevel() == QualityType::kASIL_B;
}

impl::tracing::ITracingRuntimeBinding* Runtime::GetTracingRuntime() noexcept
{
    return tracing_runtime_.get();
}

MessagePassingFacade::AsilSpecificCfg Runtime::GetMessagePassingCfg(const QualityType asil_level) const
{
    AMP_ASSERT_PRD_MESSAGE(((asil_level == QualityType::kASIL_B) || (asil_level == QualityType::kASIL_QM)),
                           "Asil level must be asil_qm or asil_b.");
    if ((!HasAsilBSupport()) && (asil_level == QualityType::kASIL_B))
    {
        bmw::mw::log::LogFatal("lola")
            << __func__ << __LINE__
            << "Invalid call to GetMessagePassingCfg with asil_level B although app/process is configured for QM only.";
        
        /* Terminate call tolerated.See Assumptions of Use in mw/com/design/README.md*/
        std::terminate();
        
    }
    std::set<uid_t> aggregated_allowed_users;

    for (const auto& instanceDeplElement : configuration_.GetServiceInstances())
    {
        const auto* const instance_deployment =
            amp::get_if<LolaServiceInstanceDeployment>(&instanceDeplElement.second.bindingInfo_);
        if (instance_deployment != nullptr)
        {
            if (AggregateAllowedUsers(aggregated_allowed_users, instance_deployment->allowed_consumer_, asil_level) ==
                true)
            {
                break;
            }
            if (AggregateAllowedUsers(aggregated_allowed_users, instance_deployment->allowed_provider_, asil_level) ==
                true)
            {
                break;
            }
        }
    }

    return {configuration_.GetGlobalConfiguration().GetReceiverMessageQueueSize(asil_level),
            std::vector<uid_t>(aggregated_allowed_users.begin(), aggregated_allowed_users.end())};
}

bool Runtime::AggregateAllowedUsers(std::set<uid_t>& aggregated_allowed_users,
                                    const std::unordered_map<QualityType, std::vector<uid_t>>& allowed_user_ids,
                                    const QualityType asil_level)
{
    const auto user_ids = allowed_user_ids.find(asil_level);
    if (user_ids != allowed_user_ids.cend())
    {
        if (user_ids->second.empty())
        {
            aggregated_allowed_users.clear();
            return true;
        }
        for (const auto& user_identifier : user_ids->second)
        {
            // result of insert can be ignored, because at the end the element is in
            // the set and we have no expectation, if it already was in the set before or not.
            amp::ignore = aggregated_allowed_users.insert(user_identifier);
        }
    }
    return false;
}

ShmSizeCalculationMode Runtime::GetShmSizeCalculationMode() const
{
    return configuration_.GetGlobalConfiguration().GetShmSizeCalcMode();
}

IServiceDiscoveryClient& Runtime::GetServiceDiscoveryClient() noexcept
{
    return service_discovery_client_;
}

RollbackData& Runtime::GetRollbackData() noexcept
{
    return rollback_data_;
}

pid_t Runtime::GetPid() const noexcept
{
    return pid_;
}

uid_t Runtime::GetUid() const noexcept
{
    return uid_;
}

}  // namespace lola
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
