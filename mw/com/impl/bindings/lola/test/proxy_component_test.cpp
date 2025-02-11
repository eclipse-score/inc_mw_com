// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#include "platform/aas/lib/filesystem/i_standard_filesystem.h"
#include "platform/aas/lib/result/result.h"
#include "platform/aas/mw/com/impl/bindings/lola/element_fq_id.h"
#include "platform/aas/mw/com/impl/bindings/lola/proxy.h"
#include "platform/aas/mw/com/impl/bindings/lola/rollback_data.h"
#include "platform/aas/mw/com/impl/bindings/lola/runtime_mock.h"
#include "platform/aas/mw/com/impl/bindings/lola/test/proxy_event_test_resources.h"
#include "platform/aas/mw/com/impl/bindings/lola/test_doubles/fake_service_data.h"
#include "platform/aas/mw/com/impl/configuration/lola_event_id.h"
#include "platform/aas/mw/com/impl/configuration/lola_event_instance_deployment.h"
#include "platform/aas/mw/com/impl/configuration/lola_service_instance_deployment.h"
#include "platform/aas/mw/com/impl/configuration/lola_service_type_deployment.h"
#include "platform/aas/mw/com/impl/configuration/quality_type.h"
#include "platform/aas/mw/com/impl/configuration/service_identifier_type.h"
#include "platform/aas/mw/com/impl/handle_type.h"
#include "platform/aas/mw/com/impl/instance_identifier.h"
#include "platform/aas/mw/com/impl/service_discovery_mock.h"

#include "platform/aas/lib/os/unistd.h"

#include <amp_optional.hpp>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <algorithm>
#include <cstdint>
#include <new>
#include <string>
#include <utility>
#include <vector>

namespace bmw
{
namespace mw
{
namespace com
{
namespace impl
{
namespace lola
{
namespace
{

class ProxyWithRealMemFixture : public ::testing::Test
{
  public:
    void RegisterShmFile(std::string shm_file) { shm_files_.push_back(std::move(shm_file)); }

  protected:
    void SetUp() override
    {
        EXPECT_CALL(runtime_mock_.mock_, GetBindingRuntime(BindingType::kLoLa))
            .WillRepeatedly(::testing::Return(&lola_runtime_mock_));
        ON_CALL(lola_runtime_mock_, GetRollbackData()).WillByDefault(::testing::ReturnRef(rollback_data_));
        ON_CALL(runtime_mock_.mock_, GetServiceDiscovery())
            .WillByDefault(::testing::ReturnRef(service_discovery_mock_));
    }

    void TearDown() override
    {
        for (const auto& file : shm_files_)
        {
            bmw::filesystem::IStandardFilesystem::instance().Remove(std::string{"/dev/shm"} + file);
        }
        shm_files_.clear();
    }

    std::unique_ptr<FakeServiceData> CreateFakeSkeletonData(std::string control_file_name,
                                                            std::string data_file_name,
                                                            std::string service_instance_usage_marker_file,
                                                            bool init = true)
    {
        auto fake_skeleton_data = FakeServiceData::Create(control_file_name,
                                                          data_file_name,
                                                          service_instance_usage_marker_file,
                                                          os::Unistd::instance().getpid(),
                                                          init);
        if (fake_skeleton_data == nullptr)
        {
            return {};
        }
        RegisterShmFile(control_file_name);
        RegisterShmFile(data_file_name);

        return fake_skeleton_data;
    }

    std::vector<std::string> shm_files_{};
    RuntimeMockGuard runtime_mock_{};
    lola::RuntimeMock lola_runtime_mock_{};
    RollbackData rollback_data_{};
    ServiceDiscoveryMock service_discovery_mock_{};
};

TEST_F(ProxyWithRealMemFixture, IsEventProvidedOnlyReturnsTrueIfEventIsInSharedMemory)
{
#if defined(__QNXNTO__)
    constexpr auto kServiceInstanceUsageMarkerFile =
        "/tmp_discovery/mw_com_lola/partial_restart/usage-0000000000052719-00016";
#else
    constexpr auto kServiceInstanceUsageMarkerFile = "/tmp/mw_com_lola/partial_restart/usage-0000000000052719-00016";
#endif

    const bool initialise_skeleton_data{true};
    auto fake_data_result = CreateFakeSkeletonData("/lola-ctl-0000000000052719-00016",
                                                   "/lola-data-0000000000052719-00016",
                                                   kServiceInstanceUsageMarkerFile,
                                                   initialise_skeleton_data);
    ASSERT_NE(fake_data_result, nullptr);

    const std::string event_name{"DummyEvent1"};
    const std::string non_provided_event_name{"DummyEvent2"};
    const ElementFqId element_fq_id{0xcdef, 0x5, 0x10, ElementType::EVENT};
    const ElementFqId non_provided_element_fq_id{0xcdef, 0x6, 0x10, ElementType::EVENT};

    // Only provide the first event in shared memory
    fake_data_result->AddEvent<std::uint8_t>(element_fq_id, SkeletonEventProperties{10U, 3U, true});

    LolaServiceInstanceDeployment shm_binding{LolaServiceInstanceId{element_fq_id.instance_id_},
                                              {{event_name, LolaEventInstanceDeployment{10U, 10U, 2U}},
                                               {non_provided_event_name, LolaFieldInstanceDeployment{10U, 10U, 2U}}}};
    LolaServiceTypeDeployment service_deployment{
        element_fq_id.service_id_,
        {{event_name, LolaEventId{element_fq_id.element_id_}},
         {non_provided_event_name, LolaEventId{non_provided_element_fq_id.element_id_}}}};
    auto service_identifier = make_ServiceIdentifierType("foo");
    auto service_type_deployment = ServiceTypeDeployment{service_deployment};
    auto instance_specifier_result = InstanceSpecifier::Create("/my_dummy_instance_specifier");
    ASSERT_TRUE(instance_specifier_result.has_value());
    auto service_instance_deployment = ServiceInstanceDeployment{
        service_identifier, shm_binding, QualityType::kASIL_QM, std::move(instance_specifier_result).value()};
    auto identifier = make_InstanceIdentifier(service_instance_deployment, service_type_deployment);
    auto handle = make_HandleType(identifier, ServiceInstanceId{LolaServiceInstanceId{element_fq_id.instance_id_}});

    ON_CALL(service_discovery_mock_, StartFindService(::testing::_, EnrichedInstanceIdentifier{handle}))
        .WillByDefault(::testing::Return(make_FindServiceHandle(10U)));

    auto proxy = Proxy::Create(handle);
    ASSERT_NE(proxy, nullptr);
    EXPECT_TRUE(proxy->IsEventProvided(event_name));
    EXPECT_FALSE(proxy->IsEventProvided(non_provided_event_name));
}

}  // namespace
}  // namespace lola
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
