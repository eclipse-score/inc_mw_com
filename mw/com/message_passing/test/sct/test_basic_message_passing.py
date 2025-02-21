# *******************************************************************************
# Copyright (c) 2025 Contributors to the Eclipse Foundation
#
# See the NOTICE file(s) distributed with this work for additional
# information regarding copyright ownership.
#
# This program and the accompanying materials are made available under the
# terms of the Apache License Version 2.0 which is available at
# https://www.apache.org/licenses/LICENSE-2.0
#
# SPDX-License-Identifier: Apache-2.0
# *******************************************************************************

import sctf

class Commander(sctf.BaseSim):
    def __init__(self, environment):
        super().__init__(
            environment=environment,
            binary_path="bin/messaging_app_mqueue",
            args=["-m", "send", "-n", "10", "-b", "5"],
            wait_on_exit=True,
            cwd="/opt/messaging_app_mqueue",
            enforce_clean_shutdown=True,
        )


class Controller(sctf.BaseSim):
    def __init__(self, environment):
        super().__init__(
            environment=environment,
            binary_path="bin/messaging_app_mqueue",
            args=["-m", "recv", "-n", "10", "-b", "5"],
            wait_on_exit=True,
            cwd="/opt/messaging_app_mqueue",
            enforce_clean_shutdown=True,
        )


def test_basic_message_passing_commander_first(simple_environment_fixture):
    """
    !@brief Start one commander which will wait until a Controller is there. Once the controller is started, the
            commander is started, the commander will send a sequence of messages. This sequence of messages is then
            printed and checked for validity by the controller.
    """

    with Commander(simple_environment_fixture):
        with Controller(simple_environment_fixture):
            pass


def test_basic_message_passing_controller_first(simple_environment_fixture):
    """
    !@brief Start a controller first. Once the controller is started, the
            commander is started, the commander will send a sequence of messages. This sequence of messages is then
            printed and checked for validity by the controller.
    """

    with Controller(simple_environment_fixture):
        with Commander(simple_environment_fixture):
            pass


if __name__ == "__main__":
    sctf.run(__file__)
