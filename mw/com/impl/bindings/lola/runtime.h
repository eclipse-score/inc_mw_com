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


#ifndef PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_RUNTIME_H
#define PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_RUNTIME_H

#include "platform/aas/mw/com/impl/bindings/lola/i_runtime.h"
#include "platform/aas/mw/com/impl/bindings/lola/messaging/message_passing_control.h"
#include "platform/aas/mw/com/impl/bindings/lola/messaging/message_passing_facade.h"
#include "platform/aas/mw/com/impl/bindings/lola/rollback_data.h"
#include "platform/aas/mw/com/impl/bindings/lola/service_discovery_client.h"
#include "platform/aas/mw/com/impl/bindings/lola/tracing/tracing_runtime.h"
#include "platform/aas/mw/com/impl/configuration/configuration.h"

#include "platform/aas/lib/concurrency/executor.h"

#include <memory>
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

/// \brief LoLa binding specific implementation of impl::IRuntimeBinding, which holds the infrastructure for LoLa
///        specific messaging used by LoLa skeletons/proxies.
class Runtime final : public IRuntime
{
  public:
    ~Runtime() override = default;

    /// \brief ctor of LoLa specific runtime.
    /// \param config configuration of our mw::com process (also containing lola specific config parameters)
    Runtime(const Configuration& config,
            concurrency::Executor& long_running_threads,
            std::unique_ptr<lola::tracing::TracingRuntime> lola_tracing_runtime);

    /// \brief returns binding type
    /// \return BindingType::kLoLa
    BindingType GetBindingType() const noexcept override;

    /// \brief returns the message passing service instance needed by/used by LoLa skeletons/proxies.
    /// \return message passing service instance
    lola::IMessagePassingService& GetLolaMessaging() override;

    /// \brief returns, whether LoLa binding runtime has been created with ASIL-B support or not
    /// \return _True_ in case Runtime was created with ASIL-B support, _False_ else.
    bool HasAsilBSupport() const noexcept override;

    /// \brief return the lola specific tracing runtime
    /// \return valid pointer to LoLa specific tracing runtime or nullptr in case Runtime has been created without
    ///         tracing support.
    impl::tracing::ITracingRuntimeBinding* GetTracingRuntime() noexcept override;

    /// \brief Read LoLa message passing related configuration from configuration for given ASIL level.
    /// \param asil_level either QM or B. If parameter is set to B although the app/process has been configured to
    ///        be QM, std::terminate() will be called.
    /// \return ASIL level specific message passing cfg.
    MessagePassingFacade::AsilSpecificCfg GetMessagePassingCfg(const QualityType asil_level) const;

    ShmSizeCalculationMode GetShmSizeCalculationMode() const override;

    IServiceDiscoveryClient& GetServiceDiscoveryClient() noexcept override;

    RollbackData& GetRollbackData() noexcept override;

    pid_t GetPid() const noexcept override;

    uid_t GetUid() const noexcept override;

  private:
    const Configuration& configuration_;
    concurrency::Executor& long_running_threads_;
    MessagePassingControl lola_message_passing_control_;
    MessagePassingFacade lola_messaging_;
    ServiceDiscoveryClient service_discovery_client_;
    std::unique_ptr<lola::tracing::TracingRuntime> tracing_runtime_;
    RollbackData rollback_data_;

    /// \brief Helper func aggregates allowed_user_ids of the given quality type into aggregated_allowed_users. If
    ///        allowed_user_ids is empty (no access restriction!), then aggregated_allowed_users is cleared!
    /// \param aggregated_allowed_users aggregated user ids (for access control) for the given asil_level
    /// \param allowed_user_ids user ids to be aggregated/added into aggregated_allowed_users
    /// \param asil_level asil level
    /// \return true, in case aggregated_allowed_users has been cleared
    static bool AggregateAllowedUsers(std::set<uid_t>& aggregated_allowed_users,
                                      const std::unordered_map<QualityType, std::vector<uid_t>>& allowed_user_ids,
                                      const QualityType asil_level);
    pid_t pid_;
    uid_t uid_;
};

}  // namespace lola
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_RUNTIME_H
