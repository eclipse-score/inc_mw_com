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



# Dynamic Proxies for `ara::com`

## Motivation

The communication pattern in the `ara::com` middleware is based on compile-time, strongly typed interfaces. I.e. from a
service interface definition (which in pure AUTOSAR `ara::com` is described in an `ARXML` format) the following C++
types get generated pre-compile time of an application:

- Data types, which get exchanged via event, field, service-method communication between the service provider side
  (skeleton) and consumer side (proxy)
- Skeleton classes (which depend on/use aforementioned data types)
- Proxy classes (which depend on/use aforementioned data types)

This approach has the following consequences:

For an `ara::com` application, which wants to communicate with a service provider based on a service interface "new"
to the application, the application has to be **enhanced with new generated data types**, proxy class types and then
**recompiled**. Note, that since a signature change is involved here even any approaches of `dynamic loading` the
updated interfaces is no solution.

In almost all cases this is no problem or downside: The strong type-safety between skeleton and proxy is beneficial and
the need of a proxy side application to communicate with a new type of service provider (skeleton) comes typically with
complete new application/business logic, so enhancing/recompiling the proxy side application is straight-forward and
expected.

But there are some "niche" use cases, where the need to regenerate and recompile the proxy side can be detrimental in
cases, where:

1. signatures are trivial and changes/differences between them are minimal
2. the communicated data/payload gets handled very generically (loosely typed) anyhow
3. the communicated data/payload has to get deep-inspected based on additional/separate type-information anyhow

Two "real world" examples, which fall into those categories:

- AUTOSAR diagnostic stack (`diagnostic manager`) as the `UDS` protocol deduced signatures/data-types are simple and
  uniform, yet you have a plethora of "different" (although quite similar) diagnostic services (`DID` or `Routine`
  based) provided by applications. Regeneration/recompile of the `diagnostic manager` everytime a new `DID`/`Routine`
  gets deployed on application level is bloated/clumsy. Besides that the `diagnostic manager` is simply forwarding the
  "payload" without interpretation anyhow, so any strong typing is useless. So for `diagnostic manager` (1) and (2)
  from above do apply.
- BMW `CDC` (Crowd Data Collector): An application, which gets deployed on an ECU with the "capability" to consume any
  data/service provided on the very same ECU to apply dynamically operations on it and send info about this data to the
  backend. This application gets dynamically updated configuration (from the backend outside the car), which determine,
  what to consume and how to interpret/dissect and transform the data. So basically the data layout and semantics as
  well as the algorithms processed on that data are contained in this dynamically updated configuration. Here (3) from
  above does apply.

In the concrete case of `CDC` there is the typical scenario, where the `CDC` gets connected to "data/service providers"
already at its build time (time of the build for the whole ECU image). Thereby CDC is "connected" via a generated
(strongly typed) ara::com proxies used to communicate with the "data/service providers".
Note, that at its build time the CDC just gets strongly typed proxies of a subset of "data/service providers" residing
on the ECU! Only for "data/service providers", where there bare already known use cases for consuming their data via
`CDC`.

Then later BMW may decide to do an in field update of a running `CDC` instance. I.e. just updating its configuration,
instructing `CDC` to also consume/process further "data/service providers", for which `CDC` hasn't any built in
(strongly typed) proxy classes. So the configuration update only contains new deployment info, stating, that `CDC` shall
also now connect to service type "xyz" with some instance id 42 and consume "EventA" and "EventB" from it.
Additionally, this configuration update may contain some description what bits/bytes to extract from the binary
representation of a received "EventA" and "EventB" instance. This description can be created in the backend, because
there it is known what the C++ data types of "EventA" and "EventB" are and how their in memory representation looks
like, because architecture and compiler settings of the ECU are known.

## Similar solutions in other middleware technologies

### CORBA

