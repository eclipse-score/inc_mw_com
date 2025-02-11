// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#include "platform/aas/mw/com/runtime.h"
#include "platform/aas/lib/result/result.h"
#include "platform/aas/mw/com/impl/com_error.h"
#include "platform/aas/mw/com/impl/runtime.h"

#include <amp_span.hpp>

#include <cstdint>

bmw::Result<bmw::mw::com::InstanceIdentifierContainer> bmw::mw::com::runtime::ResolveInstanceIDs(
    const impl::InstanceSpecifier model_name)
{
    const auto instance_identifier_container = bmw::mw::com::impl::Runtime::getInstance().resolve(model_name);
    if (instance_identifier_container.empty())
    {
        return MakeUnexpected(impl::ComErrc::kInstanceIDCouldNotBeResolved,
                              "Binding returned empty vector of instance identifiers");
    }
    return instance_identifier_container;
}

// NOLINTNEXTLINE(modernize-avoid-c-arrays):C-style array tolerated for command line arguments
void bmw::mw::com::runtime::InitializeRuntime(const std::int32_t argc, bmw::StringLiteral argv[])
{
    const amp::span<const bmw::StringLiteral> args(argv, argc);
    bmw::mw::com::impl::Runtime::Initialize(args);
}
