// Copyright (c) 2023 Contributors to the Eclipse Foundation
//
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
//
// This program and the accompanying materials are made available under the
// terms of the Apache Software License 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0, or the MIT license
// which is available at https://opensource.org/licenses/MIT.
//
// SPDX-License-Identifier: Apache-2.0 OR MIT

use core::time::Duration;
use iceoryx2::prelude::*;
use iceoryx2::sample_mut::SampleMut;
use iceoryx2::sample_mut_uninit::SampleMutUninit;
use iceoryx2_bb_container::byte_string::FixedSizeByteString;
use serde::{Deserialize, Serialize};
use std::fmt::Debug;
use std::mem::MaybeUninit;
use std::thread::{self, JoinHandle};

const CYCLE_TIME: Duration = Duration::from_secs(1);

#[derive(Default, Debug)]
#[repr(C)]
pub struct CustomHeader {
    pub version: i32,
    pub timestamp: u64,
}

#[derive(Debug, Serialize, Deserialize)]
#[repr(C)]
pub struct TransmissionData {
    pub x: i32,
    pub y: i32,
    pub funky: f64,
    pub data: FixedSizeByteString<100>,
}

struct ScorePubSubBuilder {
    service_name: Option<String>,
}

impl ScorePubSubBuilder {
    pub fn _load_from_config(&mut self, _json_config_file: &str) {
        // ... todo .. load json into builder config ...
        /***
        ```json
        { "bindings" {
          "service_name" : "score_example",
          "bindings" : [
            {
              "type" : "mqtt",
              "mqtt_configuration" : {
                    "server" : "192.168.0.5",
                    "client_name" : "important_sensor_data_producer_1",
                    "topic" : "/score_example",
                    "qos" : "AtLeastOnce"
                },
               "preprocessors" : [
                    {
                        "type" : "bucket_ratelimiter",
                        "bucket_ratelimiter_configuration" : {
                            "bucket_size" : 10,
                            "bucket_length_in_milliseconds": 1000,
                        }
                    }
                ]
            },
            {
                "type" : "iceoryx2",
                "iceoryx2_configuration" : {
                    "service_name": "score_example"
                }
            }
          ]
         }}
        ```
         ***/
    }

    pub fn service_name(&mut self, service_name: &str) -> &ScorePubSubBuilder {
        self.service_name = Some(service_name.to_owned());
        self
    }

    // add builder for different transports .. to conifgure mqtt server name, rate limiter et cetera

    pub fn new() -> ScorePubSubBuilder {
        ScorePubSubBuilder { service_name: None }
    }

    pub fn build<T>(&self) -> Result<ScorePubSub<T>, String>
    where
        T: Debug + Sized + Serialize + 'static,
    {
        let service_name = self.service_name.as_ref().ok_or("service_name not set")?;
        Ok(ScorePubSub {
            iceoryx2_transport: ScoreIceoryx2Transport::new(service_name),
            mqtt_transport: MqttTransport::new(service_name),
        })
    }
}

struct ScorePubSub<T>
where
    T: Debug + Sized + Serialize + 'static,
{
    iceoryx2_transport: ScoreIceoryx2Transport<T>,
    mqtt_transport: MqttTransport,
}

impl<T> ScorePubSub<T>
where
    T: Debug + Sized + Serialize + 'static,
{
    pub fn loan_sample(
        &self,
    ) -> SampleMutUninit<iceoryx2::service::ipc::Service, MaybeUninit<T>, CustomHeader> {
        // TODO: this should probably not be a lola or iceoryx functionality but rather something
        // that is _compatible_ with them, so they can use that data later when we want to publish it
        self.iceoryx2_transport.loan_sample()
    }

    pub fn send_sample(&self, sample: SampleMut<iceoryx2::service::ipc::Service, T, CustomHeader>) {
        // iterate over all bound transports and send the sample off

        // mqtt transport will serialize (and thus copy it) before sending it off
        self.mqtt_transport.send_sample(sample.payload());

        // Option to speed things up:
        // implement a subscriber for all non-time-critical transports and thus allow the producer
        // to continue on its merry way without having to wait for all other, slow transports
        // Think of a system integrator configuring additional binding, altering the behavior of
        // the software without the developer having though about that .. by cutting short the
        // time spend sending (on the producer's thread) this effect can be minimized

        // Note: We can only have one binding that consumes the data / uses the actual data and "sends it" away, right?
        // iceoryx2 comes last as it consumes the sample
        self.iceoryx2_transport.send_sample(sample);
    }

    pub fn wait_for_iceoryx(&self) -> bool {
        // Just a workaround for iceoryx .. similar to the mqtt notification thread .. not relevant for api design decision
        self.iceoryx2_transport.node.wait(CYCLE_TIME).is_ok()
    }
}