The idea to construct calls to services on-the-fly during runtime dynamically isn't new. Already the "grandfather" of
all middleware technologies `CORBA` did provide such an approach via `DII` (Dynamic Invocation Interface) although via
a slightly different approach (`DII` allows async, so-called `Deferred Synchronous Requests`, which the normal strongly
typed stubs/proxies don't allow)

### JAVA RMI Dynamic Proxies et al

Programming languages with extremely deep introspection (RTTI) like e.g. Java and dynamic class loading obviously are
"easy" environments for completely dynamic proxies, which can be setup during runtime and don't need to be generated
at compile time.

## High level solution proposal for ara::com

Instead of a concrete/specific proxy class generated from the service-interface definition (arxml), we introduce a
`ara::com::GenericProxy` class.
This class provides exactly the same "FindService" API signatures/mechanisms (`FindService`, `StartFindService`,
`StopFindService`) like the existing generated proxy classes.
The members for events, fields and service-methods, which generated proxies do provide according to the
service-interface definition, on which they are based on, will become **replaced by** three getter methods returning
maps:

- public member function `const EventMap& GetEvents() const noexcept` which returns a `const EventMap&`, which
  represents a read-only map, where the key is a string (containing the
  event short name) and the value is a `GenericProxyEvent`: It shall provide at least the following interface
 (taken from `ara::core::Map`):
- 
  - `cbegin()`
  - `cend()`
  - `find()`
  - `size()`
  - `empty()`

- public member function `const FieldMap& GetFields() const noexcept` which returns a `const FieldMap&`, which
  represents a read-only map, where the key is a string (containing the
  field short name) and the value is a `GenericProxyField`: It shall provide at least the following interface like the
  `EventMap` above.
- public member function `const MethodMap& GetMethods() const noexcept` which returns a `const MethodMap&`, which
  represents a read-only map, where the key is a string (containing the
  method short name) and the value is a `GenericProxyMethod`: It shall provide at least the following interface like the
  `EventMap` above.

With this change, resp. new proxy type an application not being aware of the concrete service-type at its build time,
can work like this:

- it gets a new deployment info for the new/unknown service-type and instance (e.g. sent by the backend), which it makes
  accessible to its `ara::com` runtime for the next start/restart.
- this deployment info also contains the `InstanceSpecifier` with which the application will later call
  `ara::com::GenericProxy::FindService()`
- `ara::com::GenericProxy::FindService(InstanceSpecifier)` works as expected: It does a lookup into the deployment to
  resolve the `InstanceSpecifier` to the concrete service instance, it is mapped to and &ndash; if the service instance
  exists &ndash; returns a ServiceHandle.
- Now from this `ServiceHandle` a `GenericProxy` gets created.
- This `GenericProxy` can now "asked" via the different maps, which events, fields, methods it does contain.
- Each of those events, fields, methods can then be interacted with in an **untyped form** (aka blob/byte-array).

## Binary representation of untyped form

So how does the untyped form look like? This is &ndash; **unfortunately** &ndash; implementation specific! The reason is
the following:
Even if the **C++ data type** representing an event or field data type or a method result/call argument is exactly defined
for the event/field/method the `GenericProxy` interacts with, depending on the underlying technology/implementation, it
might not be possible for a `GenericProxy` (without type specific compile-time information) to represent the data
received from the provider/skeleton side as this C++ data type binary representation! This is always the case, when the
data is serialized/transformed by the provider/skeleton side before passing it to the consumer/proxy **and** the proxy
side needs compile-time type info for deserialization!
In our Dynamic Proxy approach we do not have this compile-time type info! This will lead to the following possible
outcomes:

- the received data (event data, field data, method result/output data) will be represented as the expected binary C++
  type representation (In these cases the user of `GenericProxy` "only" needs to know compiler settings (alignments,
  sizes, offsets) to interpret the data correctly):
  -- in case the implementation doesn't do any serialization at all (e.g. zero-copy implementations,
  implementations which just copy raw representations without serialization).
  -- in case the implementation applies a serialization, but encodes enough "meta-info" into the serialized form, that the
  C++ type binary representation can be rebuilt at the receiver side without compile-time type info.
- the received data will be represented at the GenericProxy side in serialized format. In this case the user of
  `GenericProxy` needs to be able to understand/interpret the implementation specific serialized form.

## Detailed solution proposal for ara::com

### Map for Events

The data type (see [here](#high-level-solution-proposal-for-aracom)) for the event map member of an
`ara::com::GenericProxy` shall be:

`const ara::com::EventMap` which could be defined as:
`using ara::com::EventMap ara::core::Map<ara::core::StringView, ara::com:GenericProxyEventara::com:GenericProxyEvent>`.
At least the following interfaces (taken from `ara::core::Map` resp. `std::map`) shall be exposed:

- `cbegin()`
- `cend()`
- `find()`
- `size()`
- `empty()`

where `ara::com:GenericProxyEvent` is very similar to strongly typed/generated events as specified in  and
chapter 8.1.3 API Reference in the AUTOSAR SWS Communication Management.
These are the deviations:

1. The underlying event data type is `void` (as its exact type is unknown)
2. Therefore `GenericProxyEvent::GetNewSamples()` will call the callback F with an `ara::com::SamplePtr<void>`.
3. For this obviously a void specialization of `ara::com::SamplePtr` is needed. This specialization removes the
   following members:
    - `T& operator*() const noexcept`
4. `GenericProxyEvent` gets a new member func: `std::size_t GetSampleSize()`, which returns the "static" size of the sample
   type in bytes.
5. `GenericProxyEvent` gets a new member func: `bool HasSerializedFormat()`, which returns `true` in case the
   representation of the event provided by the implementation is some serialized format or `false` in case the
   representation is that of the expected C++ type. (see [here](#binary representation-of-untyped-form))

### Map for Fields

The data type (see [here](#high-level-solution-proposal-for-aracom)) for the field map member of an
`ara::com::GenericProxy` shall be structurally the same as the [EventMap](#map-for-events). Only difference is, that the
type is an `ara::com:GenericProxyField` instead of `ara::com:GenericProxyEvent`

where `ara::com:GenericProxyField` is very similar to strongly typed/generated fields as specified in  and
chapter 8.1.3 API Reference in the AUTOSAR SWS Communication Management.
These are the deviations:

1. The underlying field data type is `void` (as its exact type is unknown)
2. Therefore `GenericProxyField::GetNewSamples()` will call the callback F with an `ara::com::SamplePtr<void>`.
3. For this obviously a void specialization of `ara::com::SamplePtr` is needed. See above with events!
4. `GenericProxyField` gets a new member func: `std::size_t GetSampleSize()`, which returns the "static" size of the field
   type in bytes.
5. `GenericProxyField` gets a new member func: `bool HasSerializedFormat()`, which returns `true` in case the
   representation of the field provided by the implementation is some serialized format or `false` in case the
   representation is that of the expected C++ type. (see [here](#binary representation-of-untyped-form))
5. The Get method `GenericProxyField:Get` signature changes to:
   `ara::core::Future<GenericFieldPayload> Get()`
6. `ara::core::Future<GenericFieldPayload> Set(const void* value)`
7. `GenericFieldPayload` class is a wrapper around the **untyped field form** (aka blob/byte-array), where the
   implementation can implement/hide its memory allocation/de-allocation. GenericFieldPayload has two public methods:
    - `const void* getData()`
    - `std::size_t getSize()`

### Map for Methods

**TBD**.
**Note**: Methods are a different animal from complexity perspective! Dealing in a fully dynamic/generic way with
previously unknown methods adds additional challenges and is postponed right now until hard requirements come up.
