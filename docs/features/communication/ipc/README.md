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



# LoLa - Inter Process Communication (IPC)

LoLa (Low Latency) IPC represents a safe zero-copy shared-memory based IPC mechanism.
This document describes the high-level architecture concept.

## Problem Formulation

In the beginning of the xPAD project, nearly all customer functions were deployed into one executable.
One of the reasons was, that there is the need for high frequent data exchange between single parts and the provided
IPC mechanisms by our `ara::com` vendor were not performant enough.

Thus, the need arose for a IPC mechanism that:

- utilizes zero-copy
- provides minimum latency overhead
- is ASIL-B ready

Due to the limited time-frame it was decided that LoLa will not implement a fully-fledged `ara::com` but rather
only a subset of functionality. Namely, it only provides event-based communication. Which is advanced by time.

## Technical Concept Description

Since LoLa will not implement all communication mechanisms like `methods` and `fields` that `ara::com` foresees.
It rather needs to exist in parallel and needs to be operated independent of any adaptive AUTOSAR Stack[[1]](#relevant-software-component-requirements).
Still, it will be necessary to migrate certain applications towards Lola. In order to make this as easy as possible
and to ensure that developers don't need to learn another API, adaptive AUTOSAR shall be mimicked where possible - only with the difference
that we will not use the `ara` C++ namespace, but a BMW one[[2]](#relevant-software-component-requirements). The [sub-section](#features-from-adaptive-autosar) will go into more detail on which adaptive AUTOSAR features specifically will be needed.

![LoLa and ara::com in parallel](/swh/ddad_platform/aas/docs/features/ipc/lola/assets/lola_ara_com_parallel.uxf)

The Communication Management Specification of adaptive AUTOSAR foresees two major building blocks that implement `ara::com`.
One is the so-called *frontend*, the other one *network binding*. The idea is that the *frontend* does not change depending
on which *network binding* is selected. Meaning, the *frontend* stays the same no matter if we use SOME/IP or Shared Memory
as *network binding*. In order to be as flexible as possible and reduce compilation times in the CI, we want to follow the
*Multi Target Build* concept. In summary it shall be possible to configure on runtime which *network binding* shall be used[[3]](#relevant-software-component-requirements).
While this does not make sense at the moment, since we have only one network binding, it ensures that no deployment information
leaks into the *frontend* and thus we can reduce compilation times (e.g. by only generating C++ libraries per interface).

The basic idea of our LoLa concept is to use two main operating system facilities:

1. *Shared Memory[[4]](#relevant-software-component-requirements)*: Shall be used for the heavy lifting of data exchange [[6]](#relevant-software-component-requirements)
2. *Message Passing[[5]](#relevant-software-component-requirements)*: Shall be used as notification mechanism [[7]](#relevant-software-component-requirements)

We decided for this side channel since implementing a notification system via shared memory would include
the usage of condition variables. These condition variables would require a mutex, wich would require
read-write access. This could lead to the situation that a malicious process could lock the mutex
forever and thus destroy any event notification. In general we can say that any kind of notification
shall be exchanged via message passing facilities [[7]](#relevant-software-component-requirements). The [sub-section](#message-passing-facilities) below will go into more detail for the Message Passing Facilities.

The usage of shared memory has some implications. First, any synchronization regarding thread-safety / process-safety
needs to be performed by the user. Second, the memory that is shared between the processes is directly mapped into
their virtual address space. This implies that it is easy for a misbehaving process to destroy or manipulate any data
within this memory segment. In order to cope with the latter, we split up the shared memory into three segments.

- First, a segment where only the to-be-exchanged data is provided. This segment shall be read-only to consumer
and only writeable by the producer [[8]](#relevant-software-component-requirements). This will ensure that nobody besides the
producer process can maniuplate the provided data.
- The second and third segment shall contain necessary control information
for the data segment[[9]](#relevant-software-component-requirements). Necessary control information can include atomics that are
used to synchronize the access to the data segments. Since this kind of access requires write access, we split the shared memory
segments for control data by ASIL Level. This way it can be ensured that no low-level ASIL process interferes with higher level ones.
More information on shared memory handling can be found in [sub-section](#shared-memory-handling).

![Shared Memory split](./assets/MixedCriticality_1_N.svg)

One of the main ideas in this concept is the split of control data from sample (user) data.
In order to ensure a mapping, the shared memory segments are divided into slots
[[10]](#relevant-software-component-requirements) [[11]](#relevant-software-component-requirements). By convention, we then define
that the slot indexes correlate. Meaning, slot 0 in the control data is user to synchronize slot 0 in the sample data.
More information on these slot and the underlying algorithm can be found in [sub-section](#synchronization-algorithm).

![Relation Control-Block, Data-Block](./assets/ControlData_SampleData.svg)

### Features from adaptive AUTOSAR

As already mentioned earlier we will only implemente the `ara::com` specification partially. In the following paragraph, we
will list all necessary requirements from the SWS Communication Management of the 19.03 adaptive AUTOSAR Standard.

We will further give reasoning for deviations or if parts are not implemented

#### Header File and C++ Namespace Structure

In general we take over all structuring parts that define where which types are defined.
It shall only be noted that we will deviate when it comes to implementing the ara-namespace.
See [[2]](#relevant-software-component-requirements).

1. [Folder Structure]() <!--  -->
2. [Service header files existence]() <!--  -->
3. [Inclusion of common header file]() <!--  -->
4. [Namespace of Serivce header file]() <!--  -->
5. [Service skeleton namespace]() <!--  -->
6. [Service proxy namespace]() <!--  -->
7. [Service events namespace]() <!--  -->
8. [Common header file existence]() <!--  -->
9. [Inclusion of Types header file]() <!--  -->
10. [Inclusion of Implementation Types header files]() <!--  -->
11. [Service Identfier Type definitions]() <!--  -->
12. [Namespace for Service Identifier Type definitions]() <!--  -->
13. [Types header file existence]() <!--  -->
14. [Types header file namespace]() <!--  -->
15. [Data Type declarationsin Types header file]() <!--  -->
16. [Implementation Types header files existence]() <!--  -->
17. [Data Type definitions for AUTOSAR Data Types in Implementation Types header files]() <!--  -->
18. [Implementation Types header file namespace]() <!--  -->

#### API Types

Also the major API types are taken over without changes. We only have to adjust requirement [19] since the underlying
requirement, , would require this data-type in the `ara::core` namespace.
Here we will again diverge to fulfill requirement [[2]](#relevant-software-component-requirements).

19. [Instance Specifier Class]() <!--  -->
20. [Instance Identifier Class]() <!--  -->
21. [Instance Identifier Container Class]() <!--  -->
22. [Find Service Handle]() <!--  -->
23. [Handle Type Class]() <!--  -->
24. [Copy semantics of Handle Type Class]() <!--  -->
25. [Move semantics of Handle Type Class]() <!--  -->
26. [Service Handle Container]() <!--  -->
27. [Find Service Handler]() <!--  -->

#### Event Types

For events we will provide only the types listed below. Any E2E types are not necessary, since we do *not* need support
End-2-End protection (see [[17]](#relevant-system-requirements)). Also types associated with *Subscription State* are
not necessary, since there is no use-case on customer function side, so we try to keep the overhead minimal. This includes
unused types like custom future/promise implementations, which are only necessary for method support. Also custom
variant or optional types will not be implemented, since these are already provided by **amp**. Last but not least, the *ScaleLinearAndTexttable*
class also has no usage within BMW, thus it can be skipped also.

28. [Sample Pointer]() <!--  -->
29. [Sample Allocatee Pointer]() <!--  -->
30. [Event Receive Handler]() <!--  -->
31. [Subscription State]() <!--  -->

#### Communication Payload Types

Adaptive AUTOSAR clearly defines which data-types can be transmitted via its communication mechanism.
We orient ourselves strongly on this concepts. Our adaptive AUTOSAR generator, `aragen` shall support
the following requirements.

32. [Data Type Mapping]() <!--  -->
33. [Provide data type definitions]() <!--  -->
34. [Avoid Data Type redeclaration]() <!--  -->
35. [Naming of data type by short name]() <!--  -->
36. [Supported Primitive Cpp Implementation Types]() <!--  -->
37. [Primitive fixed with integer types]() <!--  -->
38. [StdCppImplementationDataType of category ARRAY with one dimension]() <!--  -->
39. [Array Data Type with more than one dimension]() <!--  -->
40. [Structure Data Type]() <!--  -->
90. [Element specification typed by CppImplementationDataType]()<!--  -->
41. [StdCppImplementationDataType with the category STRING]() <!--  -->
42. [StdCppImplementationDataType with the category VECTOR with one dimension]() <!--  -->
43. [Vector Data Type with more than one dimension]() <!--  -->
44. [Data Type redefinition]() <!--  -->
45. [Enumeration Data Type]() <!--  -->

It shall be noted that some specifics are not supported for transmission and will not be generated. This includes:

- associative maps
- variants
- optionals in struct
- ScaleLinearAndTexttable
- custom allocators

The latter is especially important, since our implementation will need a custom allocator to ensure correct shared memory handling.
In addition to these requirements, we need to clearly specify the max-size for each container. This is due to the fact
that the shared memory needs to be preallocated[[4]](#relevant-system-requirements). If we need to preallocate on startup,
the maximum size needs to be calculated in advance[[12]](#relevant-software-component-requirements). In order to do that, it is necessary to know the maximum number of elements in each
container. This solves the same underlying issue as `` and `SW_CM_00450` in a more generic way, thus a custom BMW extension.

<!--- TODO: fix reference - does not exist--->
46. `[Shall enable definition of max-elements for container types]()`

The error concept cannot be taken over as well. This relies heavily on the infrastructure provided by `ara::core`
Within BMW we have a similar infrastructure within `bmw::Result`. Instead of following the requirements
``, `` and ``, we define that any error shall be reported via `bmw::Result`.

<!--- TODO: fix reference - does not exist--->
47. `[Utilize bmw::Result for any error reporting]()`

#### API Reference

The general API for proxies and skeletons can be taken over completely.

48. [Service skeleton class]() <!--  -->
49. [Service skeleton Event class]() <!--  -->
50. [Service proxy class]() <!--  -->
51. [Service proxy Event class]() <!--  -->
52. [Declaration of Construction Token]() <!--  -->
53. [Creation of Construction Token]() <!--  -->
54. [Method to offer a service]() <!--  -->
55. [Method to stop offering a service]() <!--  -->

#### Skeleton creation

There are many ways to create a skeleton. Within BMW we only want to support one way,
using exceptionless mechanisms and Instance Specifier. All other methods are not supported.
Since we don't support method processing either, our constructors don't allow to specify the processing mode.

56. [Exception-less creation of service skeleton using Instance Spec]() <!--  -->
57. [Copy semantics of service skeleton class]() <!--  -->
58. [Move semantics of service skeleton class]() <!--  -->

#### Event sending

59. [Send event where application is responsible for the data]() <!--  -->
60. [Send event where Communication Management is responsible for the data]() <!--  -->
61. [Allocating data for event transfer]() <!--  -->

#### Find Service & Proxy creation

The same story is true for proxies. We only allow polling based search of proxies and only with InstanceSpecifier.
This also excludes the support for *ANY*.

62. [Find service with immediately returned request using Instance Spec]() <!--  -->
63. [Exception-less creation of service proxy]() <!--  -->
64. [Copy semantics of service proxy class]() <!--  -->
65. [Move semantics of service proxy class]() <!--  -->

#### Proxy Event Handling

The proxy event handling is mostly the same as in `ara::com`. Only the re-establishment for subscriptions (``) and
any kind of subscription state query is not supported. Former needs to include a fully working service discovery, which was
dismissed due to timing constraints. Latter is not used within BMW and was thus dismissed.

66. [Method to subscribe to a service event]() <!--  -->
67. [Ensure memory allocation of maxSampleCount samples]() <!--  -->
68. [Method to unsubscribe from a service event]() <!--  -->
69. [Method to update the event cache]() <!--  -->
70. [Signature of Callable f]() <!--  -->
71. [Sequence of actions in GetNewSamples]() <!--  -->
72. [Return Value]() <!--  -->
73. [Reentrancy]() <!--  -->
74. [Query Free Sample Slots]() <!--  -->
75. [Return Value of GetFreeSampleCount]() <!--  -->
76. [Calculation of Free Sample Count]() <!--  -->
77. [Possibility of exceeding sample count by one]() <!--  -->
78. [Enable service event trigger]() <!--  -->
79. [Disable service event trigger]() <!--  -->

#### Multi-Target-Build

As stated already earlier, we want that our implementation is Multi-Target-Build ready.
Same does adaptive AUTOSAR, which is why we can take these requirements over.

80. [Change of Service Interface Deployment]() <!--  -->
81. [Change of Service Instance Deployment]() <!--  -->
82. [Change of Network Configuration]() <!--  -->

#### Security

The security chapter of the SWS Communication Management defines a possibility to
restrict communication. While this is generally good, we agreed with our stakeholders
that this kind of functionality will not be configured via ARXML. Thus, we will not
follow these requirements (e.g. SWS_COM_90002), since they describe the direct connection
with the AUTOSAR Meta Model. Our security requirements will be custom-made in the lower sections.

#### Additional Functional Requirements

In addition to the previous API centry requirements, there are some more functional requirements
which are grouped in this section. Any network binding specific requirements like the ones for DDS
or SOME/IP are obviously irrelevant, since we implement a User-Defined-Network binding.

83. [Uniqueness of offered service]() <!--  -->
84. [Protocol where a service is offered]() <!--  -->
85. [InstanceSpecifier check during creation of service skeleton]() <!--  -->
86. [FIFO semantics]() <!--  -->
87. [No implicit context switches]() <!--  -->
88. [Event Receive Handler call serialization]() <!--  -->
89. [Functionality afte event received]() <!--  -->

### Message Passing Facilities

The Message Passing facilities, under QNX this will be implemented by QNX Message Passing, will *not* be used to synchronize the access
to the shared memory segments. This is done over the control segments. We utilize message passing for notfications only.
These notifications include:

- subscribe / unsubscribe[[24]](#relevant-software-component-requirements)
- event notification[[25]](#relevant-software-component-requirements)

This is done, since there is no need to implement an additional notification handling via shared memory, which would only
be possible by using mutexes and condition variables. The utilization of mutexes would make the implementation of a wait-free algorithms
more difficult. As illustrated in the graphic below a process should provide one message passing port to receive data for each supported ASIL-Level[[26]](#relevant-software-component-requirements).
In order to ensure that messages received from QM processes will not influence ASIL messages, each message passing port shall use a custom thread to wait for new messages[[27]](#relevant-software-component-requirements). Further, it must be possible to register callbacks for mentioned messages[[28]](#relevant-software-component-requirements).
These callbacks shall then be invoked in the context of the socket specific thread[[29]](#relevant-software-component-requirements). This way we can ensure
that messages are received in a serialized manner.

![](./assets/Lola_Messaging.svg)

### Shared Memory Handling

POSIX based operating systems generally support two kinds of shared memory: file-backed and anonymous.
Former is represented by a file within the file-system, while the latter is not visible directly to other processes. We decide for former,
in order to utilize the filesystem for a minimal service discovery[[13]](#relevant-software-component-requirements),[[14]](#relevant-software-component-requirements).
In order to avoid fault propagation over restarts of the system, any shared memory communication shall not be persistent[[15]](#relevant-software-component-requirements).
Processes will identify shared memory segments over their name. The name will be commonly known by producers and consumers and deduced by additional
parameters like for example service id and instance id[[16]](#relevant-software-component-requirements). When it comes to the granularity of the data stored
in the shared memory segments, multiple options can be considered. We could have one triplet of shared memory segments per process or one triplet
of shared memory segments per event within a service instance. Former would make the ASIL-Split of segments quite hard, while the latter would explode the number of necessary segments within the system. As trade-of we decided to have one triplet of shared memory segments per service instance[[17]](#relevant-software-component-requirements).

It is possible to map shared memory segments to a fixed virtual address. This is highly discouraged by POSIX and leads to
undefined behaviour[[18]](#relevant-software-component-requirements). Thus, shared memory segments will be mapped to different virtual adresses. In consequence
no raw pointer can be stored within shared memory, since it will be invalid within another process. Only offset pointer (fancy pointer, relative pointer)
shall be stored within shared memory segments[[19]](#relevant-software-component-requirements).

The usage of shared memory does not involve the operating system, after shared memory segments are setup. Thus, the operating system
can no longer ensure freedom from interference between processes that have access to these shared memory regions. In order to restrict
access we use ACL support of the operating system[[20]](#relevant-software-component-requirements), [21](#relevant-software-component-requirements). In addition
to the restricted permissions, we have to ensure that a corrupted shared memory region cannot influence other process-local memory regions.
This can be ensured by performing *Active Bounds Checking*. So the only way how data corruption could propagate throughout a shared
memory region is if a pointer within a shared memory region points out of it. Thus, a write operation to such a pointer could forward
memory corruption. The basic idea to overcome such a scenario is, that we check that any pointer stays within the bounds of the shared memory region.
Since anyhow only offset pointer can be stored in a shared memory region, this active bound check can be performed whenever a offset pointer
is dereferenced[[22]](#relevant-software-component-requirements). The last possible impact can be on timing. If another process for example wrongly locks a mutex
within the shared memory region and another process would then wait for this lock, we would end up in a deadlock. While this should not harm
any safety goal, we still want to strive for wait-free algorithms to avoid such situations[[23]](#relevant-software-component-requirements).

### Synchronization Algorithm

A slot shall contain all necessary meta-information in order to synchronize data access[[30]](#relevant-software-component-requirements).
This information most certainly needs to include a timestamp to indicate the order of produced data within the slots. Additionally, a use count is needed, indicating if a slot is currently in use by one process. The concrete data is implementation defined and must be covered by the detailed design.

The main idea of the algorithm is that a producer shall always be able to store one new data sample[[31]](#relevant-software-component-requirements).
If he cannot find a respective slot, this indicates a contract violation, which indicates that a QM process misbehaved.
In such a case, a producer should exclude any QM consumer from the communication[[32]](#relevant-software-component-requirements).

This whole idea builds up on the split of shared memory segements by ASIL levels. This way we can ensure that an QM process
will not *degradate* the ASIL Level for a communication path. In another case, where we already have a QM producer, it is
possible for an ASIL B consumer to consume the QM data. In that case the data will always be QM since it is impossible for the middleware to apply additional
checks to enhance the quality of data. This can only be done on application layer level.

![](./assets/QMCriticality_1_N.svg)

## Requirements

This section gives an overview of the incoming requirements towards this concept (system requirements) and the
outgoing requirements, derived by this concept (software component requirements).

### Relevant System Requirements

The codebeamer aggregation can be found [here]().

1. [Intra-SoC communication using events]()
2. [Support for zero-copy shared memory IPC]()
3. [Support for synchronous sending of data over IPC]()
4. [Prevent memory fragmentation in real-time processes]()
5. [IPC synchronization support]()  
6. [No direct usage of POSIX IPC functions by application]()
7. [IPC communication shall be whitelisted]()
8. [The platform shall provide a mechanism to block IPC communication]()
9. [All platform processes shall use a different user id]()
10. [No platform process shall run as the user root]( )
11. [All platform processes shall run as a normal user with limited privileges]( )
12. [The platform shall limit the number of shared resources between processes]()
13. [IPC communication shall be integrity-protected]()
14. [The CDC Platform shall have the ability to access IPC (ara::com) communication]()
15. [IPC Tracing for Development Purposes]()
16. [Internal ECU Communication before Middleware Startup]()
17. [Use end-to-end protection for communication]()
18. [Freedom from interference on Application Software]()
19. [Static and automatic memory]()

### Relevant Software Component Requirements

The codebeamer aggregation can be found [here]()

1. [Operate LoLa in parallel to adaptive AUTOSAR]()
2. [Use bmw specific namespace]()
3. [Enable Multi-Target-Build]()
4. [OS shall provide Shared Memory IPC]()
5. [OS shall provide Message Passing IPC]()
6. [User data shall be exchanged via Shared Memory]()
7. [Notifications shall be exchanged via Message Passing]()
8. [User data shall be provided in a separate read-only shared memory segment]()
9. [One shared memory segment per ASIL level for control data]()
10. [The shared memory segments shall be devided into slots]()
11. [The number of slots shall be configurable]()
12. [Calculate necessary Shared-Memory size prior to creating it]()
13. [Shared Memory segments used by mw::com shall start with prefix `lola`]()
14. [A service shall marked as found, if its underlying file exists]()
15. [Files that are used for Shared Memory IPC shall not be persistent]()
16. [Shared Memory segments shall be identfied via a common known name]()
17. [There shall be one triplet of shared memory segments per service instance]()
18. [Shared Memory segments shall not be mapped to a fixed virtual address]()
19. [Only offset pointer within Shared Memory segments]()
20. [The operating system shall support ACL for shared memory segments]()
21. [Only configured UIDs shall have access to the LoLa shared memory segments]()
22. [Perform Active Bound Checking on dereferenciation of offset pointer]()
23. [Sychronizing Shared Memory regions shall ensure wait-freedom]()
24. [Subscription handling shall be implemented via Message Passing]()
25. [Event Notifications shall be implemented via Message Passing]()
26. [One message passing receive port per ASIL-Level pro process]()
27. [Each message passing port should use a custom thread]()
28. [It shall be possible for all exchange message to register a callback]()
29. [Registered callbacks for messages shall be invoked of the respective waiting thread]()
30. [A control slot shall contain all necessary data information for synchronizing data access]()
31. [A producer shall always be able to store new data]()
32. [On contract violation, QM communication for the affected service instance shall be withdrawn]()

Additionally the requirements derived from the adaptive AUTOSAR requirements shall be considered, as decribed [here](#features-from-adaptive-autosar).

## Software Architecture

The high-level software architecture can be found here: <:27112/collaborator/document/4ff2a028-8ac7-4f7a-b42e-dd90816fec81?viewId=718e62d1-c6e4-4751-9458-40ec38ac39a2&viewType=model&sectionId=856b1ace-a576-49fb-ac5e-b030656afa6f>

The detailed design can be found here: </swh/ddad_platform/tree/master/aas/mw/com/design>

## Additional Considerations

### Security

The operation of shared memory is always a security concern, since it makes it easier for an attacker
to access the memory space of another process.

This is especially true, if two processes have read / write access to the same pages.
We are confident that our applied mechanisms, like reduced access to shared memory segements
and active bounds checking prevent any further attack vectors.

The only scenario that is not covered is an attack against the control segments.
An attacker could in the worst case null all usage counter. In that scenario a race-condition
could happen, that data that is read, is written at the same time, causing incomplete data reads.

This is a drawback that comes with the benefit of less overhead for read/write synchronization,
reducing our latency a lot. At this point in time we accept this drawback by the benefit of the performance.

### Safety

Software Component Failure Analysis: <>
Functional Failure Analysis: <>

#### AoUs towards Applications using LoLa

LoLa represents a substantial infrastructure part of our safety goals. Thus, additional AoUs towards the application side will result from LoLa.

An overview can be found here: <>

### Performance

This is a performance measure. Thus, we expect a reduction in latency in communication
and a reduced memory footprint, since there is no longer the need for memory copies.

### Diagnostics

There are no implications on diagnostics, since there will be no diagnostic job used.

### Testing

There will be substantial need for testing. A concrete test plan is only possible after the FMEA.
At this point we are sure that all tests can be executed utilizing ITF Tests.
This is necessary, since the tests heavily rely on the operating system.
All other tests will be possible to be conducted as unit tests.

### Dynamic Invocation Interface

For some use cases a loosely typed
interface to services, which can be created dynamically during runtime without the need of compile-time dependencies,
would be favorable. For this the [DII concept](./ara_com_dii/README.md) has been created, which LoLa will implement for
event communication in IPNext.
