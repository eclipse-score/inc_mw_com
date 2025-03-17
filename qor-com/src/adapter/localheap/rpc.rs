// Copyright (c) 2025 Qorix GmbH
//
// This program and the accompanying materials are made available under the
// terms of the Apache License, Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0.
//
// SPDX-License-Identifier: Apache-2.0

use super::topic::*;
use super::HeapAdapter;

use crate::base::*;
use qor_core::prelude::*;

use std::sync::atomic::AtomicBool;
use std::sync::atomic::AtomicU64;
use std::sync::atomic::AtomicUsize;
use std::{fmt::Debug, sync::Arc, time::Duration};

//
// RemoteProcedure
//

/// The HeapInvoker implementation. Every invoker has a unique Id that is given by the heap remote procedure.
#[derive(Debug)]
struct HeapInvoker<'s, 't, Args, R>
where
    Args: ParameterPack,
    R: Return,
{
    /// Id of the invoker. This is used for requests together with parameters.
    client_tag: Tag,

    /// The request transaction counter
    transaction_counter: AtomicUsize,

    /// The request publisher for the arguments a call sends: (client_tag, transaction, Args)
    request: HeapTopic<'s, 't, RequestMessage<Args>>,

    /// The response subscriber for the result.
    /// The channel receives (transaction, R).
    response: HeapTopic<'s, 't, ResponseMessage<R>>,
}

impl<'s, 't, Args, R> Invoker<'s, 't, HeapAdapter<'t>, Args, R> for HeapInvoker<'s, 't, Args, R>
where
    Args: ParameterPack + 's,
    R: Return + 's,
    't: 's,
{
    fn prepare(
        &'s self,
        args: Args,
    ) -> ComResult<(
        impl PublishBuffer<'s, 't, HeapAdapter<'t>, RequestMessage<Args>>,
        impl Subscriber<'s, 't, HeapAdapter<'t>, ResponseMessage<R>>,
    )> {
        // get the request and response buffers
        let (req, res) = self.prepare_uninit()?;

        // we increment our transaction counter
        let counter = self
            .transaction_counter
            .fetch_add(1, std::sync::atomic::Ordering::Relaxed);

        // write the arguments to the request buffer (and convert the buffer to initialized)
        let req = req.write((self.client_tag, counter, args));

        // we are prepared with arguments written
        Ok((req, res))
    }

    fn prepare_uninit(
        &'s self,
    ) -> ComResult<(
        impl PublishBufferMaybeUninit<'s, 't, HeapAdapter<'t>, RequestMessage<Args>>,
        impl Subscriber<'s, 't, HeapAdapter<'t>, ResponseMessage<R>>,
    )> {
        // loan the request buffer for the arguments
        let req = self.request.loan_uninit()?;

        // clone the result subscriber
        let res = self.response.clone();

        // we are prepared with an uninitialized request buffer
        Ok((req, res))
    }

    fn invoke(
        &'s self,
        args: Args,
    ) -> ComResult<impl BufferMut<'s, 't, HeapAdapter<'t>, ResponseMessage<R>>> {
        // we increment our transaction counter
        let counter = self
            .transaction_counter
            .fetch_add(1, std::sync::atomic::Ordering::Relaxed);

        // prepare the request and response buffers
        let req = self.request.loan_with((self.client_tag, counter, args))?;

        // send the request
        req.publish()?;

        // wait for the response
        let res = self.response.receive()?;
        Ok(res)
    }

    async fn invoke_async(
        &'s self,
        args: Args,
    ) -> ComResult<impl BufferMut<'s, 't, HeapAdapter<'t>, ResponseMessage<R>>> {
        self.invoke(args)
    }
}

impl<'s, 't, Args, R> HeapInvoker<'s, 't, Args, R>
where
    Args: ParameterPack,
    R: Return,
    't: 's,
{
    #[inline(always)]
    fn new(
        client_tag: Tag,
        request: HeapTopic<'s, 't, RequestMessage<Args>>,
        response: HeapTopic<'s, 't, ResponseMessage<R>>,
    ) -> Self {
        Self {
            client_tag,
            transaction_counter: AtomicUsize::new(0),
            request,
            response,
        }
    }
}

/// The HeapInvokee implementation. Every invokee has a unique Id that is given by the heap remote procedure.
#[derive(Debug)]
struct HeapInvokee<'s, 't, Args, R>
where
    Self: 's,
    Args: ParameterPack + 's,
    R: Return + 's,
    't: 's,
{
    /// The request subscriber for the arguments a call sends: (<Instance>, <Transaction>, <Args>)
    request_channel: HeapTopic<'s, 't, RequestMessage<Args>>,

    /// The known response publishers for the results.
    response_channels: Arc<[HeapTopic<'s, 't, ResponseMessage<R>>]>,
}

