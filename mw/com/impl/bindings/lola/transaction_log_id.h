// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#ifndef PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_TRANSACTION_LOG_ID_H
#define PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_TRANSACTION_LOG_ID_H

#include <sys/types.h>

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

/// \brief A unique identifier for identifying / retrieving a TransactionLog
///
/// The TransactionLogId is needed so that a Proxy / Skeleton service element can retrieve its own TransactionLog after
/// a crash. Note. This identifier is not unique for different instances of the same service WITHIN the same process.
/// E.g. a SkeletonEvent / ProxyEvent of the same service that are created within the same process will have the same
/// TransactionLogId. Similarly, 2 instantiations of the same ProxyEvent will share the same TransactionLogId. This is
/// acceptable since ALL service elements within a process will live / die together. So in the TransactionLogSet
/// rollback mechanism, we can simply rollback all TransactionLogs corresponding to a given TransactionLogId.
using TransactionLogId = uid_t;

}  // namespace lola
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_BINDINGS_LOLA_TRANSACTION_LOG_ID_H
