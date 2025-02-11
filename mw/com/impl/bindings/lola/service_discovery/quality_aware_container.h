// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#ifndef PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_SERVICE_DISCOVERY_QUALITY_AWARE_CONTAINER_H
#define PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_SERVICE_DISCOVERY_QUALITY_AWARE_CONTAINER_H

namespace bmw::mw::com::impl::lola
{

template <class T>
struct QualityAwareContainer
{
    T asil_b{};
    // 
    T asil_qm{};
};

}  // namespace bmw::mw::com::impl::lola

#endif  // PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_SERVICE_DISCOVERY_QUALITY_AWARE_CONTAINER_H
