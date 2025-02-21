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



# IPC Tracing Design

## Introduction

`mw::com`/`LoLa` needs to support `IPC Tracing` by usage of the so-called `Generic Trace API` lib, which is documented
[here](/swh/ddad_platform/tree/master/aas/analysis/tracing/library/design).

This `IPC Tracing` allows to trace the majority of calls a user of `mw::com`/`LoLa` does to the public API of `mw::com`.
The exact supported list of API calls is described [here](/swh/ddad_platform/tree/master/aas/analysis/tracing/library/design#providerskeleton-side-trace-points).
The tracing is controlled by a
[json based configuration](/swh/ddad_platform/blob/master/aas/analysis/tracing/config/schema/comtrace_config_schema.json), 
which describes, which API entry point shall be traced.
Additionally, the underlying element (event/field) the API call relates to has to also be [enabled for tracing](#service-element-specific-properties).

## Configurability

Since tracing per API call is a costly operation, `IPC Tracing` is turned off by default and needs to be turned on
explicitly within `mw::com` configuration. Beside this turn on/off switch, there are further "global" IPC Tracing
related switches, but also switches on the more detailed level of events and fields.

### Global config properties

Our existing [mw_com_config.json](../../impl/configuration/ara_com_config_schema.json) gets extended with the following
property:

```javascript
{
    "tracing": {
      "type": "object",
      "description": "This section contains general settings for IPC Tracing functionality",
      "required": [
        "applicationInstanceID"
      ],
      "properties": {
        "enable": {
          "type": "boolean",
          "description": "enables (or disables) IPC Tracing for the current mw::com application/process.",
          "default": true
        },
        "applicationInstanceID": {
          "type": "string",
          "description": "identification with which this mw::com application shall register itself with the IPC Tracing subsystem. Hint: Typically re-using the DLT AppID could make sense."
        },
        "traceFilterConfigPath": {
          "type": "string",
          "description": "Path, where the trace filter config json is located (adhering to the 'comtrace_config.json' schema)",
          "default": "./etc/mw_com_trace_filter.json"
        }
      }
    }
}
```

These config properties are then stored in class `bmw::mw::com::impl::TracingConfiguration` and will be stored within
the `Configuration`, which is shown [here](../configuration/README.md#c-representation-of-configuration-and-mappings)

### Service element specific properties

Beside the [global](#global-config-properties) config properties, the support for `IPC Tracing` needs to be enabled per
each event/field instance. Default is `false` -> "tracing disabled". Therefore, each event/field config gets the
following extension:

```javascript
"enableIpcTracing": {
   "type": "boolean",
   "description": "Optional flag, which describes, whether this event/field shall be enabled for IPCTracing. Default is false. If it is disabled and a trace-filter-config demands this field being traced, a WARN message will be logged."
   "default": false
}
```

The reason is, that trace support costs memory resources per each event/field instance to be supported! So we require it
to be explicitly switched on, if needed.

This additional config property is then also stored in class `impl::TracingConfiguration` (see above) via calls to
`SetServiceElementTracingEnabled(service_element_identifier, instance_specifier)`, so that later events/fields can set
up efficient data-structures (see [below](#enriching-mwcom-classes-with-trace-switches)) for their decision, whether
specific API calls of them need to be traced.

## Trace Filter Config

The `trace filter config` is represented as a json format according to the schema [here](/swh/ddad_platform/blob/master/.github/json-schemas/comtrace_config.json).
It defines for specific `trace points`, whether they shall be enabled or not. A trace point is a public API of
`mw::com`/`LoLa`. It is identified by:
- the service type
- the element within the service (which event/field/method)
- the provided API of this element ([see here](/swh/ddad_platform/tree/master/aas/analysis/tracing/library/design#providerskeleton-side-trace-points))
  for an overview of the supported APIs.

Note: Currently the trace filter config does **NOT** distinguish between service instances! I.e. if a certain
`trace point` is `enabled`, it is enabled for any instance. In our design we already foresee an instance identification,
which will be currently not used.

### Parsing

The parsing of the `trace filter config` is carried out during `bmw::mw::com::impl::Runtime::Initialize()`. I.e. after
the configuration has been parsed and transformed into an instance of `bmw::mw::com::impl::Configuration`, it is
checked via `Configuration::GetTracingConfiguration()`, whether `TracingConfiguration::IsTracingEnabled()` == `true`.
Only if this is the case the `trace filter config` in the path given by
`impl::TracingConfiguration::GetTracingFilterConfigPath()` is parsed and transformed into an instance of
`tracing::TracingFilterConfig`, which inherits from `tracing::ITracingFilterConfig` to support testing/mocking and has
the following structure:

<img src="/swh/ddad_platform/aas/mw/com/design/ipc_tracing/structural_view.uxf?ref=mf_mw_com_ipc_tracing_design" />

For each activated/enabled trace point in the `trace filter config` a corresponding
`tracing::TracingFilterConfig::AddTracePoint` is called, but **only** if for the enclosing service element (field/event)
tracing has been enabled at all in the `mw::com::Configuration` for the related service instance deployments
(see [here](#service-element-specific-properties)).
I.e. `tracing::TracingFilterConfig::AddTracePoint` gets only called, if
`impl::TracingConfiguration::IsServiceElementTracingEnabled()` returns `true`.

#### Avoid registration of non-existing trace points

The `trace filter config` could contain trace point definitions, which relate to service types, which aren't used within
the `mw::com`/`LoLa` application. I.e. the `trace filter config` contains a trace point for a service with a specific 
`shortname_path`, but the `mw_com_config.json` configuration of the application doesn't contain an element in array
`serviceTypes`, whose property `serviceTypeName` matches the `shortname_path`! In this case the trace point definition
will not be added to the `TracingFilterConfig` via  call to `AddTracePoint`.

#### Managing config strings

The `trace filter config` contains many strings (service type or service element names), which have to be stored within
`TracingFilterConfig`. These strings are initially owned by the json object, that the json parser returns. Since
these `trace filter config` can be very big, the json object will be disposed at the end to free resources. So any
string, which we need within `TracingFilterConfig` needs to be copied from the provided json objects into the
`TracingFilterConfig` instance. As many of those strings are very frequently used, we create a `std::string` copy of
the json-provided string once and then reference this string contained by the `std::string` with `amp::string_view` from
several locations.

**Example**:
In the `trace filter config` we will have several trace points for a service type S1 identified by a short-name-path
"&lt;S1 service type short name path&gt;". So we will call several times `Add<Proxy|Skeleton><Event|Field>TracePoint`
with the same 1st `amp::string_view parameter` "&lt;S1 service type short name path&gt;". In each such call the implementation
will check, whether the internal member `TracingFilterConfig::config_names_` already contains a `std::string` matching
"&lt;S1 service type short name path&gt;". If not, a std::string for "&lt;S1 service type short name path&gt;" gets
created and inserted into `TracingFilterConfig::config_names_`. Then the corresponding trace point is inserted into
one of the related maps (`skeleton_event_trace_points`, `skeleton_field_trace_points`, `proxy_event_trace_points` or
`proxy_field_trace_points`). But the key created for the insertion will use an `amp::string_view` referencing the
`std::string`, which was inserted into `TracingFilterConfig::config_names_`.

### Enriching mw::com classes with trace switches

To maximize efficiency of the decision, whether a certain public API call of `mw::com` shall be traced, this information
is cached directly in the classes, which make up the public API with trace support. I.e. we want to avoid, that in
context of a public API call a costly search/lookup needs to be done, to decide, whether this call needs to be traced or
not.
Therefore, existing binding independent classes for events/fields at proxy and skeleton side get extended with
`tracing_data_` members, which contain simple enable/disable flags per API:

<img src="/swh/ddad_platform/aas/mw/com/design/ipc_tracing/structural_event_field_extensions.uxf?ref=mf_mw_com_ipc_tracing_design" />

Due to our current architecture, we can't hand those `<Skeleton|Proxy>EventTracingData` instances over in the ctor
calls of the binding independent classes for events/fields at proxy and skeleton side, since at least on the skeleton
side binding specific information is required for setting `SkeletonEventTracingData::trace_context_id`.
So instead these classes create those `tracing_data_` members within their ctor in the following way:

<img src="/swh/ddad_platform/aas/mw/com/design/ipc_tracing/sequence_API_trace_setup.uxf?ref=mf_mw_com_ipc_tracing_design" />

As mentioned above the `SkeletonEventTracingData` has (opposed to `ProxyEventTracingData`) an additional member
`trace_context_id`, which has to be set up, because only the skeleton side tracing deals with the asynchronous trace
calls referring to data located in shared memory. How this member gets set and is used is detailed
[here](#setting-up-sampleptr-array).

_Note_: The reason, why the creation of the proxy/skeleton event/field related `tracing_data`_ members aren't set up in
the non-templated base-classes `impl::ProxyEventBase` resp. `impl::SkeletonEventBase`, but in the subclass
`impl::ProxyEvent`/`impl::SkeletonEvent` class template `ctors` is the following:
The `impl::ProxyEvent`/`impl::SkeletonEvent` ctor will be also called by the `impl::ProxyField`/`impl::SkeletonField`,
because our fields are a composition, which dispatches event-functionality to its corresponding event
(`impl::ProxyEvent` or `impl:SkeletonEvent`). Therefore, the `impl::ProxyField`/`impl::SkeletonField` class calls
a **specific** ctor of `impl::ProxyEvent`/`impl::SkeletonEvent`, in which we have the context knowledge, to set up the
`tracing_data_` members correctly.
With this in place the sequence above shown for a `impl::ProxyEvent`, is almost identically executed for a proxy
field, a skeleton event and a skeleton field. The only small adaptions are, that the corresponding helper functions
(instead of `GenerateProxyTracingStructFromEventConfig`) calling the adequate overloads of
`TracingFilterConfig::isTracePointEnabled()` are:
  - `GenerateProxyTracingStructFromFieldConfig`
  - `GenerateSkeletonTracingStructFromEventConfig`
  - `GenerateSkeletonTracingStructFromFieldConfig`

## Interaction with the Generic Trace API library

Calls to `analysis::tracing::GenericTraceAPI` are not done directly within the `mw::com` codebase, but are
wrapped in a "facade", which is `impl::tracing::TracingRuntime`.
Besides the core Trace() calls the `analysis::tracing::GenericTraceAPI` provides APIs to register the client/user of
library and do some setup stuff.
Furthermore `impl::tracing::TracingRuntime` eventually dispatches to a binding specific tracing runtime binding
(implementing `ITracingRuntimeBinding`), where we currently obviously only have `lola::tracing::TracingRuntime`. This
solves again the problem, that different bindings might need to use/call the APIs of `analysis::tracing::GenericTraceAPI`
slightly different. So in case there is no need to differentiate for an API call, it is directly done by/dispatched from
`impl::tracing::TracingRuntime` to `analysis::tracing::GenericTraceAPI`, otherwise `impl::tracing::TracingRuntime`
dispatches to the correct binding specific implementation of `ITracingRuntimeBinding`, which then calls
`analysis::tracing::GenericTraceAPI`!

An additional feature of the "facade" `impl::tracing::TracingRuntime` is, that it manages cross-cutting concerns over
multiple calls to the `GenericTraceAPI`. E.g. it monitors errors, when calling to `GenericTraceAPI` and decides, when
to turn off tracing completely because of unrecoverable errors or a threshold of consecutive errors being reached.

### Handling Trace calls

At some point within an `LoLa`/`mw::com` public API call a call to `analysis::tracing::GenericTraceAPI::Trace()`
(wrapped by our "facade" `impl::tracing::TracingRuntime`) has to be done, in case this API trace point is enabled
according to bool switches in `tracing::SkeletonEventTracingData` and `tracing::ProxyEventTracingData`, which we
discussed in the [previous chapter](#enriching-mwcom-classes-with-trace-switches).

In our design, we favored to provide as much functionality as possible in regard to tracing at the
**binding independent level**. Currently, as we only have our LoLa/shared-memory binding, this is a bit more effort,
but we are prepared for any future binding and then would spare us code duplication, in case we would implement tracing
completely at the lower binding specific layer.

The reason, that the API call tracing can't be completely done on the **binding independent level** is due to
efficiency/performance reasons designed into the API provided by the `Generic Trace API` library. The core performance
characteristic of this library is, that it can avoid costly data-copying of trace data, if the data already resides in
shared-memory. Because in this case, it directly accesses the data at its source-location and makes it available to
network hardware by mapping this shared memory also in the network hardware address space.

Therefore, the general idea is, that API calls, which don't deal directly with data (event/field samples) or binding
specific meta-information of the data, are implemented solely on the **binding independent level**.
Some examples for this:

- `ProxyEventBase::Subscribe()`
- `ProxyEventBase::Unsubscribe()`
- `ProxyEventBase::SetReceiveHandler()`
- `ProxyEventBase::UnsetReceiveHandler()`

in the cases, where we need binding specific implementation for tracing, we try to keep our binding level implementation
as independent as possible from any `Generic Trace API` specific API types! We realize this, by a pattern, where we
create the call to the `Generic Trace API` with its specific types on the **binding independent level** as an optional
callable (optional, because the callable only gets created, when the trace-point is currently enabled),
which gets all the inputs it needs from the binding specific implementation as call arguments, whose types are
abstracted from the `Generic Trace API`. This optional callable is then handed over from the
**binding independent level** to the binding level implementation, where it then gets called (if the optional is set),
with the binding specific information/arguments.

The following sequence shows as an example the call to `SkeletonEvent<SampleDataType>::Send`

<img src="/swh/ddad_platform/aas/mw/com/design/ipc_tracing/sequence_layer_interaction_sample.uxf?ref=mf_mw_com_ipc_tracing_design" />

Like explained above &ndash; the `LoLa` binding specific `lola::SkeletonEvent` doesn't know anything, which is really
tracing specific! It gets handed over an optional "tracing callback", which has been prepared by the binding independent
`SkeletonEvent`, which expects to be called in this case with a `SampleAllocateePtr`. The binding specific `SkeletonEvent`
simply checks, whether there is an optional "tracing callback" and if so calls it with its current `SampleAllocateePtr`.

### Managing TraceDone callbacks

Tracing calls are **asynchronous** by nature. At least in the case of the "data-heavy" `Trace()` API overload, which
deals with data, which already resides in shared-memory:

```c++
static TraceResult Trace(const TraceClientId client, const MetaInfoVariants::type& meta_info,
                             ShmDataChunkList& data, TraceContextId context_id);
```

The data to be traced is handed over in the form of a `ShmDataChunkList` in this overload. I.e. a list of shared memory
locations (given by an identification of the shared-memory object, an offset into the shared-memory object and a size).
The asynchronous nature is reflected by the last argument `context_id`. This is an identification generated by the
caller (in our case `mw::com`/`LoLa`), which identifies the data in shared memory given to the `Trace()` call.
As soon as the tracing sub-system has processed the data of the `Trace()` call, it reports this back via a call to a
`TraceDoneCallback` with the same `context_id` value as argument.

A `TraceDoneCallback` is registered for each binding (so we currently only have one for `LoLa`). Each `TraceDoneCallback`
will be registered within the binding dependent `lola::TracingRuntime::RegisterWithGenericTraceApi()` during the
construction of the binding independent `impl::TracingRuntime`. This register function calls
`bmw::analysis::tracing::GenericTraceAPI::RegisterTraceDoneCB(const TraceClientId, TraceDoneCallBackType)`.

This means, that `LoLa` has to keep the shared-memory data (this is `LoLa` event/field sample data) stable up to the
point, where the `TraceDoneCallback` with the related `context_id` gets invoked. Only then the shared-memory referenced
by the `data`/`ShmDataChunkList` can be reclaimed again.

#### Tracing Subsystem as an event/field consumer

To realize this shared-memory usage/reclaim pattern described above, the `IPC Tracing` implementation in `LoLa` falls
back on existing functionality! The `IPC Tracing` subsystem is just another consumer/`proxy`, which subscribes to
events/fields and accesses event/field samples. I.e. we use exactly the same ref-counting logic within our
`lola::EventDataControl`, which our `ProxyEvents`/`ProxyFields` use. Accordingly, when an event/field sample shall
be traced via the `Trace()` API overload for `ShmDataChunkList`, first a `SamplePtr` gets created, which manages the
lifetime (i.e. usage/reclaim pattern) of the underlying sample data in shared-memory. So the steps, when an event-send/
field-update call shall be traced are:

- after skeleton side created a new sample within an allocated slot in shared-memory it instantly increments the
  ref-count and creates a `SamplePtr` referencing the related slot.
- it stores the `SamplePtr` in an array and hands over the array-index, where the `SamplePtr` was stored as `context_id`
  in the call to `Trace()`.
- When the registered `TraceDoneCallback` gets invoked with the `context_id` (aka array index of the `SamplePtr` array)
  the callback implementation deletes the related `SamplePtr` from the array again, whereby decrementing the ref-count
  of the underlying field/event-slot again.

The semantic difference to the normal use of `SamplePtrs` is obviously, that for `IPC Tracing` the `SamplePtrs` are
created at the `Skeleton` side and **not** the `Proxy` side. This is logical as for `IPC Tracing` the decision is to
trace data at the point, where it gets created. In the case of event/field samples, this is the `Skeleton` side.
In `IPC Tracing` data gets always traced together with an `DataId` (ID identifying the data). At the reception side
(in case of an event-send/field-update, this is the `ProxyEvent::GetNewSamples()`/sample-callback API)
only the `DatId` gets traced, which allows to correlate the data value traced previously at the sender/creator side.
This is efficient as we often do have 1:N communication (one event/field provider with N consumers): Potentially large
event/field data is only traced **once** and potentially N receivers are just relating to this data via an ID in their
trace output.

#### DataId realization

In `mw::com`/`LoLa` the `DataId` (mentioned above) to be used to identify field/event samples/data is simply the
timestamp `lola::EventSlotStatus::EventTimeStamp`, which is stored for each sample in the control-slot-array. Since in
the calls to `Trace` [see signature here](#managing-tracedone-callbacks), there is also the `meta_info`, which contains
service-type, instance-id and element-id, the combination with the timestamp as our `DataId` is sufficient to uniquely
identify traced event/field data within a trace-output.

#### Setting up SamplePtr array

`TraceDoneCallback` can be invoked concurrently by the `IPC Tracing` subsystem / `analysis::tracing::GenericTraceAPI`.
At least there aren't any restrictions and since the calls to `Trace()` are potentially done concurrently, we shall be
prepared for concurrent calls to the `TraceDoneCallback`, we have registered.
In our design, we assured, that no (costly) synchronization has to be done, when a `TraceDoneCallback` gets called, and
we have to deduce the resources to be freed from the given `context_id` argument. This is essentially solved with the
`context_id` of type `TraceContextId` being an index in our `SamplePtr` array. Since it is a fixed size array (allocated
with a fixed size at initialization time) and each array-element is only used by one explicit service-element, there is
no "synchronized management"/locking needed during runtime.

The `SamplePtr` array is realized in `lola::tracing::TracingRuntime::type_erased_sample_ptrs_`. Its size gets decided
after the entire tracing related configurations have been evaluated and it is exactly known (see [here](#parsing)), for
which service elements tracing is enabled at all. When the `lola::tracing::TracingRuntime` gets constructed, it gets the
information in the constructor call via `number_of_service_elements_with_trace_done_callback`, how many instances of
service elements, with tracing enabled and a potential call to the asynchronous `Trace()` overload
[see signature here](#managing-tracedone-callbacks) exist. This gets determined by the call to
`TracingFilterConfig::GetNumberOfServiceElementsWithTraceDoneCB()`.

Later (after `lola::tracing::TracingRuntime` construction) during initialization, where the service element specific
trace switches are [set up](#enriching-mwcom-classes-with-trace-switches) also a specific "registration" of service
elements with tracing enabled and potential `TraceDoneCallback` need to take place: They register themselves at the
`lola::tracing::TracingRuntime` via call to`RegisterServiceElement()`, which returns their specific `TraceContextId` to
be used for calls to the async `Trace()` overload.
The returned `TraceContextId` is simply the index of the first available slot in
`lola::tracing::TracingRuntime::type_erased_sample_ptrs_`, which gets incremented for each `RegisterServiceElement()`
call.

In case the pre-calculated number of needed `TraceContextId`s (i.e. the array size of
`lola::tracing::TracingRuntime::type_erased_sample_ptrs_`) returned by
`TracingFilterConfig::GetNumberOfServiceElementsWithTraceDoneCB()` was wrong/too small, it will be detected in the
course of the `RegisterServiceElement()` calls.

#### Current Restrictions with regard to buffer space

As of now we only foresee one `SamplePtr` (one sample slot occupation) per service element instance using the
asynchronous `Trace()` overload. The consequence is: If currently such an asynchronous `Trace()` call done by a `LoLa`
service element instance is active, i.e. the corresponding `TraceDoneCallback` with the same `context_id` has not yet
been called, the `SamplePtr` is blocked. If then a 2nd user level API call happens, which would trigger the same
asynchronous `Trace()` call again, the current implementation detects, that the same `context_id`/`SamplePtr` is in use:

`impl::tracing::TracingRuntime::Trace()` calls
`lola::tracing::TracingRuntime::IsServiceElementTracingActive(trace_context_id)`, which
checks, whether `lola::tracing::TracingRuntime::type_erased_sample_ptrs_[trace_context_id]` already contains a valid
`SamplePtr`.

If we need to support a **higher number** of parallel `Trace()` calls per service element instance, because the provider
is faster in producing new events/fields than the tracing subsystem to consume it, we would need to loosen the one
SamplePtr restriction per service element instance for tracing. This could be done and has the following consequences:

- Adding a **new** configuration parameter in `mw_com_config.json`, to allow to specify the number of slots/`SamplePtrs`
  of a service element instance for tracing. Right now we only have the property `enableIpcTracing` on this level
  (see [here](#service-element-specific-properties)). If this property set to `true`, it sets the value for tracing
  slots/`SamplePtrs`implicitly to 1 and also cares for internally adding 1 to the configured `numberOfSampleSlots`.
  With a new property, where the user can adapt this number to something bigger than 1, also the implicit addition to
  `numberOfSampleSlots` has to be adapted (which is used to calculate the size of the `SamplePtrArray` ->
  `lola::tracing::TracingRuntime::type_erased_sample_ptrs_`).
- The calculation of `TracingFilterConfig::GetNumberOfServiceElementsWithTraceDoneCB()` would then also need to be
  adapted.
- A service element instance (if it has been configured with a bigger number than 1) has to call
  `lola::tracing::TracingRuntime::RegisterServiceElement()` N times and store then multiple returned
  `trace_context_ids`. And then in the context of the `Trace()` call, it has to iterate through the stored
  `trace_context_ids` in a loop/ring-buffer semantics and always use the next one.





