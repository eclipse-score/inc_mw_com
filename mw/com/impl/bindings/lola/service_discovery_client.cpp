// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#include "platform/aas/mw/com/impl/bindings/lola/service_discovery_client.h"

#include "platform/aas/mw/com/impl/bindings/lola/service_discovery/flag_file_crawler.h"

#include "platform/aas/lib/filesystem/filesystem.h"
#include "platform/aas/mw/log/logging.h"

#include <amp_expected.hpp>
#include <amp_variant.hpp>

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
using underlying_type_readmask = std::underlying_type<os::InotifyEvent::ReadMask>::type;

auto ReadMaskSet(const os::InotifyEvent& event, const os::InotifyEvent::ReadMask mask) noexcept -> bool
{
    return static_cast<underlying_type_readmask>(event.GetMask() & mask) != 0;
}

std::vector<HandleType> GetKnownHandles(EnrichedInstanceIdentifier enriched_instance_identifier,
                                        QualityAwareContainer<KnownInstancesContainer> known_instances) noexcept
{
    std::vector<HandleType> known_handles{};
    switch (enriched_instance_identifier.GetQualityType())
    {
        case QualityType::kASIL_B:
            known_handles = known_instances.asil_b.GetKnownHandles(enriched_instance_identifier);
            break;
        case QualityType::kASIL_QM:
            known_handles = known_instances.asil_qm.GetKnownHandles(enriched_instance_identifier);
            break;
        case QualityType::kInvalid:
            [[fallthrough]];
        default:
            bmw::mw::log::LogFatal("lola") << "Quality level not set for instance identifier. Terminating.";
            std::terminate();
    }

    return known_handles;
}

}  // namespace

ServiceDiscoveryClient::ServiceDiscoveryClient(concurrency::Executor& long_running_threads) noexcept
    : ServiceDiscoveryClient(long_running_threads,
                             std::make_unique<os::InotifyInstanceImpl>(),
                             std::make_unique<os::internal::UnistdImpl>(),
                             filesystem::FilesystemFactory{}.CreateInstance())
{
}

ServiceDiscoveryClient::ServiceDiscoveryClient(concurrency::Executor& long_running_threads,
                                               std::unique_ptr<os::InotifyInstance> inotify_instance,
                                               std::unique_ptr<os::Unistd> unistd,
                                               filesystem::Filesystem filesystem) noexcept
    : IServiceDiscoveryClient{},
      offer_disambiguator_{std::chrono::steady_clock::now().time_since_epoch().count()},
      i_notify_{std::move(inotify_instance)},
      unistd_{std::move(unistd)},
      filesystem_{std::move(filesystem)},
      long_running_threads_{long_running_threads}
{
    worker_thread_result_ = long_running_threads_.Submit([this](const auto stop_token) noexcept {
        amp::stop_callback foo{stop_token, [this]() noexcept { i_notify_->Close(); }};
        while (!stop_token.stop_requested())
        {
            const auto expected_events = i_notify_->Read();
            HandleEvents(expected_events);
        }
    });
}

ServiceDiscoveryClient::~ServiceDiscoveryClient()
{
    // Shut down worker thread correctly to avoid concurrency issues during destruction
    worker_thread_result_.Abort();
    amp::ignore = worker_thread_result_.Wait();
}

