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


#ifndef PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_ROLLBACK_DATA_H
#define PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_ROLLBACK_DATA_H

#include <mutex>
#include <unordered_set>

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

struct ServiceDataControl;

struct RollbackData
{
    std::unordered_set<ServiceDataControl*> synchronisation_data_set{};
    std::mutex set_mutex{};
};

}  // namespace lola
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_ROLLBACK_DATA_H
