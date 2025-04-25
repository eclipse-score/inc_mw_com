// Copyright (c) 2025 Qorix GmbH
//
// This program and the accompanying materials are made available under the
// terms of the Apache License, Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0.
//
// SPDX-License-Identifier: Apache-2.0

//! # qor-com Library
//!
//! Qor-com is a high-performance communication library designed for safety-critical, embedded real-time systems.
//!
//! It provides a set of tools and abstractions to facilitate efficient and reliable
//! communication between different components of a system.
//!
//! ## Features
//!
//! - High performance communication abstractions
//! - Remote Procedure Calls (RPC)
//! - Publish/Subscribe Topics
//! - Events
//! - Access control and security
//! - Performance tracing
//! - Real-time capabilities
//! - Easy-to-use API
//! - Support for multiple platforms
//!
//! ## Getting Started
//!
//! To use qor-com in your project, add the following dependency to your `Cargo.toml`:
//!
//! ```toml
//! [dependencies]
//! qor-com = "0.1.0"
//! ```
//!
//! Then, import the necessary modules in your code:
//!
//! ```rust
//! use qor_com::prelude::*;
//! ```
//!
//! ## Examples
//!
//! Here is a simple example of how to use qor-com:
//!
//! ```rust
//! // Example code goes here
//! ```
//!
//! ## Concepts
//!
//! The communication framework essentially assumes a service-oriented architecture.
//!
//! ### Information Elements of Communication
//!
//! Communication is about the exchange of information between two or more entities.
//!
//! #### Fundamental Elements
//!
//! Three fundamental information elements are distinguished:
//!
//! - *Topics* are elements that carry typed values as information.
//! - *Events* are elements that carry the runtime notifications of 'something occurred' as information.
//! - *Remote Procedure* are elements that carry a typed result as information. The result is obtained by *invocation* of the *Remote Procedure* with *arguments* as information.
//!
//! These elements represent the information entity itself. For interaction with each of these elements, a *publisher* and a *subscriber* are required.
//! The publisher provides ('publishes') information associated with the element. The subscriber consumes the information.
//!
//! For each of the three information elements the publisher
//!
//! - Sends ('publishes') new values to a *Topic*
//! - Notifies an *Event*, i.e. 'publishes' the occurrence.
//! - Implements a *Remote Procedure* with *Parameters* and sends a *Result*, i.e. 'publishes' a method's result and how it is created.
//!
//! For each of the three information elements the subscriber
//!
//! - Receives ('subscribes' to) values from a *Topic*´.
//! - Listens ('subscribes') to an *Event*.
//! - Invokes a *Remote Procedure* with *Arguments* and receives ('subscribes' to) a *Result* generated through the remote procedure.
//!
//! **Note**: For remote procedures, the analogy is that of an async functions: The publisher creates a `future` ('publish'),
//!           based upon arguments delivered with the invocation. The subscriber `await`s the result.
//!
//! #### Interface, Service and Client
//!
//! Grouping the publisher and subscriber elements and tagging them with a name constitutes an *Interface*
//! where they act as *Ports*. That way, an *Interface* becomes a collection of needs, i.e. element to subscribe to and provisions, i.e. elements to publish.
//!
//! *Interface*: An interface is a versioned high-cohesion collection of name tagged publishers and subscribers to *Topics*, *Events* and *Remote Procedures*.
//!     It defines a communication contract between the communication partners inside and outside of the interface.
//!     As part of an interface, we also find the term *Property* for topics and *Method* for remote procedures.
//!
//! Elements that fulfill the contract of an interface from the inside are said to 'implement the interface'. We call them *Services*.
//!
//! Elements that fulfill the contract of an interface from the outside are said to 'use the interface'. We call them *Clients*.
//!
//! #### Scope of Communication Elements
//!
//! Information elements, Interfaces, Services and Clients are entities of the *Scope* they are defined in.
//! A *Scope* is a logical partition of the communication framework in which the contained elements are mutually visible and accessible.
//!
//! A *Scope* can have nested other *Scopes* and therefore form a hierarchy.
//!
//! *Interface* and *Service* are *Scopes*.
//! Scopes have two more representations: *Packages* and *Modules*.
//!
//! A *Package* is a top-level scope of distinct elements that form a distribution unit. This means, the content of a package is either completely present or absent.
//!
//! A *Module* opens a scope within a package that operates as segmentation within the package.
//!
//! Giving an element a *Name* within a *Scope* links the element to the scope and makes it visible within the scope.
//! The attached name of an element must be unique within the scope.
//!
//! As scopes are potentially nested, the sequence of names from the root scope to the leaf scope is called a *Path*.
//!
//! *Mapping* a scope into another scope means to make the mapped scope visible and accessible in the context of the mapping scope.
//!
//! The path separator symbol is '/'. Paths can be absolute, starting with '/' or relative. Absolute paths refer to the package root they are used in.
//! Relative paths refer to the current scope.
//!
//! **Example**: The paths "/vehicle/chassis/wheel/fl" and "/vehicle/chassis/wheel/fr" are two instances of the same wheel service.
//!
//! **Example**: From multiple instances of services providing a vehicle speed we map the "/vehicle/chassis/speed/0" into our namespace "/vehcile/adas" as "speed".
//!
//! #### Origin of Communication Elements
//!
//! When describing Communication Elements with the properties and eventually attached name that describtion we call in the first place the *Declaration* of an element.
//! The declaration tells about the structure and properties of the element, but is in itself not yet the instance of the element.
//!
//! The instantiation of an element according to the declaration in a physical form we call *Definition* of the element. In a distributed system, the definition of an
//! element happens at a physical place.
//! That place we call the *Origin* of the element. The *Origin* we describe and share through the elements of the communication infrastructure.
//!
//!
//! ### Infrastructure Elements of Communication
//TODO: We have to rethink this. A node has a memory adapter to create all the nice elements we need. That constitutes the origin and home of the elements.
//      In addition we need a transport adapter for acutally moving the elements around.
//!
//! The communication framework is built on an infrastructure of interconnected *Nodes*.
//! *Nodes* also form the [*Origin*](#Origin_of_Communication_Elements) of the information elements.
//!
//! The interconnection of nodes sharing the same means of information exchange is called the *Transport* system.
//!
//! Different transport systems exist. We distinguish
//!
//! - Memory based transport systems, basically implemented through IPC mechanisms utilizing shared memory.
//! - Wire based transport systems, where essentially network protocols (UDP, TCP/IP, SOME/IP) are summarized.
//!
//! All connected nodes that can exchange information with each other form a *Fabric*.
//!
//! **Example**: Assume ten processes running under a single operating system with one node in each of the processes.
//!              The nodes share a single IPC framework as transport system.
//!              This is a *Mesh*.
//!
//! **Example**: Many nodes communicate through a SOME/IP protocol. The network this protocol runs on forms a *Mesh*.
//!
//! ![Communication Network Fabric](../assets/fabric.png)
//!
//!
//! ## Communication Framework Structure
//!
//! The qor-com library has a layered architecture.
//!
//! - API layer
//! - Routing layer
//! - Transport layer
//! - Transport memory layer
//!
//! ### The API Layer
//!
//! The API layer provides the fundamtental communication abstractions to the application.
//! This is the only layer visible to the application.
//!
//! The application is always considered as the local *Package*. This means, each application has a distinct scope within the global scope.
//!
//! It provides the following functionality:
//!
//! - Creation of *Modules* as scopes within the local package
//! - Mapping of *Scopes*
//! - Creation of *Interfaces* with their *Topics*, *Methods* and *Events*
//! - Creation of *Services* and *Clients* that implement or use the *Interfaces*
//! - Publish Services within a scope
//! - Discover Services by path
//! - Subscribe to and unsubscribe from Topics and Events
//! - Publish values to and read values from Topics
//! - Notify and wait for Events
//! - Invoke Methods with *Arguments* and wait for *Results* received
//!
//! The API layer also allows the configuration of the lower layers for expert use.
//!
//! ```rust
//! use qor_com::prelude::*;
//!
//! struct Calculator {
//!    digits: [u8;10];
//! }
//!
//! impl Calculator {
//!    fn new() -> Self {
//!       Self { digits: [0;10] }
//!    }
//!
//!    fn add(&mut self, a: f32, b: f32) -> f32 {
//!        let result = a + b;
//!        self.digits = bcd_digits(result);
//!        result
//!    }
//!
//!    fn sub(&mut self, a: f32, b: f32) -> f32 {
//!        let result = a - b;
//!        self.digits = bcd_digits(result);
//!        result
//!    }
//! }
//!
//! fn bcd_digits(n: f32) -> [u8;10] {
//!    let mut digits = [0;10];
//!    let mut n = n;
//!    for i in 0..10 {
//!       digits[i] = (n % 10.0) as u8;
//!       n = n / 10.0;
//!    }
//!    digits
//! }
//!
//! fn main() {
//!    // name the package
//!    let package = package::application();
//!    package.set_name("demo").unwrap();
//!
//!    // create an interface
//!    let interface = InterfaceBuilder::new()
//!       .with_version(Version::new(0, 1, 0))
//!       .with_topic<[u8;10]>("digits")
//!       .with_method<Fn(f32, f32)->f32>("add")
//!       .with_method<Fn(f32, f32)->f32>("sub")
//!       .with_event("powerup")
//!       .build();
//!
//!    let calc = Rc::new(Calculator::new());
//!    let calc_add = calc.clone();
//!    let calc_sub = calc.clone();
//!
//!    // create a service in the root scope
//!    let service = ServiceBuilder::new(interface)
//!       .bind_method("add", move |a, b| calc_add.add(a, b))
//!       .bind_method("sub", move |a, b| calc_sub.sub(a, b))
//!       .build();
//!
//!    // publish the service in the package scope
//!    service.publish("calculator", package);
//!
//!    // spawn the service
//!    service.spawn();
//!
//!    // run the communication loop
//!    com::join();
//! }
//! ```
//!
//!
//! ### The Routing Layer
//!
//! The routing layer is a general abstraction of channeling information between *Endpoints* utilizing *Nodes* and *Links*.
//!
//! #### Addressing
//!
//! *Nodes* have an address called *NodeAddress* that is unique within the fabric the node is in. Using the NodeAddress, the Routing layer can physically find any node in the fabric.
//! *Endpoints* also have an address called *EndpointAddress* that is unique among the endpoints attached to the same node. A node can dispatch information to the
//! endpoint using the EndpointAddress.
//!
//! *NodeAddress* and *EndpointAddress* are combined to form an *Address* that uniquely identifies an endpoint in the fabric.
//!
//! #### Names
//!
//! Addresses can have aliases called *Names* providing a human-readable way to resolve addresses. In a path, address names start with a "//" signature as first characters of the path.
//! The dot '.' should be used to separate elements of the address name.
//!
//! **Example**: The address name "qor://ai.qorix.services/weather" is an alias for the Qorix cloud services weather service.
//!
//! Resolution of names can be static by configuration and dynamic by propagation. Both allow the implementation of discovery mechanisms.
//!
//! #### Payload
//!
//! The routing layer fundamentally introduces the notion of a *Payload* for any data values to be transported
//!
//! **Note**: Depending on the binding and capabilities of the underlying transport layer *Events* may not require payload.
//!
//! In general, for each information exchange, the sequence of handling the dispatch of payload is as follows:
//!
//! 1) Open up an appropriate channel from the transport layer to move the payload.
//! 1) Acquire an appropriate payload buffer from the transport layer.
//! 1) Fill the payload buffer with the information to be communicated.
//! 1) Send and forget the payload buffer.
//!
//! The receive sequence for payload is
//!
//! 1) Open up an appropriate channel from the transport layer to receive the payload.
//! 1) Wait for a payload buffer to become available.
//! 1) Process the payload in the buffer.
//! 1) Release the payload buffer.
//!
//! This procedure allows the use of zero-copy techniques and efficient memory management if the transport layer provides the necessary support.
//!
//!
//!
//! ### The Transport Layer
//!
//! ## License
//!
//! This project is licensed under the terms of the Eclipse Public License 2.0 or the Apache License, Version 2.0.
//!
//! ## Contributors
//!
//! - Qorix Performance Team, <qore@qorix.ai>
//! - Nico Hartmann (nico.hartmann@web.de)
//!
//!