auto ServiceDiscoveryClient::OfferService(InstanceIdentifier instance_identifier) noexcept -> ResultBlank
{
    const EnrichedInstanceIdentifier enriched_instance_identifier{instance_identifier};
    AMP_PRECONDITION_PRD_MESSAGE(
        enriched_instance_identifier.GetBindingSpecificInstanceId<LolaServiceInstanceId>().has_value(),
        "Instance identifier must have instance id for service offer");
    const auto offer_disambiguator = offer_disambiguator_ += 1U;

    {
        std::lock_guard lock{flag_files_mutex_};
        if (flag_files_.find(instance_identifier) != flag_files_.cend())
        {
            return MakeUnexpected(ComErrc::kBindingFailure, "Service is already offered");
        }
    }

    QualityAwareContainer<amp::optional<FlagFile>> flag_files{};
    switch (enriched_instance_identifier.GetQualityType())
    {
        case QualityType::kASIL_B:
        {
            EnrichedInstanceIdentifier asil_b_enriched_instance_identifier{enriched_instance_identifier,
                                                                           QualityType::kASIL_B};
            auto asil_b_flag_file =
                FlagFile::Make(asil_b_enriched_instance_identifier, offer_disambiguator, filesystem_);
            if (!asil_b_flag_file.has_value())
            {
                return bmw::MakeUnexpected(ComErrc::kServiceNotOffered, "Failed to create flag file for ASIL-B");
            }
            flag_files.asil_b = std::move(asil_b_flag_file).value();
        }
            [[fallthrough]];
        case QualityType::kASIL_QM:
        {
            EnrichedInstanceIdentifier asil_qm_enriched_instance_identifier{enriched_instance_identifier,
                                                                            QualityType::kASIL_QM};
            auto asil_qm_flag_file =
                FlagFile::Make(asil_qm_enriched_instance_identifier, offer_disambiguator, filesystem_);
            if (!asil_qm_flag_file.has_value())
            {
                return bmw::MakeUnexpected(ComErrc::kServiceNotOffered, "Failed to create flag file for ASIL-QM");
            }
            flag_files.asil_qm = std::move(asil_qm_flag_file).value();
            break;
        }
        case QualityType::kInvalid:
            [[fallthrough]];
        default:
            return bmw::MakeUnexpected(ComErrc::kBindingFailure, "Unknown quality type of service");
    }

    {
        std::lock_guard lock{flag_files_mutex_};
        flag_files_.emplace(enriched_instance_identifier.GetInstanceIdentifier(), std::move(flag_files));
    }

    return {};
}

auto ServiceDiscoveryClient::StopOfferService(InstanceIdentifier instance_identifier,
                                              IServiceDiscovery::QualityTypeSelector quality_type_selector) noexcept
    -> ResultBlank
{
    const EnrichedInstanceIdentifier enriched_instance_identifier{instance_identifier};
    AMP_PRECONDITION_PRD_MESSAGE(
        enriched_instance_identifier.GetBindingSpecificInstanceId<LolaServiceInstanceId>().has_value(),
        "Instance identifier must have instance id for service offer stop");

    {
        std::lock_guard lock{flag_files_mutex_};
        const auto flag_file_iterator = flag_files_.find(enriched_instance_identifier.GetInstanceIdentifier());
        if (flag_file_iterator == flag_files_.cend())
        {
            return bmw::MakeUnexpected(ComErrc::kBindingFailure, "Never offered or offer already stopped");
        }

        switch (quality_type_selector)
        {
            case IServiceDiscovery::QualityTypeSelector::kBoth:
                flag_files_.erase(flag_file_iterator);
                break;
            case IServiceDiscovery::QualityTypeSelector::kAsilQm:
                flag_file_iterator->second.asil_qm.reset();
                break;
            default:
                return bmw::MakeUnexpected(ComErrc::kBindingFailure, "Unknown quality type of service");
        }
    }

    return {};
}

auto ServiceDiscoveryClient::StopFindService(FindServiceHandle find_service_handle) noexcept -> ResultBlank
{
    {
        std::lock_guard lock{worker_mutex_};
        obsolete_search_requests_.emplace(find_service_handle);
    }

    mw::log::LogDebug("lola") << "LoLa SD: Stopped service discovery for FindServiceHandle"
                              << FindServiceHandleView{find_service_handle}.getUid();

    return {};
}

auto ServiceDiscoveryClient::TransferSearchRequests() noexcept -> void
{
    TransferObsoleteSearchRequests();
}