impl<'s, 't, Args, R> Invokee<'s, 't, HeapAdapter<'t>, Args, R> for HeapInvokee<'s, 't, Args, R>
where
    Args: ParameterPack,
    R: Return,
    't: 's,
{
    fn try_accept(
        &'s self,
    ) -> ComResult<
        Option<(
            impl Buffer<'s, 't, HeapAdapter<'t>, RequestMessage<Args>>,
            impl PublishBufferMaybeUninit<'s, 't, HeapAdapter<'t>, ResponseMessage<R>>,
        )>,
    > {
        // check incoming request
        if let Some(req) = self.request_channel.try_receive()? {
            let client_id: u64 = req.0.into();
            if client_id >= self.response_channels.len() as u64 {
                return Err(Error::from_code(qor_core::core_errors::INVALID_TAG));
            }

            // lookup the response channel for the client
            let channel = &self.response_channels[client_id as usize];

            // loan the response buffer for the result
            let res = channel.loan_uninit()?;

            // we are prepared with an uninitialized result buffer
            Ok(Some((req.into_buffer(), res)))
        } else {
            Ok(None)
        }
    }

    fn accept(
        &'s self,
    ) -> ComResult<(
        impl Buffer<'s, 't, HeapAdapter<'t>, RequestMessage<Args>>,
        impl PublishBufferMaybeUninit<'s, 't, HeapAdapter<'t>, ResponseMessage<R>>,
    )> {
        // wait for incoming request
        let req = self.request_channel.receive()?;

        let client_id: u64 = req.0.into();
        if client_id >= self.response_channels.len() as u64 {
            return Err(Error::from_code(qor_core::core_errors::INVALID_TAG));
        }

        // lookup the response channel for the client
        let channel = &self.response_channels[client_id as usize];

        // loan the response buffer for the result
        let res = channel.loan_uninit()?;

        // we are prepared with an uninitialized result buffer
        Ok((req.into_buffer(), res))
    }

    fn accept_timeout(
        &'s self,
        timeout: Duration,
    ) -> ComResult<(
        impl Buffer<'s, 't, HeapAdapter<'t>, RequestMessage<Args>>,
        impl PublishBufferMaybeUninit<'s, 't, HeapAdapter<'t>, ResponseMessage<R>>,
    )> {
        // wait for incoming request
        let req = self.request_channel.receive_timeout(timeout)?;

        let client_id: u64 = req.0.into();
        if client_id >= self.response_channels.len() as u64 {
            return Err(Error::from_code(qor_core::core_errors::INVALID_TAG));
        }

        // lookup the response channel for the client
        let channel = &self.response_channels[client_id as usize];

        // loan the response buffer for the result
        let res = channel.loan_uninit()?;

        // we are prepared with an uninitialized result buffer
        Ok((req.into_buffer(), res))
    }

    async fn accept_async(
        &'s self,
    ) -> ComResult<(
        impl Buffer<'s, 't, HeapAdapter<'t>, RequestMessage<Args>>,
        impl PublishBufferMaybeUninit<'s, 't, HeapAdapter<'t>, ResponseMessage<R>>,
    )> {
        self.accept()
    }
}

impl<'s, 't, Args, R> HeapInvokee<'s, 't, Args, R>
where
    Args: ParameterPack + 's,
    R: Return + 's,
    't: 's,
{
    #[inline(always)]
    fn new(
        request_channel: HeapTopic<'s, 't, RequestMessage<Args>>,
        response_channels: Arc<[HeapTopic<'s, 't, ResponseMessage<R>>]>,
    ) -> Self {
        Self {
            request_channel,
            response_channels,
        }
    }
}

#[derive(Debug)]
struct HeapRemoteProcedureInner<'s, 't, Args, R>
where
    Self: 's,
    Args: ParameterPack + 's,
    R: Return + 's,
    't: 's,
{
    client_id: AtomicU64,

    request_channel: HeapTopic<'s, 't, RequestMessage<Args>>,
    response_channels: Arc<[HeapTopic<'s, 't, ResponseMessage<R>>]>,
    has_invokee: AtomicBool,
}