/// The prelude module contains the core functionality of the qor-com library.
pub mod prelude {
    pub use crate::base::*;
    pub use crate::concepts::*;
    pub use crate::types::*;

    pub use crate::adapter;

    #[cfg(feature = "dynamic_adapter")]
    pub use crate::adapter::dynamic::*;
}

/// Basic definitions of communication
pub mod base;

/// Concepts of communication
pub mod concepts;

/// Convenience types aliases of communication
pub mod types {
    pub use crate::concepts::*;

    #[cfg(feature = "signals_supported")]
    pub type SignalBuilder<A> = <A as TransportAdapterConcept>::SignalBuilder;
    #[cfg(feature = "signals_supported")]
    pub type Signal<A> = <SignalBuilder<A> as SignalBuilderConcept<A>>::Signal;
    #[cfg(feature = "signals_supported")]
    pub type SignalEmitter<A> = <Signal<A> as SignalConcept<A>>::Emitter;
    #[cfg(feature = "signals_supported")]
    pub type SignalCollector<A> = <Signal<A> as SignalConcept<A>>::Collector;

    #[cfg(feature = "topics_supported")]
    pub type TopicBuilder<A, T> = <A as TransportAdapterConcept>::TopicBuilder<T>;
    #[cfg(feature = "topics_supported")]
    pub type Topic<A, T> = <TopicBuilder<A, T> as TopicBuilderConcept<A, T>>::Topic;
    #[cfg(feature = "topics_supported")]
    pub type TopicPublisher<A, T> = <Topic<A, T> as TopicConcept<A, T>>::Publisher;
    #[cfg(feature = "topics_supported")]
    pub type TopicSubscriber<A, T> = <Topic<A, T> as TopicConcept<A, T>>::Subscriber;
    #[cfg(feature = "topics_supported")]
    pub type SampleMaybeUninit<A, T> =
        <TopicPublisher<A, T> as PublisherConcept<A, T>>::SampleMaybeUninit;
    #[cfg(feature = "topics_supported")]
    pub type SampleMut<A, T> =
        <SampleMaybeUninit<A, T> as SampleMaybeUninitConcept<A, T>>::SampleMut;
    #[cfg(feature = "topics_supported")]
    pub type Sample<A, T> = <SampleMut<A, T> as SampleMutConcept<A, T>>::Sample;