auto ServiceDiscoveryClient::TransferNewSearchRequest(NewSearchRequest search_request) noexcept
    -> SearchRequestsContainer::value_type&
{
    auto& [find_service_handle,
           instance_identifier,
           watch_descriptors,
           on_service_found_callback,
           known_instances,
           previous_handles] = search_request;

    std::unordered_set<os::InotifyWatchDescriptor> watch_descriptor_placeholder{};
    watch_descriptor_placeholder.reserve(watch_descriptors.size());

    const auto added_search_request = search_requests_.emplace(find_service_handle,
                                                               std::make_tuple(std::move(watch_descriptor_placeholder),
                                                                               std::move(on_service_found_callback),
                                                                               instance_identifier,
                                                                               previous_handles));
    AMP_PRECONDITION_PRD_MESSAGE(added_search_request.second,
                                 "The FindServiceHandle should be unique for every call to StartFindService");

    for (const auto& watch_descriptor : watch_descriptors)
    {
        const auto watch_iterator = StoreWatch(watch_descriptor.first, watch_descriptor.second);
        LinkWatchWithSearchRequest(watch_iterator, added_search_request.first);
    }

    known_instances_.asil_b.Merge(std::move(known_instances.asil_b));
    known_instances_.asil_qm.Merge(std::move(known_instances.asil_qm));

    return *(added_search_request.first);
}

auto ServiceDiscoveryClient::TransferObsoleteSearchRequests() noexcept -> void
{
    for (const auto& obsolete_search_request : obsolete_search_requests_)
    {
        TransferObsoleteSearchRequest(obsolete_search_request);
    }
    obsolete_search_requests_.clear();
}

auto ServiceDiscoveryClient::TransferObsoleteSearchRequest(const FindServiceHandle& find_service_handle) noexcept
    -> void
{
    const auto search_iterator = search_requests_.find(find_service_handle);
    if (search_iterator == search_requests_.end())
    {
        mw::log::LogWarn("lola") << "Could not find search request for:"
                                 << FindServiceHandleView{find_service_handle}.getUid();
        return;
    }

    // Intentional copy since it allows us to iterate over the watches while we modify the original set in
    // UnlinkWatchWithSearchRequest(). This could be optimised, but would make the algorithm even more complex.
    const auto watches = std::get<std::unordered_set<os::InotifyWatchDescriptor>>(search_iterator->second);
    for (const auto& watch : watches)
    {
        const auto watch_iterator = watches_.find(watch);
        if (watch_iterator == watches_.end())
        {
            mw::log::LogError("lola") << "Could not find watch for:"
                                      << FindServiceHandleView{find_service_handle}.getUid();
            continue;
        }

        UnlinkWatchWithSearchRequest(watch_iterator, search_iterator);
        auto& [enriched_instance_identifier, find_service_handles] = watch_iterator->second;
        if (find_service_handles.empty())
        {
            known_instances_.asil_b.Remove(enriched_instance_identifier);
            known_instances_.asil_qm.Remove(enriched_instance_identifier);
            amp::ignore = i_notify_->RemoveWatch(watch_iterator->first);
            EraseWatch(watch_iterator);
        }
    }

    search_requests_.erase(find_service_handle);
}

