// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#ifndef PLATFORM_AAS_MW_COM_IMPL_I_RUNTIME_BINDING_H
#define PLATFORM_AAS_MW_COM_IMPL_I_RUNTIME_BINDING_H

#include "platform/aas/mw/com/impl/binding_type.h"
#include "platform/aas/mw/com/impl/i_service_discovery_client.h"
#include "platform/aas/mw/com/impl/tracing/i_tracing_runtime_binding.h"

#include <cstdint>

namespace bmw
{
namespace mw
{
namespace com
{
namespace impl
{

/// \brief Interface implemented by runtimes of all different bindings.
/// \details this interface is very thin/coarse as bindings are quite specific and therefore do not have much
///          in common. This also means, that instances of IRuntimeBinding returned by a factory need to be down-casted
///          to concrete implementation/derived classes of IRuntimeBinding, which is easy as the type/tag is provided by
///          IRuntimeBinding::GetBindingType().
class IRuntimeBinding
{
  public:
    IRuntimeBinding() = default;
    IRuntimeBinding(const IRuntimeBinding&) = default;
    IRuntimeBinding(IRuntimeBinding&&) noexcept = default;

    IRuntimeBinding& operator=(IRuntimeBinding&&) & noexcept = default;
    IRuntimeBinding& operator=(const IRuntimeBinding&) & = default;

    virtual ~IRuntimeBinding();
    virtual BindingType GetBindingType() const noexcept = 0;

    /// \brief returns client for binding-specific service discovery
    virtual IServiceDiscoveryClient& GetServiceDiscoveryClient() noexcept = 0;

    /// \brief Returns (optional) TracingRuntime of this binding.
    /// \return pointer to binding specific TracingRuntime or nullptr in case there is no TracingRuntime as
    ///         RuntimeBinding has been created without tracing support.
    virtual tracing::ITracingRuntimeBinding* GetTracingRuntime() noexcept = 0;
};

}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_I_RUNTIME_BINDING_H
