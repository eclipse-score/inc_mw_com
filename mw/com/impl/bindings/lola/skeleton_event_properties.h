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


#ifndef PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_SKELETON_EVENT_PROPERTIES_H
#define PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_SKELETON_EVENT_PROPERTIES_H

#include <cstddef>

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

struct SkeletonEventProperties
{
    std::size_t number_of_slots;
    std::size_t max_subscribers;
    bool enforce_max_samples;
};

}  // namespace lola
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif
