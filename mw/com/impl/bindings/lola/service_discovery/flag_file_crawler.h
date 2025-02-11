// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#ifndef PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_SERVICE_DISCOVERY_FLAG_FILE_CRAWLER_H
#define PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_SERVICE_DISCOVERY_FLAG_FILE_CRAWLER_H

#include "platform/aas/mw/com/impl/bindings/lola/service_discovery/known_instances_container.h"
#include "platform/aas/mw/com/impl/bindings/lola/service_discovery/quality_aware_container.h"

#include "platform/aas/lib/filesystem/filesystem.h"
#include "platform/aas/lib/os/utils/inotify/inotify_instance.h"
#include "platform/aas/lib/os/utils/inotify/inotify_watch_descriptor.h"

namespace bmw::mw::com::impl::lola
{

class FlagFileCrawler
{
  public:
    explicit FlagFileCrawler(os::InotifyInstance& inotify_instance,
                             filesystem::Filesystem filesystem = filesystem::FilesystemFactory{}.CreateInstance());

    auto Crawl(const EnrichedInstanceIdentifier& enriched_instance_identifier) noexcept
        -> bmw::Result<QualityAwareContainer<KnownInstancesContainer>>;

    auto CrawlAndWatch(const EnrichedInstanceIdentifier& enriched_instance_identifier,
                       const bool add_watch = true) noexcept
        -> bmw::Result<std::tuple<std::unordered_map<os::InotifyWatchDescriptor, EnrichedInstanceIdentifier>,
                                  QualityAwareContainer<KnownInstancesContainer>>>;

    static auto ConvertFromStringToInstanceId(amp::string_view view) noexcept -> Result<LolaServiceInstanceId>;
    static auto ParseQualityTypeFromString(const amp::string_view filename) noexcept -> QualityType;

  private:
    os::InotifyInstance& inotify_instance_;
    filesystem::Filesystem filesystem_;

    static auto GatherExistingInstanceDirectories(
        const EnrichedInstanceIdentifier& enriched_instance_identifier) noexcept
        -> bmw::Result<std::vector<EnrichedInstanceIdentifier>>;

    auto AddWatchToInotifyInstance(const EnrichedInstanceIdentifier& enriched_instance_identifier) noexcept
        -> Result<os::InotifyWatchDescriptor>;
};

}  // namespace bmw::mw::com::impl::lola

#endif  // PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_SERVICE_DISCOVERY_FLAG_FILE_CRAWLER_H