impl<'s, 't, Args, R> HeapRemoteProcedureInner<'s, 't, Args, R>
where
    Args: ParameterPack + 's,
    R: Return + 's,
    't: 's,
{
    #[inline(always)]
    fn new(
        max_queue_depth: usize,
        queue_policy: QueuePolicy,
        max_clients: usize,
    ) -> Self {
        // create the response channels array
        let mut response_channels = Vec::with_capacity(max_clients);
        for _ in 0..max_clients {
            response_channels.push(HeapTopic::new(max_queue_depth, queue_policy, 1, 1));
        }

        let response_channels: Arc<[HeapTopic<'s, 't, ResponseMessage<R>>]> =
            response_channels.into_boxed_slice().into();

        Self {
            client_id: AtomicU64::new(0),

            request_channel: HeapTopic::new(max_queue_depth, queue_policy, max_clients, 1),
            response_channels,
            has_invokee: AtomicBool::new(false),
        }
    }

    fn invoker(self: &Arc<Self>) -> ComResult<impl Invoker<'s, 't, HeapAdapter<'t>, Args, R>> {
        // generate client_id from client_id counter
        let client_id = self
            .client_id
            .fetch_add(1, std::sync::atomic::Ordering::Relaxed);

        // check index
        if client_id > self.response_channels.len() as u64 {
            self.client_id
                .fetch_sub(1, std::sync::atomic::Ordering::Relaxed);
            return Err(Error::from_code(qor_core::core_errors::COLLECTION_FULL));
        }

        // create the client tag from id
        let client_tag = Tag::from_const(client_id);

        // The request channel is a single channel for all clients
        let request = self.request_channel.clone();

        // The response channel is unique for each client, we are the only one that can write to it, the client is the only one that can read from it
        let response = self.response_channels[client_id as usize].clone();

        // create the invoker
        let invoker = HeapInvoker::new(client_tag, request, response);
        Ok(invoker)
    }

    #[inline(always)]
    fn invokee(self: &Arc<Self>) -> ComResult<impl Invokee<'s, 't, HeapAdapter<'t>, Args, R>> {
        // check if we already have an invokee
        if self
            .has_invokee
            .swap(true, std::sync::atomic::Ordering::Relaxed)
        {
            return Err(Error::from_code(qor_core::core_errors::COLLECTION_FULL));
        }

        // create the invokee
        let invokee = HeapInvokee::new(
            self.request_channel.clone(),
            self.response_channels.clone(),
        );

        Ok(invokee)
    }
}

#[derive(Debug)]
pub struct HeapRemoteProcedure<'s, 't, Args, R>
where
    Args: ParameterPack + 's,
    R: Return + 's,
    't: 's,
{
    inner: Arc<HeapRemoteProcedureInner<'s, 't, Args, R>>,
}

impl<'s, 't, Args, R> Clone for HeapRemoteProcedure<'s, 't, Args, R>
where
    Args: ParameterPack + 's,
    R: Return + 's,
{
    fn clone(&self) -> Self {
        Self {
            inner: self.inner.clone(),
        }
    }
}

impl<'s, 't, Args, R> RemoteProcedure<'s, 't, HeapAdapter<'t>, Args, R>
    for HeapRemoteProcedure<'s, 't, Args, R>
where
    Args: ParameterPack + 's,
    R: Return + 's,
    't: 's,
{
    #[inline(always)]
    fn invoker(&self) -> ComResult<impl Invoker<'s, 't, HeapAdapter<'t>, Args, R>> {
        self.inner.invoker()
    }

    #[inline(always)]
    fn invokee(&self) -> ComResult<impl Invokee<'s, 't, HeapAdapter<'t>, Args, R>> {
        self.inner.invokee()
    }
}

impl<'s, 't, Args, R> HeapRemoteProcedure<'s, 't, Args, R>
where
    Args: ParameterPack + 's,
    R: Return + 's,
    't: 's,
{
    #[inline(always)]
    pub(crate) fn new(
        max_queue_depth: usize,
        queue_policy: QueuePolicy,
        max_clients: usize,
    ) -> Self {
        Self {
            inner: Arc::new(HeapRemoteProcedureInner::new(
                max_queue_depth,
                queue_policy,
                max_clients,
            )),
        }
    }
}

#[derive(Debug)]
pub struct HeapRemoteProcedureBuilder<Args, R>
where
    Args: ParameterPack,
    R: Return,
{
    _phantom_args: std::marker::PhantomData<Args>,
    _phantom_result: std::marker::PhantomData<R>,

    max_queue_depth: usize,
    queue_policy: QueuePolicy,
    max_clients: usize,
}

impl<'t, Args, R> RemoteProcedureBuilder<'t, HeapAdapter<'t>, Args, R>
    for HeapRemoteProcedureBuilder<Args, R>
where
    Args: ParameterPack,
    R: Return,
{
    fn with_queue_depth(mut self, queue_depth: usize) -> Self {
        self.max_queue_depth = queue_depth;
        self
    }

    fn with_queue_policy(mut self, queue_policy: QueuePolicy) -> Self {
        self.queue_policy = queue_policy;
        self
    }

    fn with_max_clients(mut self, max_clients: usize) -> Self {
        self.max_clients = max_clients;
        self
    }

    fn build<'a>(self) -> ComResult<impl RemoteProcedure<'a, 't, HeapAdapter<'t>, Args, R>>
    where
        Args: 'a,
        R: 'a,
        't: 'a,
    {
        Ok(HeapRemoteProcedure::new(
            self.max_queue_depth,
            self.queue_policy,
            self.max_clients,
        ))
    }
}

impl<Args, R> HeapRemoteProcedureBuilder<Args, R>
where
    Args: ParameterPack,
    R: Return,
{
    #[inline(always)]
    pub(crate) fn new(_label: Label) -> Self {
        Self {
            _phantom_args: std::marker::PhantomData,
            _phantom_result: std::marker::PhantomData,
            max_queue_depth: 8,
            queue_policy: QueuePolicy::Error,
            max_clients: 8,
        }
    }
}
