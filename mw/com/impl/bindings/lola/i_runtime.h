// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#ifndef PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_I_RUNTIME_H
#define PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_I_RUNTIME_H

#include "platform/aas/mw/com/impl/bindings/lola/messaging/i_message_passing_service.h"
#include "platform/aas/mw/com/impl/bindings/lola/rollback_data.h"
#include "platform/aas/mw/com/impl/configuration/shm_size_calc_mode.h"
#include "platform/aas/mw/com/impl/i_runtime_binding.h"

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

/// \brief LoLa binding specific runtime interface
class IRuntime : public impl::IRuntimeBinding
{
  public:
    /// \brief returns the message passing service instance needed by/used by LoLa skeletons/proxies.
    /// \return message passing service instance
    virtual lola::IMessagePassingService& GetLolaMessaging() = 0;

    /// \brief returns, whether LoLa binding runtime was created with ASIL-B support
    virtual bool HasAsilBSupport() const noexcept = 0;

    /// \brief returns configured mode, how shm-sizes shall be calculated.
    virtual ShmSizeCalculationMode GetShmSizeCalculationMode() const = 0;

    virtual RollbackData& GetRollbackData() noexcept = 0;

    /// \brief We need our PID in several locations/frequently. So the runtime shall provide/cache it.
    virtual pid_t GetPid() const noexcept = 0;
    /// \brief We need our UID in several locations/frequently. So the runtime shall provide/cache it.
    virtual uid_t GetUid() const noexcept = 0;
};

}  // namespace lola
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_I_RUNTIME_H
