// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#include "platform/aas/lib/concurrency/thread_pool.h"
#include "platform/aas/mw/com/message_passing/receiver_factory.h"
#include "platform/aas/mw/com/message_passing/sender_factory.h"

#include <boost/program_options.hpp>

#include <csignal>
#include <exception>
#include <iostream>
#include <random>
#include <thread>
#include <unordered_map>

namespace
{

class PseudoRandomGenerator
{
  public:
    PseudoRandomGenerator(uint64_t seed = std::mt19937_64::default_seed) : generator_{seed} {}

    std::int8_t getId() { return static_cast<std::int8_t>(generator_() & 0x7F); }
    std::uint64_t getShort() { return generator_(); }
    std::array<uint8_t, 16> getMedium()
    {
        const std::uint64_t head = generator_();
        const std::uint64_t tail = generator_();
        std::array<uint8_t, 16> result;
        std::memcpy(&result[0], &head, sizeof(std::uint64_t));
        std::memcpy(&result[8], &tail, sizeof(std::uint64_t));
        return result;
    }

  private:
    std::mt19937_64 generator_;
};

constexpr char kReceiverIdentifier[] = "/message_passing_test_receiver0";

amp::stop_source stop_test{amp::nostopstate_t{}};

void sigterm_handler(int signal)
{
    if (signal == SIGTERM || signal == SIGINT)
    {
        std::cout << "Stop requested" << std::endl;
        amp::ignore = stop_test.request_stop();
    }
}

std::int32_t messaging_sender(const amp::stop_token token, const std::uint32_t num, const std::uint32_t burst)
{
    auto sender = bmw::mw::com::message_passing::SenderFactory::Create(kReceiverIdentifier, std::move(token));
    const auto pid = ::getpid();

    PseudoRandomGenerator prng{static_cast<std::uint64_t>(pid)};  // seed with sender's pid

    for (std::uint32_t counter = 0; counter < num; ++counter)
    {
        if (token.stop_requested())
        {
            return -1;
        }
        const std::int8_t id = prng.getId();
        const bool is_short = bool(id % 2);
        if (is_short)
        {
            bmw::mw::com::message_passing::ShortMessage message{};
            message.id = id;
            message.pid = pid;  // the receiver will check if the pid matches the sending process
            message.payload = prng.getShort();
            const auto result = sender->Send(message);
            if (!result.has_value())
            {
                std::cerr << "Short send returned error: " << result.error() << std::endl;
                
                /* Terminate call tolerated.See Assumptions of Use in mw/com/design/README.md*/
                std::terminate();
                
            }
        }
        else
        {
            bmw::mw::com::message_passing::MediumMessage message{};
            message.id = id;
            message.pid = pid;  // the receiver will check if the pid matches the sending process
            message.payload = prng.getMedium();
            const auto result = sender->Send(message);
            if (!result.has_value())
            {
                std::cerr << "Medium send returned error: " << result.error() << std::endl;
                
                /* Terminate call tolerated.See Assumptions of Use in mw/com/design/README.md*/
                std::terminate();
                
            }
        }
        if (counter % burst == burst - 1)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds{100});
        }
    }

    return 0;
}