struct MqttTransport {
    client: rumqttc::Client,
    // no proper shutdown cleanup. oops.
    _mqtt_notification_thread: JoinHandle<()>,
    topic: String,
}

impl MqttTransport {
    pub fn new(service_name: &str) -> MqttTransport {
        let mut mqttoptions = rumqttc::MqttOptions::new("rumqttc-sync", "127.0.0.1", 1883);
        mqttoptions.set_keep_alive(Duration::from_secs(5));
        let (client, mut connection) = rumqttc::Client::new(mqttoptions, 10);

        // Implementation detail of rust mqtt client, we need such a thread in the background for housekeeping
        let mqtt_notification_thread = thread::spawn(move || {
            for notification in connection.iter() {
                match notification {
                    Ok(_) => {}
                    Err(_) => {
                        return;
                    }
                }
            }
        });
        MqttTransport {
            client,
            _mqtt_notification_thread: mqtt_notification_thread,
            topic: service_name.into(),
        }
    }

    pub fn send_sample(&self, payload: impl Serialize) {
        let json_str = serde_json::to_string(&payload).unwrap();
        println!("publish: {}", json_str);
        self.client
            .publish(
                &self.topic,
                rumqttc::QoS::AtLeastOnce,
                false,
                json_str.as_bytes(),
            )
            .unwrap();
    }
}

struct ScoreIceoryx2Transport<T>
where
    T: Debug + Sized + 'static,
{
    node: Node<iceoryx2::service::ipc::Service>,
    publisher:
        iceoryx2::port::publisher::Publisher<iceoryx2::service::ipc::Service, T, CustomHeader>,
}

impl<T> ScoreIceoryx2Transport<T>
where
    T: Debug + Sized + 'static,
{
    pub fn new(name: &str) -> ScoreIceoryx2Transport<T> {
        let node = NodeBuilder::new().create::<ipc::Service>().unwrap();
        let service_builder = node.service_builder(&name.try_into().unwrap());
        let publisher = service_builder
            .publish_subscribe::<T>()
            .user_header::<CustomHeader>()
            .open_or_create()
            .unwrap()
            .publisher_builder()
            .create()
            .unwrap();

        ScoreIceoryx2Transport { node, publisher }
    }

    pub fn loan_sample(
        &self,
    ) -> SampleMutUninit<iceoryx2::service::ipc::Service, MaybeUninit<T>, CustomHeader> {
        self.publisher.loan_uninit().unwrap()
    }

    pub fn send_sample(&self, sample: SampleMut<iceoryx2::service::ipc::Service, T, CustomHeader>) {
        // why pass this sample all the way down if we could've "sent" it earlier? because this
        // code is just demonstrating in principal where the logic should be ... with lola and other
        // bindings, things are different. we also may opt to wrap the sample itself and make it
        // aware of it's bindings!
        sample.send().unwrap();
    }
}

fn main() -> Result<(), Box<dyn core::error::Error>> {
    // Note: The frontend is basically a layer lower than the Runtime we talked about at the workshop on 2025-03-27.

    // The frontend we build is typed for TransmissionData
    let score_com_frontend: ScorePubSub<TransmissionData> = ScorePubSubBuilder::new() // Get a builder
        .service_name("/score_example2") // configure the frontend
        .build::<TransmissionData>()?; // build the transport and the bindings (already bound to TransmissionData datatype)

    // Send out samples
    let mut counter: u64 = 0;
    while score_com_frontend.wait_for_iceoryx() {
        counter += 1;

        // Get (shared) memory
        let sample = score_com_frontend.loan_sample();

        // Fill memory with data
        let data = format!("Hello there this is iteraton {counter}");
        let sample: SampleMut<iceoryx2::service::ipc::Service, TransmissionData, CustomHeader> =
            sample.write_payload(TransmissionData {
                x: counter as i32,
                y: counter as i32 * 3,
                funky: counter as f64 * 812.12,
                data: FixedSizeByteString::from_bytes(data.as_bytes()).unwrap(),
            });

        // Send it off
        score_com_frontend.send_sample(sample);

        println!("Sent sample {} ...", counter);
    }

    // No cleanup on exit, not even a proper shutdown handler.

    Ok(())
}
