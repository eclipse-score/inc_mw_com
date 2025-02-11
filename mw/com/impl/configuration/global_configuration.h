// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#ifndef PLATFORM_AAS_MW_COM_IMPL_CONFIGURATION_GLOBAL_CONFIGURATION_H
#define PLATFORM_AAS_MW_COM_IMPL_CONFIGURATION_GLOBAL_CONFIGURATION_H

#include "platform/aas/mw/com/impl/configuration/quality_type.h"
#include "platform/aas/mw/com/impl/configuration/shm_size_calc_mode.h"

#include <cstdint>

namespace bmw
{
namespace mw
{
namespace com
{
namespace impl
{

class GlobalConfiguration final
{
  public:
    // default value for ASIL-QM and ASIL-B receive message queue sizes.
    static constexpr std::int32_t DEFAULT_MIN_NUM_MESSAGES_RX_QUEUE{10};
    // default value for ASIL-B send message queue sizes.
    static constexpr std::int32_t DEFAULT_MIN_NUM_MESSAGES_TX_QUEUE{20};

    GlobalConfiguration() noexcept;

    /**
     * \brief Class is only move constructible
     */
    GlobalConfiguration(const GlobalConfiguration& other) = delete;
    GlobalConfiguration(GlobalConfiguration&& other) = default;
    GlobalConfiguration& operator=(const GlobalConfiguration& other) & = delete;
    GlobalConfiguration& operator=(GlobalConfiguration&& other) & = default;

    ~GlobalConfiguration() noexcept = default;

    void SetProcessAsilLevel(const QualityType process_asil_level) { process_asil_level_ = process_asil_level; }

    void SetReceiverMessageQueueSize(const QualityType quality_type, const std::int32_t queue_size) noexcept;

    void SetSenderMessageQueueSize(const std::int32_t queue_size) noexcept { message_tx_queue_size_b = queue_size; }

    void SetShmSizeCalcMode(const ShmSizeCalculationMode shm_size_calc_mode) noexcept;

    std::int32_t GetReceiverMessageQueueSize(const QualityType quality_type) const noexcept;

    std::int32_t GetSenderMessageQueueSize() const noexcept { return message_tx_queue_size_b; }

    QualityType GetProcessAsilLevel() const { return process_asil_level_; }

    ShmSizeCalculationMode GetShmSizeCalcMode() const noexcept { return shm_size_calc_mode_; }

  private:
    /// properties/settings from the "global" section
    QualityType process_asil_level_;

    std::int32_t message_rx_queue_size_qm;
    std::int32_t message_rx_queue_size_b;
    std::int32_t message_tx_queue_size_b;

    ShmSizeCalculationMode shm_size_calc_mode_;
};

}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_CONFIGURATION_GLOBAL_CONFIGURATION_H