    #[cfg(feature = "rpcs_supported")]
    pub type RpcBuilder<A, F, Args, R> = <A as TransportAdapterConcept>::RpcBuilder<F, Args, R>;
    #[cfg(feature = "rpcs_supported")]
    pub type Rpc<A, F, Args, R> =
        <RpcBuilder<A, F, Args, R> as RpcBuilderConcept<A, F, Args, R>>::Rpc;
    #[cfg(feature = "rpcs_supported")]
    pub type RpcInvoker<A, F, Args, R> = <Rpc<A, F, Args, R> as RpcConcept<A, F, Args, R>>::Invoker;
    #[cfg(feature = "rpcs_supported")]
    pub type RpcRequestMaybeUninit<A, F, Args, R> =
        <RpcInvoker<A, F, Args, R> as InvokerConcept<A, F, Args, R>>::RequestMaybeUninit;
    #[cfg(feature = "rpcs_supported")]
    pub type RpcRequestMut<A, F, Args, R> =
        <RpcRequestMaybeUninit<A, F, Args, R> as RequestMaybeUninitConcept<A, F, Args, R>>::RequestMut;
    #[cfg(feature = "rpcs_supported")]
    pub type RpcPendingRequest<A, F, Args, R> =
        <RpcRequestMut<A, F, Args, R> as RequestMutConcept<A, F, Args, R>>::PendingRequest;
    #[cfg(feature = "rpcs_supported")]
    pub type RpcResponse<A, F, Args, R> =
        <RpcPendingRequest<A, F, Args, R> as PendingRequestConcept<A, F, Args, R>>::Response;
    #[cfg(feature = "rpcs_supported")]
    pub type RemoteProcedureInvoked<A, F, Args, R> =
        <Rpc<A, F, Args, R> as RpcConcept<A, F, Args, R>>::Invoked;
    #[cfg(feature = "rpcs_supported")]
    pub type RpcRequest<A, F, Args, R> =
        <RemoteProcedureInvoked<A, F, Args, R> as InvokedConcept<A, F, Args, R>>::Request;
    #[cfg(feature = "rpcs_supported")]
    pub type RpcResponseMaybeUninit<A, F, Args, R> =
        <RpcRequest<A, F, Args, R> as RequestConcept<A, F, Args, R>>::ResponseMaybeUninit;
    #[cfg(feature = "rpcs_supported")]
    pub type RpcResponseMut<A, F, Args, R> =
        <RpcResponseMaybeUninit<A, F, Args, R> as ResponseMaybeUninitConcept<A, F, Args, R>>::ResponseMut;
}

/// Default transport adapter implementations
pub mod adapter;