auto ServiceDiscoveryClient::HandleEvents(
    const amp::expected<amp::static_vector<os::InotifyEvent, os::InotifyInstance::max_events>, os::Error>&
        expected_events) noexcept -> void
{
    std::lock_guard lock{worker_mutex_};

    TransferSearchRequests();

    if (!(expected_events.has_value()))
    {
        if (expected_events.error() != os::Error::Code::kOperationWasInterruptedBySignal)
        {
            mw::log::LogError("lola") << "Inotify Read() failed with:" << expected_events.error().ToString();
        }
        return;
    }

    const auto& events = expected_events.value();

    std::vector<os::InotifyEvent> deletion_events{};
    std::vector<os::InotifyEvent> creation_events{};
    for (const auto& event : events)
    {

        const bool inotify_queue_overflowed = ReadMaskSet(event, os::InotifyEvent::ReadMask::kInQOverflow);
        const bool search_directory_was_removed = ReadMaskSet(event, os::InotifyEvent::ReadMask::kInIgnored);
        const bool flag_file_was_removed = ReadMaskSet(event, os::InotifyEvent::ReadMask::kInDelete);
        const bool inode_was_removed = search_directory_was_removed || flag_file_was_removed;
        const bool inode_was_created = ReadMaskSet(event, os::InotifyEvent::ReadMask::kInCreate);

        if (inotify_queue_overflowed)
        {
            mw::log::LogError("lola")
                << "Service discovery lost at least one event and is compromised now. Bailing out!";
            // Potential optimization: Resync the full service discovery with the file system and update all ongoing
            // searches with potential changes.
            std::terminate();
        }

        if (inode_was_removed)
        {
            deletion_events.push_back(event);
        }
        else if (inode_was_created)
        {
            creation_events.push_back(event);
        }
        else
        {
            const auto watch_iterator = watches_.find(event.GetWatchDescriptor());
            if (watch_iterator == watches_.end())
            {
                mw::log::LogWarn("lola") << "Received unexpected event on unknown watch"
                                         << event.GetWatchDescriptor().GetUnderlying() << "with mask"
                                         << static_cast<underlying_type_readmask>(event.GetMask());
            }
            else
            {
                const auto enriched_instance_identifier = std::get<EnrichedInstanceIdentifier>(watch_iterator->second);
                const auto file_path =
                    GetSearchPathForIdentifier(enriched_instance_identifier) / event.GetName().to_string().c_str();
                mw::log::LogWarn("lola") << "Received unexpected event on" << file_path << "with mask"
                                         << static_cast<underlying_type_readmask>(event.GetMask());
            }
        }
    }

    HandleDeletionEvents(deletion_events);

    HandleCreationEvents(creation_events);
}

auto ServiceDiscoveryClient::HandleDeletionEvents(const std::vector<os::InotifyEvent>& events) noexcept -> void
{
    std::unordered_set<FindServiceHandle> impacted_searches{};
    for (const auto& event : events)
    {
        const auto watch_descriptor = event.GetWatchDescriptor();
        const auto watch_iterator = watches_.find(watch_descriptor);
        if (watch_iterator == watches_.end())
        {
            continue;
        }

        const bool file_was_deleted = ReadMaskSet(event, os::InotifyEvent::ReadMask::kInDelete);

        const auto& [enriched_instance_identifier, search_keys] = watch_iterator->second;
        if (file_was_deleted)
        {
            if (enriched_instance_identifier.GetBindingSpecificInstanceId<LolaServiceInstanceId>().has_value())
            {
                OnInstanceFlagFileRemoved(watch_iterator, event.GetName());
                impacted_searches.insert(search_keys.cbegin(), search_keys.cend());
            }
            else
            {
                mw::log::LogFatal("lola")
                    << "Directory" << GetSearchPathForIdentifier(enriched_instance_identifier) << "/" << event.GetName()
                    << "was deleted. Outside tampering with service discovery. Aborting!";
                std::terminate();
            }
        }
    }

    CallHandlers(impacted_searches);
}

auto ServiceDiscoveryClient::HandleCreationEvents(const std::vector<os::InotifyEvent>& events) noexcept -> void
{
    std::unordered_set<FindServiceHandle> impacted_searches{};
    for (const auto& event : events)
    {
        const auto watch_descriptor = event.GetWatchDescriptor();
        const auto watch_iterator = watches_.find(watch_descriptor);
        if (watch_iterator == watches_.end())
        {
            continue;
        }

        const auto& [enriched_instance_identifier, search_keys] = watch_iterator->second;
        if (enriched_instance_identifier.GetBindingSpecificInstanceId<LolaServiceInstanceId>().has_value())
        {
            OnInstanceFlagFileCreated(watch_iterator, event.GetName());
        }
        else
        {
            OnInstanceDirectoryCreated(watch_iterator, event.GetName());
        }

        impacted_searches.insert(search_keys.cbegin(), search_keys.cend());
    }

    CallHandlers(impacted_searches);
}

