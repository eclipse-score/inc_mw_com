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


#include "platform/aas/mw/com/impl/bindings/lola/service_discovery/flag_file_crawler.h"
#include "platform/aas/lib/filesystem/filesystem.h"
#include "platform/aas/mw/com/impl/bindings/lola/service_discovery/flag_file.h"

#include <charconv>

namespace bmw::mw::com::impl::lola
{

namespace
{
bool MapInstancesToQualityTypes(QualityAwareContainer<KnownInstancesContainer>& known_instances,
                                std::vector<EnrichedInstanceIdentifier>& quality_unaware_identifiers_to_check) noexcept
{
    for (const auto& quality_unaware_identifier_to_check : quality_unaware_identifiers_to_check)
    {
        constexpr std::array<QualityType, 2> supported_quality_types{QualityType::kASIL_B, QualityType::kASIL_QM};
        for (const auto quality_type : supported_quality_types)
        {
            EnrichedInstanceIdentifier quality_aware_identifier_to_check{quality_unaware_identifier_to_check,
                                                                         quality_type};
            if (FlagFile::Exists(quality_aware_identifier_to_check))
            {
                switch (quality_type)
                {
                    case QualityType::kASIL_B:
                    {
                        mw::log::LogDebug("lola")
                            << "LoLa SD: Added" << GetSearchPathForIdentifier(quality_aware_identifier_to_check)
                            << "(ASIL-B)";
                        amp::ignore = known_instances.asil_b.Insert(quality_aware_identifier_to_check);
                        break;
                    }
                    case QualityType::kASIL_QM:
                    {
                        mw::log::LogDebug("lola")
                            << "LoLa SD: Added" << GetSearchPathForIdentifier(quality_aware_identifier_to_check)
                            << "(ASIL-QM)";
                        amp::ignore = known_instances.asil_qm.Insert(quality_aware_identifier_to_check);
                        break;
                    }
                    case QualityType::kInvalid:
                    default:
                    {
                        return false;
                    }
                }
            }
        }
    }
    return true;
}
}  // namespace

FlagFileCrawler::FlagFileCrawler(os::InotifyInstance& inotify_instance, filesystem::Filesystem filesystem)
    : inotify_instance_{inotify_instance}, filesystem_{std::move(filesystem)}
{
}

auto FlagFileCrawler::Crawl(const EnrichedInstanceIdentifier& enriched_instance_identifier) noexcept
    -> bmw::Result<QualityAwareContainer<KnownInstancesContainer>>
{
    const auto result = CrawlAndWatch(enriched_instance_identifier, false);
    if (result.has_value())
    {
        return std::get<1>(result.value());
    }
    else
    {
        return Unexpected{result.error()};
    }
}

auto FlagFileCrawler::CrawlAndWatch(const EnrichedInstanceIdentifier& enriched_instance_identifier,
                                    const bool add_watch) noexcept
    -> bmw::Result<std::tuple<std::unordered_map<os::InotifyWatchDescriptor, EnrichedInstanceIdentifier>,
                              QualityAwareContainer<KnownInstancesContainer>>>
{
    EnrichedInstanceIdentifier quality_unaware_enriched_instance_identifier{enriched_instance_identifier,
                                                                            QualityType::kInvalid};

    std::unordered_map<os::InotifyWatchDescriptor, EnrichedInstanceIdentifier> watch_descriptors{};

    if (add_watch)
    {
        const auto watch_descriptor = AddWatchToInotifyInstance(quality_unaware_enriched_instance_identifier);
        if (!(watch_descriptor.has_value()))
        {
            bmw::mw::log::LogError("lola") << "Could not add watch to instance identifier";
            return MakeUnexpected(ComErrc::kBindingFailure, "Could not add watch to main search directory");
        }
        amp::ignore = watch_descriptors.emplace(watch_descriptor.value(), quality_unaware_enriched_instance_identifier);
    }

    std::vector<EnrichedInstanceIdentifier> quality_unaware_identifiers_to_check{};
    if (enriched_instance_identifier.GetBindingSpecificInstanceId<LolaServiceInstanceId>().has_value())
    {
        quality_unaware_identifiers_to_check.push_back(quality_unaware_enriched_instance_identifier);
    }
    else
    {
        const auto found_instance_directories =
            GatherExistingInstanceDirectories(quality_unaware_enriched_instance_identifier);

        if (!found_instance_directories.has_value())
        {
            return MakeUnexpected(ComErrc::kBindingFailure, "Could not crawl filesystem");
        }

        for (const auto& found_quality_unaware_identifier : *found_instance_directories)
        {
            quality_unaware_identifiers_to_check.push_back(found_quality_unaware_identifier);
            if (add_watch)
            {
                const auto instance_watch_descriptor = AddWatchToInotifyInstance(found_quality_unaware_identifier);
                if (!(instance_watch_descriptor.has_value()))
                {
                    mw::log::LogError("lola") << "Could not add watch for instance"
                                              << GetSearchPathForIdentifier(found_quality_unaware_identifier);
                    return MakeUnexpected(ComErrc::kBindingFailure, "Could not add watch to search subdirectory");
                }
                amp::ignore =
                    watch_descriptors.emplace(instance_watch_descriptor.value(), found_quality_unaware_identifier);
            }
        }
    }

    QualityAwareContainer<KnownInstancesContainer> known_instances{};
    if (!MapInstancesToQualityTypes(known_instances, quality_unaware_identifiers_to_check))
    {
        bmw::mw::log::LogFatal("lola") << "Quality level not set for instance identifier. Terminating.";
        return MakeUnexpected(ComErrc::kBindingFailure, "Could not determine correct quality type");
    }

    return {{watch_descriptors, known_instances}};
}

auto FlagFileCrawler::ConvertFromStringToInstanceId(amp::string_view view) noexcept -> Result<LolaServiceInstanceId>
{
    LolaServiceInstanceId::InstanceId actual_instance_id{};
    const auto conversion_result = std::from_chars(view.cbegin(), view.cend(), actual_instance_id);
    if (conversion_result.ec != std::errc{})
    {
        return MakeUnexpected(ComErrc::kBindingFailure, "Could not parse instance id from string");
    }
    return LolaServiceInstanceId{actual_instance_id};
}

auto FlagFileCrawler::ParseQualityTypeFromString(const amp::string_view filename) noexcept -> QualityType
{
    if (filename.find(GetQualityTypeString(QualityType::kASIL_B)) != std::string::npos)
    {
        return QualityType::kASIL_B;
    }
    if (filename.find(GetQualityTypeString(QualityType::kASIL_QM)) != std::string::npos)
    {
        return QualityType::kASIL_QM;
    }
    return QualityType::kInvalid;
}

auto FlagFileCrawler::GatherExistingInstanceDirectories(
    const EnrichedInstanceIdentifier& enriched_instance_identifier) noexcept
    -> bmw::Result<std::vector<EnrichedInstanceIdentifier>>
{
    AMP_PRECONDITION_PRD_MESSAGE(
        !enriched_instance_identifier.GetBindingSpecificInstanceId<LolaServiceInstanceId>().has_value(),
        "Handle must not have instance id");
    filesystem::DirectoryIterator directory_iterator{GetSearchPathForIdentifier(enriched_instance_identifier)};

    std::vector<EnrichedInstanceIdentifier> enriched_instance_identifiers{};
    for (const auto& entry : directory_iterator)
    {
        const auto status = entry.Status();
        if (!(status.has_value()))
        {
            return MakeUnexpected(ComErrc::kBindingFailure, "Could not add watch to search subdirectory");
        }
        if (status->Type() != filesystem::FileType::kDirectory)
        {
            mw::log::LogError("lola") << "Found file" << entry.GetPath() << "- should be directory";
            continue;
        }

        const auto filename = entry.GetPath().Filename().Native();
        const auto instance_id = ConvertFromStringToInstanceId(entry.GetPath().Filename().Native());
        if (!(instance_id.has_value()))
        {
            mw::log::LogError("lola") << "Could not parse" << entry.GetPath() << "to instance id";
            continue;
        }
        EnrichedInstanceIdentifier found_enriched_instance_identifier{
            enriched_instance_identifier.GetInstanceIdentifier(),
            ServiceInstanceId{LolaServiceInstanceId{instance_id.value().id_}}};
        amp::ignore = enriched_instance_identifiers.emplace_back(std::move(found_enriched_instance_identifier),
                                                                 ParseQualityTypeFromString(filename));
    }
    return enriched_instance_identifiers;
}

auto FlagFileCrawler::AddWatchToInotifyInstance(const EnrichedInstanceIdentifier& enriched_instance_identifier) noexcept
    -> Result<os::InotifyWatchDescriptor>
{

    auto directory_creation_result = FlagFile::CreateSearchPath(enriched_instance_identifier, filesystem_);
    if (!(directory_creation_result.has_value()))
    {
        mw::log::LogError("lola") << "Could not create search path with" << directory_creation_result.error();
        return MakeUnexpected(ComErrc::kBindingFailure, "Could not create search path");
    }

    const auto search_path = std::move(directory_creation_result).value();
    const auto watch_descriptor = inotify_instance_.AddWatch(
        search_path.Native(), os::Inotify::EventMask::kInCreate | os::Inotify::EventMask::kInDelete);
    if (!(watch_descriptor.has_value()))
    {
        mw::log::LogError("lola") << "Could not add watch for" << search_path << ":"
                                  << watch_descriptor.error().ToString();
        return bmw::MakeUnexpected(ComErrc::kBindingFailure, "Could not add watch for service id");
    }

    return watch_descriptor.value();
}

}  // namespace bmw::mw::com::impl::lola
