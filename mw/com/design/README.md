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



# Software Design Description of ara::com
This and the following documents shall act as software architecture for the BMW Implementation of `ara::com` based
on the adaptive [AUTOSAR 20.11 Standard](https://www.autosar.org/fileadmin/user_upload/standards/adaptive/20-11/AUTOSAR_SWS_CommunicationManagement.pdf).

## How to Start
In order to start contributing or following this architecture description, one should be familiar with the basic
adaptive AUTOSAR concepts and more specifically with the basic usage of `ara::com` API. Therefore, it makes sense to
start reading the `ara::com` [explanatory document](https://www.autosar.org/fileadmin/user_upload/standards/adaptive/20-11/AUTOSAR_EXP_ARAComAPI.pdf).
After that it might makes sense to check the code of an example application like `//platform/aas/platform/aas/mw/com/test/bigdata/...`.

Once familiar with the API and the basic use cases, one should continue with the [Basic architectural thoughts](#basic-architectural-thoughts), before
going into detail of the [involved components](#involved-components).

The high-level requirements describing `ara::com` are provided by adaptive AUTOSAR. In our case these are mainly
[AUTOSAR 20.11 Standard](https://www.autosar.org/fileadmin/user_upload/standards/adaptive/20-11/AUTOSAR_SWS_CommunicationManagement.pdf).
Until now there was not further break down of requirements onto different implementation specific components.

The architectural design concept and the corresponding requirements on system and component level can be found
[here](/swh/xpad_documentation/blob/master/architecture/architecture_concept/communication/lola/README.md)

## Basic architectural thoughts
Before going into more detail on specific architectural parts, a general mindset shall be shared that is kept in mind
during the whole design process.

1. Using shared memory as the underlying IPC mechanism

   Shared Memory shall be used as underlying IPC mechanism to realize message exchange between ara::com proxies and
   skeletons. Main reasons for this decision are:
   1. Feedback from ongoing work on qualifying Linux for ASIL-B
      _safe_ communication shall rather use shared memory as IPC mechanism instead of other ones (pipes, sockets, message queues),
      since it is far easier to qualify, since less kernel code is involved.
   2. Performance advantage with huge message payloads
      By using shared memory an actual zero-copy IPC-mechanism can be implemented, without serialization.
      In addition it is known to be the most performant one [1]
   3. Fits better to the heavily used "last-is-best" use-case
      A high-frequent event sender, which meets a lazy event-receiver runs into the problem,
      that his last/newest events will be dropped, when kernel buffers are full, because of a lazy receiver.
      This is not the behavior, that is typically wanted. In order to avoid this pit fall, one needs to have 
      _full_ control over the memory management. Which is easier to achieve when using shared memory instead of
      an IPC mechanism that relies on (restricted) kernel buffers.

2. General Non-Blocking design

   The distributed access to the shared memory buffers used for message exchange between ara::com proxy and skeleton instances
   need some synchronization. But the underlying "exclusive access" to some data structures is typically very short-term. 
   In such cases it shall be fully avoided to use higher level C++ synchronization mechanisms such as mutexes (which boil down
   to kernel level sys-calls like `futex()`). Instead userland synchronization mechanisms built around std::atomic
   (C++ memory model with memory barriers) and effective [spinlocks](https://rigtorp.se/spinlock/) shall be used. 
  
3. Pluggable separation between binding-specific and binding-independent parts
   
   Even though there is currently no plan to introduce another IPC layer besides the shared memory one,
   there will be another binding to support communications between different ECUs based on `SOME/IP`.
   This alone already qualifies to introduce an abstraction layer.
   During the first generation of the xPAD project it has been made obvious that including deployment specific
   details, like the used communication binding, into adaptive Applications has caused tremendous issues like:
   1. Issues in variant handling
   
      adaptive Applications needed to be compiled and linked multiple times, since single instance numbers deep
      in the deployment have been different
      
   2. Issue in compile times
   
      Due to the heavy dependencies on for example the boardnet, applications needed to be recompiled just
      by deployment changes in the boardnet. This often caused that even parts that are independent of the deployment
      needed to be recompiled because there was no clear separation given.   

   Thus, it shall be possible include deployment (or binding specifics) as shared libraries. This enforces a clear separation.

4. Avoid serialization for ECU internal communication

   In order to achieve high performance, it shall be avoided to introduce any kind of serialization while communicating
   within the same ECU. This is only possible when a common endianness and a common alignment for the exchanged data
   structures can be ensured. This is most certainly possible, but needs to be proven in the further architectural work.
   
   This also means that interface changes need to be correctly guided by a versioning schema, in order to ensure
   backward compatibility. A feature, like it is implemented in SOME/IP, that the serialization can give some kind of
   backward compatibility can not be implemented.

5. Dynamic detection for memory allocation and serialization needs. 

   At the point of setting up a communication path between a proxy and skeleton instance, it shall be detected, whether 
   serialization is needed and where the memory shall be allocated (e.g. directly in shared memory or in heap).
   To achieve this _before_ the first communication is done, the relevant information must be already figured out
   during the service discovery.
   
6. Using SOME/IP serialization format

   Due to the fact that BMW requires any Ethernet based ECU external communication to use the SOME/IP protocol, the respective
   serialization format needs to be implemented anyhow. This is independent whether or not the existing vsomeip implementation
   is re-used as network router.

7. True Zero-Copy based implementation for IPC 

   It shall be mandatory for the IPC implementation to offer a way to exchange data in a zero-copy manner.
   This means that the `<Event>::Allocate/<Event>::Send` must already allocate the memory at the correct location. In our case
   this means it needs to allocate already shared-memory. The subscribed local proxy instances, when reading event data via
   `GetNewSamples/callback f` mechanism get directly the pointer to this written sample within the shared memory object.

## Involved Components and Dependencies

Our implementation of `ara::com` depends on the following components/libraries:

* `//platform/aas/mw/com/message_passing`: This message passing infrastructure is currently located in the same top
  folder as our `ara::com` implementation, although it is a complete independent/standalone layer (location/structure
  will be refactored in the future). Details of its usage can be found
  [here](events_fields/README.md#notifications-between-skeleton-and-proxy). It also enlists the dependencies it has and
  which therefore become transient dependencies for our `mw::com` (`ara::com`) implementation.
* `/platform/aas/mw/log`
* `//platform/aas/lib/concurrency`
* `//platform/aas/lib/json:json_parser`
* `//platform/aas/lib/memory/shared`: Our whole shared-memory handling is implemented in this lib.
* `//platform/aas/lib/os:errno_logging`
* `//platform/aas/lib/os:fcntl`
* `//platform/aas/lib/os:unistd`
* `//platform/aas/lib/result:result`
* amp (/swh/amp)

## Pictures and Diagrams
We decided to put the architectural description close to the source code to enable a common source of truth, ensure that
every developer directly has access to the architecture description and increase the probability that documentation and design
are adjusted if source code is changed in future.

This comes with the drawback that we shall try to not place binary blobs into the repository.
Thus, we decided to use [UMLet](https://www.umlet.com) for drawing any UML-related diagrams (it can be used on any OS).

For graphics we decided to only use SVGs.

In both cases we have no binary blob and its representation can easily be changed. Meaning, you don't need another
file like a PowerPoint presentation, to create the pictures.

## Notes on Detailed Design Diagrams
**Note:** For the sake of class diagrams, a parameter in the form of "Object o" and "const Object o" are considered the
same and in code, it could be any of the two forms. From the design point of view, both are input only parameters.

**Note:** Copy/move constructor/assignments are only present in the class diagrams when they are crucial for the
design.
Destructors are not present in the class diagrams because all classes have destructors.

## Acceptable use of the `friend` keyword
Some C++ guidelines discourage any use of the `friend` keyword outside of operator overloading. The reason why it is
considered bad is because, in some cases, it might reduce encapsulation and increase maintainability costs.

However, in `mw::com` we use it in certain patterns to increase encapsulation.
We use a "View/MutableView" pattern in `com::impl` namespace that requires the `friend` keyword. Here is the
explanation of this pattern.
  
There are three levels of relevant encapsulation: a fully public API (i.e., methods and members that the end user will
have access to), a fully private API, and the API restricted to a module. C++ does not provide a native keyword for this
since, as of now (c++17), the standard does not have a concept of modules. Thus, whenever we are in need of module level
visibility, i.e. an API that should be visible to the entire implementation name space but not visible to the end user,
we use a pattern involving a friend class. In these cases, an `Example` class has a public API exposed to the end user,
and one or both friend classes, named `ExampleView` and `ExampleMutableView`, will provide view or modification access,
respectively, to the private members of the `ExampleClass`. We then restrict the visibility of these friend classes to
the implementation namespace through an [AoU](), which effectively 
provides us with module-level encapsulation.
Since this use of the `friend` keyword does not break encapsulation but provides us with a new kind of encapsulation
that we need, we find that in spirit it does not contradict the guideline, and we deem such use as acceptable in
`com::impl` namespace.

## Assumptions of Use(AoUs)
This library calls std::terminate. It could happen in following scenarios.

1. Mandatory configuration information is not provided().
2. It is not possible to recover from a specific state.

 LoLa isn't directly creating artifacts in the filesystem. It uses lib/memory/shared and mw/com/message_passing. Both of them creates artifacts in the file system.

 lib/memory/shared creates two different kind of artifacts.
 (A) lock-files under /tmp and (B) shm-object files under /dev/shmem or /dev/shm
 These artifacts are not removed when std::terminate is called. These left-overs could lead to SharedMemoryFactory::Create calls to fail. And therefore leads to a termination of a LoLa app

We expect that system is rebooted whenever a Lola app is terminated.(). In order to remove these artifacts.

mw/com/message_passing also creates artifacts in the filesystem. QNX resource manager takes care of the cleanup.

Complete AoUs are present under .

## References
   
[1] A. Venkataraman and K. K. Jagadeesha, ["Evaluation of inter-process communication mechanisms"](http://pages.cs.wisc.edu/~adityav/Evaluation_of_Inter_Process_Communication_Mechanisms.pdf)
    Arquitecture, vol. 86, p. 64, 2015 