std::int32_t messaging_receiver(amp::stop_token token,
                                const std::uint32_t num,
                                const std::uint32_t burst,
                                std::vector<uid_t> allowed_uids)
{
    std::unordered_map<pid_t, PseudoRandomGenerator> pid_prng_map;

    bmw::concurrency::ThreadPool executor{2};
    bmw::mw::com::message_passing::ReceiverConfig receiver_config{};
    receiver_config.max_number_message_in_queue = static_cast<std::int32_t>(burst);
    auto receiver = bmw::mw::com::message_passing::ReceiverFactory::Create(
        kReceiverIdentifier, executor, std::move(allowed_uids), receiver_config);

    amp::stop_callback stop{token, [&executor]() noexcept { executor.Shutdown(); }};

    std::atomic<std::size_t> count{0};

    for (std::uint8_t id_uint = 0; id_uint <= 0x7f; ++id_uint)
    {
        const std::int8_t id = static_cast<std::int8_t>(id_uint);
        const bool is_short = bool(id % 2);
        if (is_short)
        {
            receiver->Register(id, [id, &pid_prng_map, &count](std::uint64_t payload, pid_t pid) {
                if (pid_prng_map.find(pid) == pid_prng_map.end())
                {
                    pid_prng_map.emplace(std::piecewise_construct,
                                         std::forward_as_tuple(pid),
                                         std::forward_as_tuple(static_cast<std::uint64_t>(pid)));
                }
                auto& prng = pid_prng_map[pid];
                const std::int8_t id_expected = prng.getId();
                if (id != id_expected)
                {
                    std::cerr << "Short message: wrong id " << int{id} << ", expected " << int{id_expected}
                              << std::endl;
                    
                    /* Terminate call tolerated.See Assumptions of Use in mw/com/design/README.md*/
                    std::terminate();
                    
                }
                if (payload != prng.getShort())
                {
                    std::cerr << "Short message: wrong payload" << std::endl;
                    
                    /* Terminate call tolerated.See Assumptions of Use in mw/com/design/README.md*/
                    std::terminate();
                    
                }
                ++count;
            });
        }
        else
        {
            receiver->Register(id, [id, &pid_prng_map, &count](std::array<uint8_t, 16> payload, pid_t pid) {
                if (pid_prng_map.find(pid) == pid_prng_map.end())
                {
                    pid_prng_map.emplace(std::piecewise_construct,
                                         std::forward_as_tuple(pid),
                                         std::forward_as_tuple(static_cast<std::uint64_t>(pid)));
                }
                auto& prng = pid_prng_map[pid];
                const std::int8_t id_expected = prng.getId();
                if (id != id_expected)
                {
                    std::cerr << "Medium message: wrong id " << int{id} << ", expected " << int{id_expected}
                              << std::endl;
                    
                    /* Terminate call tolerated.See Assumptions of Use in mw/com/design/README.md*/
                    std::terminate();
                    
                }
                if (payload != prng.getMedium())
                {
                    std::cerr << "Medium message: wrong payload" << std::endl;
                    
                    /* Terminate call tolerated.See Assumptions of Use in mw/com/design/README.md*/
                    std::terminate();
                    
                }
                ++count;
            });
        }
    }

    receiver->StartListening();

    while (count != num)
    {
        if (token.stop_requested())
        {
            std::cerr << "messaging_receiver: wrong number of messages before interruption, " << count << ", expected "
                      << num << std::endl;
            
            /* Terminate call tolerated.See Assumptions of Use in mw/com/design/README.md*/
            std::terminate();
            
        }
        std::this_thread::sleep_for(std::chrono::milliseconds{300});
    }

    return 0;
}

}  // namespace

int main(int argc, const char** argv)
{
    namespace po = boost::program_options;

    stop_test = amp::stop_source{};
    const auto stop_token{stop_test.get_token()};

    if (std::signal(SIGTERM, sigterm_handler) == SIG_ERR || std::signal(SIGINT, sigterm_handler) == SIG_ERR)
    {
        std::cerr << "Unable to set signal handler for SIGINT and/or SIGTERM, cautiously continuing\n";
    }

    po::options_description options;
    // clang-format off
    options.add_options()
        ("help", "Display the help message")
        ("mode,m", po::value<std::string>()->required(), "Set to either 'send' or 'recv' to determine the "
                                                         "role of the process")
        ("num,n", po::value<std::uint32_t>()->default_value(10), "Number of messages to send or to expect to receive")
        ("burst,b", po::value<std::uint32_t>()->default_value(10), "Maximum amount of messages allowed in a single burst")
        ("uid,u", po::value<std::vector<uid_t>>(), "(recv) Uid[s] of the senders allowed to send messages to the receiver");
    // clang-format on
    po::variables_map args;
    const auto parsed_args =
        po::command_line_parser{argc, argv}
            .options(options)
            .style(po::command_line_style::unix_style | po::command_line_style::allow_long_disguise)
            .run();
    po::store(parsed_args, args);

    if (args.count("help") > 0U)
    {
        std::cerr << options << std::endl;
        return -1;
    }

    po::notify(args);
    const std::uint32_t num = args["num"].as<std::uint32_t>();
    const std::uint32_t burst = args["burst"].as<std::uint32_t>();

    const auto& mode = args["mode"].as<std::string>();
    if (mode == "send")
    {
        return messaging_sender(stop_token, num, burst);
    }
    else if (mode == "recv")
    {
        std::vector<uid_t> uids{};
        if (args.count("uid"))
        {
            uids = args["uid"].as<std::vector<uid_t>>();
        }
        return messaging_receiver(stop_token, num, burst, std::move(uids));
    }
    else
    {
        std::cerr << "Unknown mode " << mode << ", terminating." << std::endl;
        return -1;
    }
}
