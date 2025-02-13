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


#ifndef PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_SERVICE_DISCOVERY_FLAG_FILE_H
#define PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_SERVICE_DISCOVERY_FLAG_FILE_H

#include "platform/aas/mw/com/impl/enriched_instance_identifier.h"

#include "platform/aas/lib/filesystem/filesystem.h"
#include "platform/aas/lib/result/result.h"

#include <chrono>

namespace bmw::mw::com::impl::lola
{

auto GetQualityTypeString(QualityType quality_type) noexcept -> amp::string_view;
auto GetSearchPathForIdentifier(const EnrichedInstanceIdentifier& enriched_instance_identifier) noexcept
    -> filesystem::Path;

class FlagFile
{
  public:
    using Disambiguator = std::chrono::steady_clock::time_point::rep;

    ~FlagFile();

    static auto Make(EnrichedInstanceIdentifier enriched_instance_identifier,
                     Disambiguator offer_disambiguator,
                     filesystem::Filesystem filesystem) noexcept -> bmw::Result<FlagFile>;

    FlagFile(const FlagFile&) = delete;
    FlagFile(FlagFile&&) noexcept;
    FlagFile& operator=(const FlagFile&) = delete;
    FlagFile& operator=(FlagFile&&) = delete;

    static auto Exists(const EnrichedInstanceIdentifier& enriched_instance_identifier) noexcept -> bool;

    static auto CreateSearchPath(
        const EnrichedInstanceIdentifier& enriched_instance_identifier,
        const filesystem::Filesystem& filesystem = filesystem::FilesystemFactory{}.CreateInstance()) noexcept
        -> bmw::Result<filesystem::Path>;

  private:
    EnrichedInstanceIdentifier enriched_instance_identifier_;
    Disambiguator offer_disambiguator_;
    bool is_offered_;

    filesystem::Filesystem filesystem_;

    FlagFile(EnrichedInstanceIdentifier enriched_instance_identifier,
             const Disambiguator offer_disambiguator,
             filesystem::Filesystem filesystem);
};

}  // namespace bmw::mw::com::impl::lola

#endif  // PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_SERVICE_DISCOVERY_FLAG_FILE_H
