// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#include "platform/aas/mw/com/impl/bindings/lola/uid_pid_mapping.h"

#include "platform/aas/lib/memory/shared/polymorphic_offset_ptr_allocator.h"
#include "platform/aas/mw/log/logging.h"

#include <cstdint>

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

std::pair<UidPidMappingEntry::MappingEntryStatus, uid_t> UidPidMappingEntry::GetStatusAndUidAtomic() noexcept
{
    constexpr std::uint64_t kMaskUid = 0x00000000FFFFFFFF;
    auto status_uid = key_uid_status_.load();
    const auto status_part = static_cast<std::uint32_t>(status_uid >> 32U);
    const auto uid_part = static_cast<std::uint32_t>(status_uid & kMaskUid);
    return {static_cast<UidPidMappingEntry::MappingEntryStatus>(status_part), static_cast<uid_t>(uid_part)};
}

void UidPidMappingEntry::SetStatusAndUidAtomic(MappingEntryStatus status, uid_t uid) noexcept
{
    key_uid_status_.store(CreateKey(status, uid));
}

UidPidMappingEntry::key_type UidPidMappingEntry::CreateKey(MappingEntryStatus status, uid_t uid) noexcept
{
    UidPidMappingEntry::key_type result = static_cast<std::uint16_t>(status);
    result = result << 32U;
    static_assert(sizeof(uid) <= 4, "For more than 32 bits we cannot guarantee the key to be unique");
#ifdef __QNX__
    // In QNX uid is signed. Technically speaking, given the type, the uid could be negative. In any case it does not
    // matter, we just need a value to be used as a key and needs to be converted always in the same way.
    const auto fixed_size_uid{static_cast<std::uint32_t>(uid)};
#else
    const std::uint32_t fixed_size_uid{uid};
#endif
    result |= fixed_size_uid;
    return result;
}

namespace detail_uid_pid_mapping
{

/// \brief Iterates through the given entries and updates the pid for the given uid, if an entry with the given uid
///        exists and is in the right state.
/// \param entries_begin start iterator/pointer for the entries
/// \param entries_end end iterator/pointer for the entries
/// \param uid uid for which the pid shall be registered/updated
/// \param pid new pid
/// \return if the given uid has been found, either the old/previous pid is returned (in case status was in kUsed) or
///         the new pid is returned, if status was in kUpdating. If uid wasn't found empty optional gets returned.
amp::optional<pid_t> TryUpdatePidForExistingUid(
    bmw::containers::DynamicArray<UidPidMappingEntry>::iterator entries_begin,
    bmw::containers::DynamicArray<UidPidMappingEntry>::iterator entries_end,
    const uid_t uid,
    const pid_t pid)
{
    // apply advance() in a for-loop because "it" is semantically is an iterator, but deduced type is a raw pointer.
    for (auto it = entries_begin; it != entries_end; std::advance(it, 1))
    {
        // extract out into a separate function
        const auto status_uid = it->GetStatusAndUidAtomic();
        const auto entry_status = status_uid.first;
        const auto entry_uid = status_uid.second;
        if (entry_status == UidPidMappingEntry::MappingEntryStatus::kUsed && entry_uid == uid)
        {
            // uid already exists. It is "owned" by us, so we can directly update pid, without atomic state changes ...
            pid_t old_pid = it->pid_;
            it->pid_ = pid;
            return old_pid;
        }
        else if (entry_status == UidPidMappingEntry::MappingEntryStatus::kUpdating && entry_uid == uid)
        {
            // This is a very odd situation! I.e. someone is currently updating the pid for OUR uid!
            // this could only be possible, when our uid/client app has crashed before, while updating the pid
            // for our uid.
            bmw::mw::log::LogWarn("lola")
                << "UidPidMapping: Found mapping entry for own uid in state kUpdating. Maybe we "
                   "crashed before!? Now taking over entry and updating with current PID.";
            it->pid_ = pid;
            it->SetStatusAndUidAtomic(UidPidMappingEntry::MappingEntryStatus::kUsed, uid);
            return pid;
        }
    }
    return {};
}

template <template <class> class AtomicIndirectorType>
// 
amp::optional<pid_t> RegisterPid(bmw::containers::DynamicArray<UidPidMappingEntry>::iterator entries_begin,
                                 bmw::containers::DynamicArray<UidPidMappingEntry>::iterator entries_end,
                                 const uid_t uid,
                                 const pid_t pid)
{
    const auto result_pid = TryUpdatePidForExistingUid(entries_begin, entries_end, uid, pid);
    if (result_pid.has_value())
    {
        return result_pid;
    }

    // \todo find a "sane" value for max-retries!
    constexpr std::size_t kMaxRetries{50};
    std::size_t retry_count{0};
    while (retry_count < kMaxRetries)
    {
        retry_count++;
        // Apply advance() in a for-loop because "it" is semantically is an iterator, but deduced type is a raw pointer.
        for (auto it = entries_begin; it != entries_end; std::advance(it, 1))
        {
            const auto status_uid = it->GetStatusAndUidAtomic();
            const auto entry_status = status_uid.first;
            const auto entry_uid = status_uid.second;

            if (entry_status == UidPidMappingEntry::MappingEntryStatus::kUnused)
            {
                auto current_entry_key = UidPidMappingEntry::CreateKey(entry_status, entry_uid);
                auto new_entry_key =
                    UidPidMappingEntry::CreateKey(UidPidMappingEntry::MappingEntryStatus::kUpdating, uid);

                if (AtomicIndirectorType<UidPidMappingEntry::key_type>::compare_exchange_weak(
                        it->key_uid_status_, current_entry_key, new_entry_key, std::memory_order_acq_rel))
                {
                    it->pid_ = pid;
                    it->SetStatusAndUidAtomic(UidPidMappingEntry::MappingEntryStatus::kUsed, uid);
                    return pid;
                }
            }
        }
    }
    return {};
}

/// Instantiate the method templates once for AtomicIndirectorReal (production use) and AtomicIndirectorMock (testing)
/// Those are all instantiations we need, so we are able to put the template definitions into the .cpp file.
template amp::optional<pid_t> RegisterPid<memory::shared::AtomicIndirectorMock>(
    bmw::containers::DynamicArray<UidPidMappingEntry>::iterator entries_begin,
    bmw::containers::DynamicArray<UidPidMappingEntry>::iterator entries_end,
    const uid_t uid,
    const pid_t pid);
template amp::optional<pid_t> RegisterPid<memory::shared::AtomicIndirectorReal>(
    bmw::containers::DynamicArray<UidPidMappingEntry>::iterator entries_begin,
    bmw::containers::DynamicArray<UidPidMappingEntry>::iterator entries_end,
    const uid_t uid,
    const pid_t pid);

}  // namespace detail_uid_pid_mapping
}  // namespace lola
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