auto ServiceDiscoveryClient::CallHandlers(const std::unordered_set<FindServiceHandle>& search_keys) noexcept -> void
{
    for (const auto& search_key : search_keys)
    {
        const auto search_iterator = search_requests_.find(search_key);
        if (search_iterator == search_requests_.cend())
        {
            continue;
        }

        const auto obsolete_search_iterator = obsolete_search_requests_.find(search_key);
        if (obsolete_search_iterator != obsolete_search_requests_.cend())
        {
            continue;
        }

        const auto& enriched_instance_identifier = std::get<EnrichedInstanceIdentifier>(search_iterator->second);
        std::vector<HandleType> known_handles{};
        switch (enriched_instance_identifier.GetQualityType())
        {
            case QualityType::kASIL_B:
                known_handles = known_instances_.asil_b.GetKnownHandles(enriched_instance_identifier);
                break;
            case QualityType::kASIL_QM:
                known_handles = known_instances_.asil_qm.GetKnownHandles(enriched_instance_identifier);
                break;
            case QualityType::kInvalid:
                [[fallthrough]];
            default:
                bmw::mw::log::LogFatal("lola") << "Quality level not set for instance identifier. Terminating.";
                std::terminate();
        }

        std::unordered_set<HandleType> new_handles{known_handles.cbegin(), known_handles.cend()};
        auto& previous_handles = std::get<std::unordered_set<HandleType>>(search_iterator->second);

        if (previous_handles == new_handles)
        {
            continue;
        }

        mw::log::LogDebug("lola") << "LoLa SD: Starting asynchronous call to handler for FindServiceHandle"
                                  << FindServiceHandleView{search_key}.getUid() << "with" << known_handles.size()
                                  << "handles";

        previous_handles = new_handles;

        const auto& handler = std::get<FindServiceHandler<HandleType>>(search_iterator->second);
        handler(std::move(known_handles), search_key);

        mw::log::LogDebug("lola") << "LoLa SD: Asynchronous call to handler for FindServiceHandle"
                                  << FindServiceHandleView{search_key}.getUid() << "finished";
    }
}

auto ServiceDiscoveryClient::StoreWatch(const os::InotifyWatchDescriptor& watch_descriptor,
                                        const EnrichedInstanceIdentifier& enriched_instance_identifier) noexcept
    -> WatchesContainer::iterator
{
    std::unordered_set<FindServiceHandle> find_service_handles{};
    const auto watch_result =
        watches_.emplace(watch_descriptor, std::make_tuple(enriched_instance_identifier, find_service_handles));
    const auto identifier = LolaServiceInstanceIdentifier{enriched_instance_identifier};

    amp::ignore = watched_identifiers_.emplace(
        identifier, IdentifierWatches{watch_descriptor, std::unordered_set<os::InotifyWatchDescriptor>{}});
    if (identifier.GetInstanceId().has_value())
    {
        auto any_identifier = LolaServiceInstanceIdentifier{identifier.GetServiceId()};
        auto emplace_result = watched_identifiers_.emplace(
            any_identifier,
            IdentifierWatches{std::nullopt, std::unordered_set<os::InotifyWatchDescriptor>{watch_descriptor}});
        if (!emplace_result.second)
        {
            auto& child_watches = emplace_result.first->second.child_watches;
            child_watches.emplace(watch_descriptor);
        }
    }

    return watch_result.first;
}

