// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#ifndef PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_HANDLERBASE_H
#define PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_HANDLERBASE_H

#include "platform/aas/mw/com/impl/bindings/lola/messaging/messages/message_common.h"
#include "platform/aas/mw/com/impl/configuration/quality_type.h"
#include "platform/aas/mw/com/message_passing/i_receiver.h"
#include "platform/aas/mw/com/message_passing/message.h"

#include <atomic>
#include <shared_mutex>
#include <unordered_map>
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
/// \brief Base class of all (message) handler used by MessagePassingFacade.
class HandlerBase
{
  public:
    HandlerBase() noexcept = default;

    virtual ~HandlerBase() noexcept = default;

    HandlerBase(HandlerBase&&) = delete;
    HandlerBase& operator=(HandlerBase&&) = delete;
    HandlerBase(const HandlerBase&) = delete;
    HandlerBase& operator=(const HandlerBase&) = delete;

    /// \brief Registers message received callbacks for messages handled by SubscribeEventHandler at _receiver_
    /// \param asil_level asil level of given _receiver_
    /// \param receiver receiver, where to register
    virtual void RegisterMessageReceivedCallbacks(const QualityType asil_level,
                                                  message_passing::IReceiver& receiver) = 0;

  protected:
    using NodeIdTmpBufferType = std::array<pid_t, 20>;

    /// \brief Copies node identifiers (pid) contained within (container) values of a map into a given buffer under
    ///        read-lock
    /// \details
    /// \tparam MapType Type of the Map. It is implicitly expected, that the key of MapType is of type ElementFqId and
    ///         the mapped_type is a std::set (or at least some forward iterable container type, which supports
    ///         lower_bound()), which contains values, which directly or indirectly contain a node identifier (pid_t)
    ///
    /// \param event_id fully qualified event id for lookup in _src_map_
    /// \param src_map map, where key_type = ElementFqId and mapped_type is some forward iterable container
    /// \param src_map_mutex mutex to be used to lock the map during copying.
    /// \param dest_buffer buffer, where to copy the node identifiers
    /// \param start start identifier (pid_t) where to start search with.
    ///
    /// \return pair containing number of node identifiers, which have been copied and a bool, whether further ids could
    ///         have been copied, if buffer would have been larger.
    template <typename MapType>
    static std::pair<std::uint8_t, bool> CopyNodeIdentifiers(ElementFqId event_id,
                                                             MapType& src_map,
                                                             std::shared_mutex& src_map_mutex,
                                                             NodeIdTmpBufferType& dest_buffer,
                                                             pid_t start)
    {
        std::uint8_t num_nodeids_copied{0U};
        bool further_ids_avail{false};
        std::shared_lock<std::shared_mutex> read_lock(src_map_mutex);
        if (src_map.empty() == false)
        {
            auto search = src_map.find(event_id);
            if (search != src_map.end())
            {
                // we copy the target_node_identifiers to a tmp-array under lock ...
                for (auto iter = search->second.lower_bound(start); iter != search->second.cend(); iter++)
                {
                    dest_buffer.at(num_nodeids_copied) = *iter;
                    num_nodeids_copied++;
                    if (num_nodeids_copied == dest_buffer.size())
                    {
                        if (std::distance(iter, search->second.cend()) > 1)
                        {
                            further_ids_avail = true;
                        }
                        break;
                    }
                }
            }
        }
        return {num_nodeids_copied, further_ids_avail};
    }
};

}  // namespace lola
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_HANDLERBASE_H
