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



# Communication Middleware (mw::com)

## Overview

Currently, there are two different components/libraries located under this git repo location:

- `LoLa`/`mw::com`: A high-level communication middleware as a partial implementation of the Adaptive AUTOSAR
  Communication Management specification. Also known as `ara::com` (as this is the related namespace in AUTOSAR).
- `Message Passing`: A low-level message-passing implementation, which is also used by `LoLa`/`mw::com`, but is
  independent and therefore can be used stand-alone.

The related directory structure actually needs some clean-up (see ticket
[](/browse/)), as the `Message Passing` library has a clear/distinct
location under `/mw/com/message_passing`, while the `LoLa`/`mw::com` library already has its public API parts directly
under `/mw/com`, and its implementation and design folders are also located directly under `/mw/com`, which lacks a
clear separation between `LoLa`/`mw::com` and `Message Passing`.

Here only `LoLa`/`mw::com` will be described. `Message Passing` will have its _Readme.md_ later
under `/mw/com/message_passing`.

## General introduction

`mw::com` and `LoLa` are **synonyms** right now. While the former just expresses the namespace (`mw::com` = middleware
communication), the latter is a hint on its technical implementation/unique selling point (`LoLa` = Low Latency).
Since `LoLa`/`mw::com` is our in-house implementation of the AUTOSAR standard, which &ndash; according to this standard
&ndash; resides under `ara::com`, the namespace similarity (`mw::com` <-> `ara::com`) is **intentional**.

Low latency in our `mw::com` implementation is realized by basing our underlying `technical binding` on zero-copy
shared-memory based communication.
The notion of a `technical binding` also comes from the `ara::com` AUTOSAR standard:
`ara::com` just defines a standardized front-end/user-level API and its behavior (which we generally follow with our
`mw::com` implementation). **How** an implementation realizes the public API within its lower layers
(`technical binding`) is completely up to its own.
It is even common, that implementations of the `ara::com` standard come up with different/several `technical binding`s
(e.g. one for local and one for remote/network communication).

## Documentation for users

If you are an adaptive application developer in the project, and you want to use `mw::com` to do local
interprocess communication, you will find the user documentation here: LINK TODO.

## Requirements
Our single source of truth for requirements is **_Codebeamer_**.

- The SW-Component requirements for `mw::com` are located [here]()
- The safety requirements for `mw::com` are located [here]()
- The assumptions of use (AoU) for mw::com are located [here]()

## List of contact people

### Main contacts

- Area Product Owner (APO) for mw::com
  - Lakmal Padmakumara ()
- Technical Area Lead (TAL) for mw::com
  - Xavier Bonaventura ()
- Developers
  - Brendan Emery ()
  - Manuel Fehlhammer ()
  - Wahaj Sethi Muhammad ()
 
 
## Useful links

- [High level architecture concept](/swh/xpad_documentation/tree/master/architecture/architecture_concept/communication/lola)
- [Detailed design](/swh/ddad_platform/tree/master/aas/mw/com/design)