auto ServiceDiscoveryClient::EraseWatch(const WatchesContainer::iterator& watch_iterator) noexcept -> void
{
    AMP_PRECONDITION_PRD_MESSAGE(std::get<std::unordered_set<FindServiceHandle>>(watch_iterator->second).empty(),
                                 "Watch must not be associated to any searches");
    const auto enriched_instance_identifier = std::get<EnrichedInstanceIdentifier>(watch_iterator->second);
    const auto identifier = LolaServiceInstanceIdentifier(enriched_instance_identifier);
    if (identifier.GetInstanceId().has_value())
    {
        watched_identifiers_.erase(identifier);
        auto any_identifier = LolaServiceInstanceIdentifier{identifier.GetServiceId()};
        auto watched_any_identifier = watched_identifiers_.find(any_identifier);
        if (watched_any_identifier != watched_identifiers_.end())
        {
            auto& child_watches = watched_any_identifier->second.child_watches;
            child_watches.erase(watch_iterator->first);
        }
    }
    else
    {
        auto watched_identifier = watched_identifiers_.find(identifier);
        if (watched_identifier != watched_identifiers_.end())
        {
            auto& main_watch = watched_identifier->second.watch_descriptor;
            main_watch = std::nullopt;
        }
    }
    watches_.erase(watch_iterator);
}

auto ServiceDiscoveryClient::LinkWatchWithSearchRequest(
    const WatchesContainer::iterator& watch_iterator,
    const SearchRequestsContainer::iterator& search_iterator) noexcept -> void
{
    AMP_PRECONDITION_PRD_MESSAGE(watch_iterator != watches_.end() && search_iterator != search_requests_.end(),
                                 "LinkWatchWithSearchRequest requires valid iterators");

    auto& search_key_set = std::get<std::unordered_set<FindServiceHandle>>(watch_iterator->second);
    const auto search_key_insertion = search_key_set.insert(search_iterator->first);
    AMP_ASSERT_PRD_MESSAGE(search_key_insertion.second, "LinkWatchWithSearchRequest did not insert search key");
    auto& watch_key_set = std::get<std::unordered_set<os::InotifyWatchDescriptor>>(search_iterator->second);
    const auto watch_key_insertion = watch_key_set.insert(watch_iterator->first);
    AMP_ASSERT_PRD_MESSAGE(watch_key_insertion.second, "LinkWatchWithSearchRequest did not insert watch key");
}

auto ServiceDiscoveryClient::UnlinkWatchWithSearchRequest(
    const WatchesContainer::iterator& watch_iterator,
    const SearchRequestsContainer::iterator& search_iterator) noexcept -> void
{
    AMP_PRECONDITION_PRD_MESSAGE(watch_iterator != watches_.end() && search_iterator != search_requests_.end(),
                                 "LinkWatchWithSearchRequest requires valid iterators");

    auto& search_key_set = std::get<std::unordered_set<FindServiceHandle>>(watch_iterator->second);
    const auto number_of_search_keys_erased = search_key_set.erase(search_iterator->first);
    AMP_ASSERT_PRD_MESSAGE(number_of_search_keys_erased == 1U,
                           "UnlinkWatchWithSearchRequest did not erase search key correctly");
    auto& watch_key_set = std::get<std::unordered_set<os::InotifyWatchDescriptor>>(search_iterator->second);
    const auto number_of_watch_keys_erased = watch_key_set.erase(watch_iterator->first);
    AMP_ASSERT_PRD_MESSAGE(number_of_watch_keys_erased == 1U,
                           "UnlinkWatchWithSearchRequest did not erase watch key correctly");
}

