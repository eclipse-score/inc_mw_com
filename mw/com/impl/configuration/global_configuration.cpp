// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#include "platform/aas/mw/com/impl/configuration/global_configuration.h"

#include "platform/aas/mw/log/logging.h"

#include <exception>

namespace bmw
{
namespace mw
{
namespace com
{
namespace impl
{

GlobalConfiguration::GlobalConfiguration() noexcept
    : process_asil_level_{QualityType::kASIL_QM},
      message_rx_queue_size_qm{DEFAULT_MIN_NUM_MESSAGES_RX_QUEUE},
      message_rx_queue_size_b{DEFAULT_MIN_NUM_MESSAGES_RX_QUEUE},
      message_tx_queue_size_b{DEFAULT_MIN_NUM_MESSAGES_TX_QUEUE},
      shm_size_calc_mode_{ShmSizeCalculationMode::kSimulation}
{
}

std::int32_t GlobalConfiguration::GetReceiverMessageQueueSize(const QualityType quality_type) const noexcept
{
    switch (quality_type)
    {
        //  std::terminate will terminate this switch clause
        case QualityType::kInvalid:
            ::bmw::mw::log::LogFatal("lola") << "Invalid ASIL in global/asil-level, terminating.";
            std::terminate();
        //  Return will terminate this switch clause
        case QualityType::kASIL_QM:
            return message_rx_queue_size_qm;
        //  Return will terminate this switch clause
        case QualityType::kASIL_B:
            return message_rx_queue_size_b;
        //  std::terminate will terminate this switch clause
        default:
            ::bmw::mw::log::LogFatal("lola") << "Unknown ASIL parsed from config, terminating";
            std::terminate();
    }
}

void GlobalConfiguration::SetReceiverMessageQueueSize(const QualityType quality_type,
                                                      const std::int32_t queue_size) noexcept
{
    switch (quality_type)
    {
        //  std::terminate will terminate this switch clause
        case QualityType::kInvalid:
            ::bmw::mw::log::LogFatal("lola") << "Invalid ASIL in global/asil-level, terminating.";
            std::terminate();
        case QualityType::kASIL_QM:
            message_rx_queue_size_qm = queue_size;
            break;
        case QualityType::kASIL_B:
            message_rx_queue_size_b = queue_size;
            break;
        //  std::terminate will terminate this switch clause
        default:
            ::bmw::mw::log::LogFatal("lola") << "Unknown ASIL parsed from config, terminating";
            std::terminate();
    }
}

void GlobalConfiguration::SetShmSizeCalcMode(const ShmSizeCalculationMode shm_size_calc_mode) noexcept
{
    shm_size_calc_mode_ = shm_size_calc_mode;
}

}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
