// *******************************************************************************>
// Copyright (c) 2024 Contributors to the Eclipse Foundation
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
// SPDX-License-Identifier: Apache-2.0 #
// *******************************************************************************



#include "platform/aas/mw/com/impl/plumbing/proxy_binding_factory.h"
#include "platform/aas/lib/os/mocklib/fcntl_mock.h"
#include "platform/aas/mw/com/impl/bindings/lola/runtime_mock.h"
#include "platform/aas/mw/com/impl/bindings/lola/test/proxy_event_test_resources.h"
#include "platform/aas/mw/com/impl/bindings/mock_binding/proxy.h"
#include "platform/aas/mw/com/impl/configuration/lola_service_instance_id.h"
#include "platform/aas/mw/com/impl/plumbing/dummy_instance_identifier_builder.h"
#include "platform/aas/mw/com/impl/plumbing/proxy_binding_factory_mock.h"
#include "platform/aas/mw/com/impl/plumbing/skeleton_binding_factory.h"
#include "platform/aas/mw/com/impl/runtime.h"
#include "platform/aas/mw/com/impl/runtime_mock.h"
#include "platform/aas/mw/com/impl/service_discovery_mock.h"

#include <gtest/gtest.h>
#include <memory>
#include <utility>

namespace bmw
{
namespace mw
{
namespace com
{
namespace impl
{

namespace
{

using ::testing::_;
using ::testing::Return;

class RuntimeMockGuard
{
  public:
    RuntimeMockGuard(impl::RuntimeMock& runtime_mock) noexcept : runtime_mock_{runtime_mock}
    {
        impl::Runtime::InjectMock(&runtime_mock_);
    }

    ~RuntimeMockGuard() { impl::Runtime::InjectMock(nullptr); }

    impl::RuntimeMock& runtime_mock_;
};

class ProxyBindingFactoryRealMemoryFixture : public ::testing::Test
{
  public:
    void SetUp() override
    {
        bmw::mw::com::impl::Runtime::InjectMock(&runtime_mock_);
        EXPECT_CALL(runtime_mock_, GetBindingRuntime(BindingType::kLoLa)).WillRepeatedly(Return(&lola_runtime_mock_));
    }

    test::DummyInstanceIdentifierBuilder instance_identifier_builder_;

  protected:
    ProxyBindingFactoryMock proxy_binding_factory_mock_{};
    impl::RuntimeMock runtime_mock_{};
    RuntimeMockGuard runtime_mock_guard_{runtime_mock_};
    lola::RuntimeMock lola_runtime_mock_{};
    os::MockGuard<os::FcntlMock> fcntl_mock_{};
};

class SkeletonBindingGuard
{
  public:
    SkeletonBindingGuard(std::unique_ptr<SkeletonBinding> skeleton_binding)
        : skeleton_binding_{std::move(skeleton_binding)}
    {
    }
    ~SkeletonBindingGuard() { skeleton_binding_->PrepareStopOffer({}); }
    SkeletonBinding& GetSkeletonBinding() noexcept { return *skeleton_binding_; }

  private:
    std::unique_ptr<SkeletonBinding> skeleton_binding_;
};

using ProxyBindingFactoryCreateFixture = lola::ProxyMockedMemoryFixture;
amp::optional<LolaServiceInstanceId> GetInstanceId(bmw::mw::com::impl::InstanceIdentifier identifier)
{
    const InstanceIdentifierView identifier_view{identifier};
    const auto* const instance_deployment =
        amp::get_if<LolaServiceInstanceDeployment>(&identifier_view.GetServiceInstanceDeployment().bindingInfo_);
    if (instance_deployment == nullptr)
    {
        return amp::nullopt;
    }
    return instance_deployment->instance_id_;
}

TEST_F(ProxyBindingFactoryCreateFixture, CanCreateLoLaProxy)
{
    RecordProperty("Verifies", "1, 2, ");
    RecordProperty("Description", "Checks whether a proxy event lola binding can be created and set at runtime");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("Priority", "1");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    // Given a valid LoLa instance with an offered skeleton
    test::DummyInstanceIdentifierBuilder instance_identifier_builder{};
    auto identifier = instance_identifier_builder.CreateValidLolaInstanceIdentifier();
    auto instance_id = GetInstanceId(identifier);
    ASSERT_TRUE(instance_id.has_value());
    auto handle = make_HandleType(identifier, ServiceInstanceId{instance_id.value()});

    ON_CALL(service_discovery_mock_, StartFindService(_, EnrichedInstanceIdentifier{handle}))
        .WillByDefault(Return(make_FindServiceHandle(10U)));

    // When creating a proxy with that
    const auto result = ProxyBindingFactory::Create(handle);

    // Then no nullptr is returned
    EXPECT_NE(result, nullptr);
}

TEST_F(ProxyBindingFactoryCreateFixture, MissingLoLaTypeDeployment)
{
    RecordProperty("Verifies", "1, 2, ");
    RecordProperty("Description",
                   "Checks whether no proxy event lola binding can be created and set at runtime if lola type "
                   "deployment is missing");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("Priority", "1");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    // Given a LoLa binding with missing type deployment
    test::DummyInstanceIdentifierBuilder instance_identifier_builder{};
    auto identifier = instance_identifier_builder.CreateLolaInstanceIdentifierWithoutTypeDeployment();
    auto handle = make_HandleType(identifier, ServiceInstanceId{LolaServiceInstanceId{1}});

    ON_CALL(service_discovery_mock_, StartFindService(_, EnrichedInstanceIdentifier{handle}))
        .WillByDefault(Return(make_FindServiceHandle(10U)));

    // When creating a proxy with that
    const auto result = ProxyBindingFactory::Create(handle);

    // Then a nullptr is returned
    EXPECT_EQ(result, nullptr);
}

TEST_F(ProxyBindingFactoryCreateFixture, CannotCreateBlank)
{
    RecordProperty("Verifies", "1, 2, ");
    RecordProperty("Description", "Checks whether a proxy event blank binding can be created and set at runtime");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("Priority", "1");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    // Given an invalid handle
    test::DummyInstanceIdentifierBuilder instance_identifier_builder{};
    auto identifier = instance_identifier_builder.CreateBlankBindingInstanceIdentifier();
    auto handle = make_HandleType(identifier, ServiceInstanceId{LolaServiceInstanceId{1}});

    ON_CALL(service_discovery_mock_, StartFindService(_, EnrichedInstanceIdentifier{handle}))
        .WillByDefault(Return(make_FindServiceHandle(10U)));

    // When creating a proxy with that
    const auto result = ProxyBindingFactory::Create(handle);

    // Then a nullptr is returned
    EXPECT_EQ(result, nullptr);
}

TEST_F(ProxyBindingFactoryCreateFixture, CannotCreateSomeIpBinding)
{
    // Given a SomeIp binding
    test::DummyInstanceIdentifierBuilder instance_identifier_builder{};
    auto identifier = instance_identifier_builder.CreateSomeIpBindingInstanceIdentifier();
    auto handle = make_HandleType(identifier, ServiceInstanceId{LolaServiceInstanceId{1}});

    ON_CALL(service_discovery_mock_, StartFindService(_, EnrichedInstanceIdentifier{handle}))
        .WillByDefault(Return(make_FindServiceHandle(10U)));

    // When creating a proxy with that
    const auto result = ProxyBindingFactory::Create(handle);

    // Then a nullptr is returned
    EXPECT_EQ(result, nullptr);
}

}  // namespace
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