ResultBlank ServiceDiscoveryClient::StartFindService(FindServiceHandle find_service_handle,
                                                     FindServiceHandler<HandleType> handler,
                                                     EnrichedInstanceIdentifier enriched_instance_identifier) noexcept
{
    std::lock_guard worker_lock{worker_mutex_};

    mw::log::LogDebug("lola") << "LoLa SD: Starting service discovery for"
                              << GetSearchPathForIdentifier(enriched_instance_identifier) << "with FindServiceHandle"
                              << FindServiceHandleView{find_service_handle}.getUid();

    std::vector<HandleType> known_handles{};
    std::unordered_map<os::InotifyWatchDescriptor, EnrichedInstanceIdentifier> watch_descriptors{};
    QualityAwareContainer<KnownInstancesContainer> known_instances{};
    // Check if the exact same search is already in progress. If it is, we can just duplicate the search request and
    // reuse cache data.
    const auto identifier = LolaServiceInstanceIdentifier(enriched_instance_identifier);
    const auto watched_identifier = watched_identifiers_.find(identifier);
    if (watched_identifier != watched_identifiers_.end() && watched_identifier->second.watch_descriptor.has_value())
    {
        known_handles = GetKnownHandles(enriched_instance_identifier, known_instances_);

        auto add_watch = [this, &watch_descriptors](const os::InotifyWatchDescriptor& watch_descriptor) {
            const auto matching_watch = watches_.find(watch_descriptor);
            AMP_ASSERT_PRD_MESSAGE(matching_watch != watches_.cend(), "Did not find matching watch");
            watch_descriptors.emplace(watch_descriptor, std::get<EnrichedInstanceIdentifier>(matching_watch->second));
        };

        add_watch(watched_identifier->second.watch_descriptor.value());
        std::for_each(watched_identifier->second.child_watches.cbegin(),
                      watched_identifier->second.child_watches.cend(),
                      add_watch);
    }
    else
    {
        auto crawler_result = FlagFileCrawler{*i_notify_}.CrawlAndWatch(enriched_instance_identifier);
        if (!crawler_result.has_value())
        {
            return bmw::MakeUnexpected(ComErrc::kBindingFailure, "Failed to crawl filesystem");
        }

        auto& [found_watch_descriptors, found_known_instances] = crawler_result.value();
        known_handles = GetKnownHandles(enriched_instance_identifier, found_known_instances);
        watch_descriptors = std::move(found_watch_descriptors);
        known_instances = std::move(found_known_instances);
    }

    const auto& stored_search_request =
        TransferNewSearchRequest({find_service_handle,
                                  enriched_instance_identifier,
                                  std::move(watch_descriptors),
                                  std::move(handler),
                                  std::move(known_instances),
                                  std::unordered_set<HandleType>{known_handles.cbegin(), known_handles.cend()}});

    if (!(known_handles.empty()))
    {
        mw::log::LogDebug("lola") << "LoLa SD: Synchronously calling handler for FindServiceHandle"
                                  << FindServiceHandleView{find_service_handle}.getUid();
        const auto& stored_handler = std::get<FindServiceHandler<HandleType>>(stored_search_request.second);
        stored_handler(known_handles, find_service_handle);
        mw::log::LogDebug("lola") << "LoLa SD: Synchronous call to handler for FindServiceHandle"
                                  << FindServiceHandleView{find_service_handle}.getUid() << "finished";
    }

    return {};
}

auto ServiceDiscoveryClient::OnInstanceDirectoryCreated(const WatchesContainer::iterator& watch_iterator,
                                                        amp::string_view name) noexcept -> void
{
    const auto& [enriched_instance_identifier, search_keys] = watch_iterator->second;

    const auto expected_instance_id = FlagFileCrawler::ConvertFromStringToInstanceId(name);
    if (!(expected_instance_id.has_value()))
    {
        mw::log::LogError("lola") << "Outside tampering. Could not determine instance id from" << name << ". Skipping!";
        return;
    }
    const auto& instance_id = expected_instance_id.value();

    const EnrichedInstanceIdentifier enriched_instance_identifier_with_instance_id_from_string{
        enriched_instance_identifier.GetInstanceIdentifier(), ServiceInstanceId{instance_id}};

    auto crawler_result =
        FlagFileCrawler{*i_notify_}.CrawlAndWatch(enriched_instance_identifier_with_instance_id_from_string);
    AMP_ASSERT_PRD_MESSAGE(crawler_result.has_value(), "Filesystem crawling failed");

    auto& [watch_descriptors, known_instances] = crawler_result.value();
    AMP_ASSERT_PRD_MESSAGE(watch_descriptors.size() == 1, "Outside tampering. Must contain one watch descriptor.");

    auto watch = StoreWatch(watch_descriptors.begin()->first, watch_descriptors.begin()->second);
    for (const auto& search_key : search_keys)
    {
        const auto search_iterator = search_requests_.find(search_key);
        LinkWatchWithSearchRequest(watch, search_iterator);
    }

    known_instances_.asil_b.Merge(std::move(known_instances.asil_b));
    known_instances_.asil_qm.Merge(std::move(known_instances.asil_qm));
}

