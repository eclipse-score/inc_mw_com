<!--- 
*******************************************************************************>
Copyright (c) 2024 Contributors to the Eclipse Foundation
See the NOTICE file(s) distributed with this work for additional
information regarding copyright ownership.
This program and the accompanying materials are made available under the
terms of the Apache License Version 2.0 which is available at
https://www.apache.org/licenses/LICENSE-2.0
SPDX-License-Identifier: Apache-2.0 #
*******************************************************************************
 ---> 



# Layout of Shared Memory Objects

## Introduction

This section shall provide a compact overview over the entire shared-memory layout. I.e. the data structures stored
within shared-memory by `Lola`. Other parts of the design documentation e.g.
[event and field description part](../events_fields/README.md#introduction) already describe/hint in their class
diagrams via the stereotype `<<SharedMemory>>` in the following form:

![Stereotype](artifacts/stereotype.svg)

whether some classes (instances of it) are partially placed within shared-memory. We use the same stereotype here, but
give a complete overview.

According to our
[architecture concept](/swh/xpad_documentation/blob/master/architecture/architecture_concept/communication/lola/README.md)
LoLa uses two different types of shared-memory objects per service instance:

* shared-memory object for **Data**
* shared-memory object for **Control**

The **specific** content of those shared-memory objects will be described below. But first the **communality** of those two
shared-memory types are described.

## Shared-Memory Object as Memory Resource

All shared-memory objects are represented by `bmw::memory::shared::SharedMemoryResource` and as such they inherit from
`ManagedMemoryResource` and `amp::pmr::memory_resource`. Therefore, they provide the mechanisms needed by `C++17`
allocator/pmr aware (STL) containers to serve as a memory resource providing memory allocation. To fulfill this task
`SharedMemoryResource` uses a control block (`SharedMemoryResource::ControlBlock`), which is located at the beginning of
the address space of a shared-memory object/`SharedMemoryResource`. It provides the bookkeeping mechanism for the memory
available to the "user" of the `SharedMemoryResource` (noting, where the first user available memory starts, how many
bytes are already allocated/left).

The following class diagram provides an overview of the classes, whose instances are placed within shared-memory and
also a rough indication how the provider side (in the form of `LoLa` skeleton) and the consumer side (in the form of
`LoLa` proxy/proxy-event) interact with them.

![Class View](/swh/ddad_platform/aas/mw/com/design/shared_mem_layout/artifacts/shared_mem_layout_classdiagram.uxf)

This object diagram below (showing a concrete instantiation of the class diagram above for some example service instance)
depicts more clearly the relation between the `SharedMemoryResource` instances representing a shared-memory object and the
anchor/root elements placed within shared-memory:

![Object View](/swh/ddad_platform/aas/mw/com/design/shared_mem_layout/artifacts/shared_mem_layout_objectdiagram.uxf)

So - the first object/element created within the shared-memory object for **Data** (represented by `SharedMemoryResource`
instance `shmResource_storage_SI_1`) is the instance `serviceDataStorage_SI_1` of class `ServiceDataStorage` and
therefore the corresponding `ControlBlock::start` points to `serviceDataStorage_SI_1`.

Accordingly, the first object/element created within the shared-memory object for the QM **Control** (represented by
`SharedMemoryResource` instance `shmResource_control_qm_SI_1`) is the instance `serviceDataControl_qm_SI_1` of class
`ServiceDataControl` and therefore the corresponding `ControlBlock::start` points to `serviceDataControl_qm_SI_1`.

In the object diagram example we used a service instance with additional ASIL-B support, so besides a QM
`SharedMemoryResource` we also do have an ASIL-B `SharedMemoryResource`.

## Shared-Memory Object for Data

The anchor/root element (i.e. the location, where `SharedMemoryResource::ControlBlock::start` points to), which gets
placed into the `SharedMemoryResource` for **Data** is an instance of `lola::ServiceDataStorage`. It gets created by a
`LoLa` skeleton instance during `lola::Skeleton::PrepareOffer()` within the initialization callback of the
newly created shared-memory object.
This anchor element of the `SharedMemoryResource` contains two members:
* `events_`: a map for the data storage (slots) of every event the service provides.
  Note, that the storage slots, which are effectively vectors of `SampleType`s:
  ```
  template <typename SampleType>
  using EventDataStorage = bmw::memory::shared::Vector<SampleType>
  ```
  are stored here as "type-erased" `bmw::memory::shared::OffsetPtr<void>`, because every slot-vector has a different
  type and only later when the skeleton events register themselves dynamically or access that storage they are cast to
  the effective/concrete type. 
  
* `events_metainfo_`: a map for meta information of every event the service provides
* `skeleton_pid_`: PID of the provider/skeleton process.

## Shared-Memory Object for Control

The anchor/root element (i.e. the location, where `SharedMemoryResource::ControlBlock::start` points to), which gets
placed into the `SharedMemoryResource` for **Control** is an instance of `lola::ServiceDataControl`. It also gets
created by a `LoLa` skeleton instance during `lola::Skeleton::PrepareOffer()` within the initialization
callback of the newly created shared-memory object.
This anchor element of the `SharedMemoryResource` contains the member:
* `event_data_control_`: a map containing the control slots of every event the service provides.

