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



# Configuration

The main configuration items we have to deal with in our `ara::com` implementation are mappings from "logical" 
service instances to real existing service instances with their concrete used technical binding.
Currently, we prepare our configuration to support the following technical bindings:
- SOME/IP
- SharedMemory IPC

Of course, we do foresee arbitrary extensions.

## Service Type identification

AUTOSAR specification demands (), that for every Service interface a corresponding class gets generated,
which contains members of the following types:
- ServiceIdentifierType
- ServiceVersionType

The exact type definitions are left to the implementer, but there are some strong hints to reflect the unique AUTOSAR
meta-model names.
So we decided to use the so called short-name-path of a service interface in the ARXML model to identify a service type.
So our implementation of `bmw::mw::com::ServiceIdentifierType` contains the fully-qualified service interface name in the form
of its short-name-path as its identifying name member.

_Important_: `bmw::mw::com::ServiceIdentifierType` is a fully binding independant identification of a service type.
The technical bindings might use complete different data types for identification of a specific service type. 
Service versioning is still a very immature topic in AUTOSAR and especially in the SWS Communication Management. So
right now our implementation of `bmw::mw::com::ServiceVersionType` fulfills the minimal requirements by providing a major
and minor part each typed as `std::uint32_t` to fulfill minial model compatibility.

## ServiceTypeDeployment vs ServiceInstanceDeployment
We have to clearly separate those two notions within our configuration.