void ServiceDiscoveryClient::OnInstanceFlagFileCreated(const WatchesContainer::iterator& watch_iterator,
                                                       const amp::string_view name) noexcept
{
    const auto& enriched_instance_identifier = std::get<EnrichedInstanceIdentifier>(watch_iterator->second);

    const auto event_quality_type = FlagFileCrawler::ParseQualityTypeFromString(name);
    switch (event_quality_type)
    {
        case QualityType::kASIL_B:
            known_instances_.asil_b.Insert(enriched_instance_identifier);
            mw::log::LogDebug("lola") << "LoLa SD: Added" << GetSearchPathForIdentifier(enriched_instance_identifier)
                                      << "(ASIL-B)";
            break;
        case QualityType::kASIL_QM:
            known_instances_.asil_qm.Insert(enriched_instance_identifier);
            mw::log::LogDebug("lola") << "LoLa SD: Added" << GetSearchPathForIdentifier(enriched_instance_identifier)
                                      << "(ASIL-QM)";
            break;
        case QualityType::kInvalid:
            [[fallthrough]];
        default:
            bmw::mw::log::LogError("lola")
                << "Received creation event for watch path" << GetSearchPathForIdentifier(enriched_instance_identifier)
                << "and file" << name << ", that does not follow convention. Ignoring event.";
    }
}

void ServiceDiscoveryClient::OnInstanceFlagFileRemoved(const WatchesContainer::iterator& watch_iterator,
                                                       amp::string_view name) noexcept
{
    const auto& enriched_instance_identifier = std::get<EnrichedInstanceIdentifier>(watch_iterator->second);

    const auto event_quality_type = FlagFileCrawler::ParseQualityTypeFromString(name);
    switch (event_quality_type)
    {
        case QualityType::kASIL_B:
            known_instances_.asil_b.Remove(enriched_instance_identifier);
            mw::log::LogDebug("lola") << "LoLa SD: Removed" << GetSearchPathForIdentifier(enriched_instance_identifier)
                                      << "(ASIL-B)";
            break;
        case QualityType::kASIL_QM:
            known_instances_.asil_qm.Remove(enriched_instance_identifier);
            mw::log::LogDebug("lola") << "LoLa SD: Removed" << GetSearchPathForIdentifier(enriched_instance_identifier)
                                      << "(ASIL-QM)";
            break;
        case QualityType::kInvalid:
            [[fallthrough]];
        default:
            bmw::mw::log::LogError("lola")
                << "Received creation event for watch path" << GetSearchPathForIdentifier(enriched_instance_identifier)
                << "and file" << name << ", that does not follow convention. Ignoring event.";
    }
}

Result<ServiceHandleContainer<HandleType>> ServiceDiscoveryClient::FindService(
    EnrichedInstanceIdentifier enriched_instance_identifier) noexcept
{
    std::lock_guard worker_lock{worker_mutex_};

    mw::log::LogDebug("lola") << "LoLa SD: find service for"
                              << GetSearchPathForIdentifier(enriched_instance_identifier);

    auto crawler_result = FlagFileCrawler{*i_notify_}.Crawl(enriched_instance_identifier);
    if (!crawler_result.has_value())
    {
        return bmw::MakeUnexpected(ComErrc::kBindingFailure, "Instance identifier does not have quality type set");
    }

    auto& known_instances = crawler_result.value();
    return GetKnownHandles(enriched_instance_identifier, known_instances);
}
}  // namespace lola
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
