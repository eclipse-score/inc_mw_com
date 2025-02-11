// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#include "platform/aas/mw/com/impl/bindings/lola/subscription_helpers.h"

#include "platform/aas/mw/com/impl/bindings/lola/i_runtime.h"
#include "platform/aas/mw/com/impl/runtime.h"

#include "platform/aas/mw/log/logging.h"

#include <amp_assert.hpp>

#include <exception>
#include <sstream>
#include <string>
#include <utility>

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

namespace
{

using HandlerRegistrationNoType = IMessagePassingService::HandlerRegistrationNoType;

lola::IRuntime& GetLolaRuntime() noexcept
{
    auto* lola_runtime = dynamic_cast<lola::IRuntime*>(Runtime::getInstance().GetBindingRuntime(BindingType::kLoLa));
    AMP_ASSERT_PRD_MESSAGE(lola_runtime != nullptr, "Lola runtime does not exist.");
    return *lola_runtime;
}

}  // namespace

void EventReceiveHandlerManager::Register(BindingEventReceiveHandler handler) noexcept
{
    Unregister();
    auto& lola_runtime = GetLolaRuntime();
    registration_number_ = lola_runtime.GetLolaMessaging().RegisterEventNotification(
        asil_level_, element_fq_id_, std::move(handler), event_source_pid_);
}

void EventReceiveHandlerManager::Reregister(
    amp::optional<BindingEventReceiveHandler> new_event_receiver_handler) noexcept
{
    if (new_event_receiver_handler.has_value())
    {
        Register(std::move(new_event_receiver_handler.value()));
    }
    else if (registration_number_.has_value())
    {
        auto& lola_runtime = GetLolaRuntime();
        lola_runtime.GetLolaMessaging().ReregisterEventNotification(asil_level_, element_fq_id_, event_source_pid_);
    }
}

void EventReceiveHandlerManager::Unregister() noexcept
{
    if (registration_number_.has_value())
    {
        auto& lola_runtime = GetLolaRuntime();
        lola_runtime.GetLolaMessaging().UnregisterEventNotification(
            asil_level_, element_fq_id_, registration_number_.value(), event_source_pid_);
        registration_number_ = amp::nullopt;
    }
}

std::string CreateLoggingString(std::string&& string,
                                const ElementFqId& element_fq_id,
                                const SubscriptionStateMachineState current_state)
{
    std::stringstream s;
    s << string << " " << element_fq_id.ToString() << MessageForSubscriptionState(current_state);
    return s.str();
}

}  // namespace lola
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