### ServiceTypeDeployment
The `ServiceTypeDeployment` aka `Service Interface Deployment` in AUTOSAR speech, maps a technology/binding independent
`ServiceType` to a concrete technical implementation.
The binding independent `ServiceType` is defined within AUTOSAR meta-model by its Service Interface and its
corresponding [service type identification](#service-type-identification).  
So within the configuration we might need to express, how a certain abstract `ServiceType` shall be represented in a
SOME/IP and a LoLa/Shm binding. F.i. both bindings might use their own/distinct service id for identification and
also the embedded service parts (events, fields, methods) might have different identification/properties between
a `SOME/IP` and a `LoLa`/Shm binding.

Note here, that these `ServiceTypeDeployment`s are independent of their concrete instances! E.g. an AUTOSAR service
`/a/b/c/InterfaceName` will be once mapped to a `SOME/IP` service id **_SIDn_** and this applies then to ALL instances of
this `/a/b/c/InterfaceName` service with a `SOME/IP` mapping!

### ServiceInstanceDeployment
The `ServiceInstanceDeployment` maps concrete instances (identified by an [Instance Specifier](#instance-specifiers)) of
a `ServiceType`, to a `ServiceTypeDeployment` and extends it with instance specific properties! Among those instance
specific properties are:
- binding specific instance identifier
- asil-level this instance is implemented for
- instance specific tuning like maximum event storage ...

### Deployment time decisions
While AUTOSAR generally foresees, that `ServiceTypeDeployment` aka `Service Interface Deployment` is a
generation/pre-compile step, this is **not** true for our implementation! The amount of code, which is affected at 
generation time is kept minimal and only applies to binding independent parts of our `ara::com` impl. as this is required
by the `ara::com` specification.

Our technical binding implementation is configured during runtime, when both artefacts `ServiceTypeDeployment` and
`ServiceInstanceDeployment` are read from configuration.

### Responsibility for ServiceTypeDeployment/ServiceInstanceDeployment
For `ServiceInstanceDeployment` it is easy/clear: It is the job of the integrator of the ECU, because knowledge about
processes/applications is needed and how to package the `ServiceInstanceDeployment` config artefacts with the applications.

For `ServiceTypeDeployment` it is not so clear: In case of `ServiceTypeDeployments` for local only communication, which is
the case of our `LoLa`/Shm binding, it is also rather the job of the ECU integrator as it only local ECU optimization,
without any effect to the boardnet. In case of `SOME/IP` `ServiceTypeDeployments`, it might be expected for the future,
that parts of `ServiceTypeDeployment` come from central toolchains (Symphony).

## Instance Specifiers

`InstanceSpecifier` is an AUTOSAR AP mechanism to specify some instance of a service type in a binding/deployment
independent way **within the source code**!
If you look at the underlying (ARXML) model of your software component, you express
the provision of a specific service instance or the requirement of a specific service instance with a P-port or R-port
respectively, which is typed by the service interface, from which the
[service type identification](#service-type-identification) is deduced.
Such a port instance also has (like a service interface) a fully qualified name (expressed via a short-name-path),
which exactly reflects the `InstanceSpecifier`.

### Mapping to concrete (technical) instances
As mentioned [above](#responsibility-for-servicetypedeploymentserviceinstancedeployment) It is the task of the
integrator, to map those `InstanceSpecifier`s to concrete technical instances of the services.

We decided to implement this mapping description via a JSON format.
An example of such a mapping is shown here:

```javascript
{
    "serviceTypes": [
        {
            "serviceTypeName": "/bmw/ncar/services/TirePressureService",
            "serviceTypeId": 99999999999,
            "version": {
                "major": 12,
                "minor": 34
            },
            "bindings": [
                {
                    "binding": "SOME/IP",
                    "serviceId": 1234,
                    "events": [
                        {
                            "eventName": "CurrentPressureFrontLeft",
                            "eventId": 633
                        }
                    ],
                    "fields": []
                },
                {
                    "binding": "SHM",
                    "serviceId": 1234,
                    "events": [
                        {
                            "eventName": "CurrentPressureFrontLeft",
                            "eventId": 20
                        }
                    ],
                    "fields": []
                }
            ]
        }
    ],
        "serviceInstances": [
        {
            "instanceSpecifier": "abc/abc/TirePressurePort",
            "serviceTypeName": "/bmw/ncar/services/TirePressureService",
            "instances": [
                {
                    "instanceId": 1234,
                    "asil-level": "QM",
                    "binding": "SOME/IP"
                },
                {
                    "instanceId": 62,
                    "asil-level": "ASIL-B",
                    "binding": "SHM",
                    "events": [
                        {
                            "eventName": "CurrentPressureFrontLeft",
                            "maxSamples": 50,
                            "maxSubscribers": 5
                        }
                    ],
                    "fields": []
                }
            ]
        }
    ]
}
```
As you can see in this example configuration, it provides both: A `ServiceTypeDeployment` (`service_types`) in the upper
part and a `ServiceInstanceDeployment` (`service_instances`) in the lower part.

As you see in this example, we map the `InstanceSpecifier`/port "abc/abc/TirePressurePort" to concrete
service instances.
What is **not** visible here: Whether "abc/abc/TirePressurePort" is a provided or required service instance. Both could
be possible, since we do support 1:n mappings in both cases.
Here we have a mapping of "abc/abc/TirePressurePort" to two different concrete technical instances: The first one is a
SOME/IP based instance (so it is most likely used for inter ECU/network communication) and the second is a concrete
instance based on our shared memory IPC for ECU local communication.

#### C++ representation of configuration and mappings
The JSON representation of the configuration shown above gets read/parsed at application startup within call to one of
the static `bmw::mw::com::impl::Runtime::Initialize()` methods. We do provide various overloads, to either allow
initialization from a default manifest/configuration path, an explicit user provided path or even (for unit testing)
a directly handed over JSON.
The sequence during startup would look like this:

<img src="/swh/ddad_platform/aas/mw/com/design/configuration/sequence_startup_view.uxf" />

During this call a singleton instance of `bmw::mw::com::impl::Runtime` gets created, which gets the parsed/validated
configuration in the form of `bmw::mw::com::detail::Configuration`.
`bmw::mw::com::detail::Configuration` in essence holds two maps:
* one (`serviceTypes`), which holds the `ServiceTypeDeployment`s, where the key is the `ServiceIdentifierType`
* one (`instanceInstances`), which holds the `ServiceInstanceDeployment`s, where the key is the `InstanceSpecifier`. The
  `ServiceInstanceDeployment`s refer to/depend on the `ServiceTypeDeployment`s.

Details can
be seen in the following class diagram:

<img src="/swh/ddad_platform/aas/mw/com/design/configuration/structural_view.uxf" />
