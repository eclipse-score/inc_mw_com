/********************************************************************************
* Copyright (c) 2025 Contributors to the Eclipse Foundation
*
* See the NOTICE file(s) distributed with this work for additional
* information regarding copyright ownership.
*
* This program and the accompanying materials are made available under the
* terms of the Apache License Version 2.0 which is available at
* https://www.apache.org/licenses/LICENSE-2.0
*
* SPDX-License-Identifier: Apache-2.0
********************************************************************************/


#include "platform/aas/mw/com/impl/service_discovery.h"

#include "platform/aas/mw/com/impl/bindings/lola/runtime_mock.h"
#include "platform/aas/mw/com/impl/runtime_mock.h"
#include "platform/aas/mw/com/impl/service_discovery_client_mock.h"

#include <amp_optional.hpp>
#include <amp_utility.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>
#include <future>

namespace bmw::mw::com::impl
{

using ::testing::_;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::SaveArg;

class DestructorNotifier
{
  public:
    explicit DestructorNotifier(std::promise<void>* handler_destruction_barrier) noexcept
        : handler_destruction_barrier_{handler_destruction_barrier}
    {
    }

    ~DestructorNotifier() noexcept
    {
        if (handler_destruction_barrier_ != nullptr)
        {
            handler_destruction_barrier_->set_value();
        }
    }

    DestructorNotifier(DestructorNotifier&& other) noexcept
        : handler_destruction_barrier_{other.handler_destruction_barrier_}
    {
        other.handler_destruction_barrier_ = nullptr;
    }

    DestructorNotifier(const DestructorNotifier&) = delete;
    DestructorNotifier& operator=(const DestructorNotifier&) = delete;
    DestructorNotifier& operator=(DestructorNotifier&&) = delete;

  private:
    std::promise<void>* handler_destruction_barrier_;
};

class ServiceDiscoveryTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        ON_CALL(runtime_, GetBindingRuntime(::testing::_)).WillByDefault(::testing::Return(&lola_runtime_));
        ON_CALL(runtime_, resolve(instance_specifier_))
            .WillByDefault(
                ::testing::Return(std::vector<InstanceIdentifier>{instance_identifier_1_, instance_identifier_2_}));

        ON_CALL(lola_runtime_, GetServiceDiscoveryClient())
            .WillByDefault(::testing::ReturnRef(service_discovery_client_));

        ON_CALL(service_discovery_client_, StartFindService(::testing::_, ::testing::_, ::testing::_))
            .WillByDefault(::testing::Return(ResultBlank{}));

        ON_CALL(service_discovery_client_, StopFindService(::testing::_))
            .WillByDefault(::testing::Return(ResultBlank{}));
    }

    RuntimeMock runtime_{};
    lola::RuntimeMock lola_runtime_{};
    ServiceDiscoveryClientMock service_discovery_client_{};

    InstanceSpecifier instance_specifier_{InstanceSpecifier::Create("/bla/blub/specifier").value()};

    ServiceInstanceDeployment instance_deployment_1_{
        make_ServiceIdentifierType("/bla/blub/service1"),
        LolaServiceInstanceDeployment{bmw::mw::com::impl::LolaServiceInstanceId{1U}},
        QualityType::kASIL_QM,
        instance_specifier_};
    ServiceTypeDeployment service_type_deployment_1_{amp::blank{}};
    InstanceIdentifier instance_identifier_1_{
        make_InstanceIdentifier(instance_deployment_1_, service_type_deployment_1_)};
    EnrichedInstanceIdentifier enriched_instance_identifier_1_{instance_identifier_1_};

    ServiceInstanceDeployment instance_deployment_2_{
        make_ServiceIdentifierType("/bla/blub/service2"),
        LolaServiceInstanceDeployment{bmw::mw::com::impl::LolaServiceInstanceId{1U}},
        QualityType::kASIL_QM,
        instance_specifier_};
    ServiceTypeDeployment service_type_deployment_2_{amp::blank{}};
    InstanceIdentifier instance_identifier_2_{
        make_InstanceIdentifier(instance_deployment_2_, service_type_deployment_2_)};
    EnrichedInstanceIdentifier enriched_instance_identifier_2_{instance_identifier_2_};
};

using ServiceDiscoveryFindServiceFixture = ServiceDiscoveryTest;
TEST_F(ServiceDiscoveryFindServiceFixture,
       FindServiceForInstanceSpecifierCallsBindingSpecificFindServiceForEachIdentifer)
{
    RecordProperty("ParentRequirement", "7, 0, 2");
    RecordProperty("Description", "FindService can find a service using an instance specifier.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    // Given a Service Discovery, where one service instance will be found
    ServiceDiscovery unit{runtime_};

    auto expected_handle = make_HandleType(instance_identifier_1_, bmw::mw::com::impl::LolaServiceInstanceId{1U});

    EXPECT_CALL(service_discovery_client_, FindService(enriched_instance_identifier_1_))
        .WillOnce(::testing::Return(ServiceHandleContainer<HandleType>{expected_handle}));
    EXPECT_CALL(service_discovery_client_, FindService(enriched_instance_identifier_2_))
        .WillOnce(::testing::Return(ServiceHandleContainer<HandleType>{}));

    // When finding for this service instance via an instance specifier
    auto handles_result = unit.FindService(instance_specifier_);

    // Then the service instance is found
    ASSERT_TRUE(handles_result.has_value());
    EXPECT_EQ(handles_result.value().size(), 1);
}

TEST_F(ServiceDiscoveryFindServiceFixture, FindServiceForInstanceIdentiierCallsBindingSpecificFindService)
{
    RecordProperty("ParentRequirement", "7, 0, 2");
    RecordProperty("Description", "FindService can find a service using an instance identifier.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    // Given a Service Discovery, where one service instance will be found
    ServiceDiscovery unit{runtime_};

    auto expected_handle = make_HandleType(instance_identifier_1_, bmw::mw::com::impl::LolaServiceInstanceId{1U});

    EXPECT_CALL(service_discovery_client_, FindService(enriched_instance_identifier_1_))
        .WillOnce(::testing::Return(ServiceHandleContainer<HandleType>{expected_handle}));

    // When finding for this service instance via an instance identifier
    auto handles_result = unit.FindService(instance_identifier_1_);

    // Then the service instance is found
    ASSERT_TRUE(handles_result.has_value());
    EXPECT_EQ(handles_result.value().size(), 1);
}

TEST_F(ServiceDiscoveryFindServiceFixture, FindServiceShouldReturnEmptyHandlesContainerIfBindingFindsNoInstances)
{
    RecordProperty("ParentRequirement", "7, 0, 2");
    RecordProperty("Description",
                   "FindService returns an empty handles containing if binding cannot find any instances.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    // Given a Service Discovery, where no service instance is found
    ServiceDiscovery unit{runtime_};

    EXPECT_CALL(service_discovery_client_, FindService(enriched_instance_identifier_1_))
        .WillOnce(::testing::Return(ServiceHandleContainer<HandleType>{}));
    EXPECT_CALL(service_discovery_client_, FindService(enriched_instance_identifier_2_))
        .WillOnce(::testing::Return(ServiceHandleContainer<HandleType>{}));

    // When finding for this service instance via an instance specifier
    auto handles_result = unit.FindService(instance_specifier_);

    // Then nothing is found
    ASSERT_TRUE(handles_result.has_value());
    EXPECT_EQ(handles_result.value().size(), 0);
}

TEST_F(ServiceDiscoveryFindServiceFixture, FindServiceShouldReturnErrorIfBindingReturnsError)
{
    RecordProperty("ParentRequirement", "7, 0, 2");
    RecordProperty("Description", "FindService returns a kBindingFailure error code if binding returns any error.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    const auto binding_error_code = ComErrc::kErroneousFileHandle;
    const auto returned_error_code = ComErrc::kBindingFailure;

    // Given that errors are returned for all bindings, while searching for a service instance
    ServiceDiscovery unit{runtime_};

    EXPECT_CALL(service_discovery_client_, FindService(enriched_instance_identifier_1_))
        .WillOnce(::testing::Return(Unexpected{binding_error_code}));
    EXPECT_CALL(service_discovery_client_, FindService(enriched_instance_identifier_2_))
        .WillOnce(::testing::Return(Unexpected{binding_error_code}));
    ;

    // When finding a specific service
    auto handles_result = unit.FindService(instance_specifier_);

    // Then an error is returned
    ASSERT_FALSE(handles_result.has_value());

    // And the error code is as expected
    EXPECT_EQ(handles_result.error(), returned_error_code);
}

using ServiceDiscoveryFindServiceTestDeathTest = ServiceDiscoveryTest;

TEST_F(ServiceDiscoveryFindServiceTestDeathTest, FindServiceForInstanceSpecifierFailsResolution)
{
    RecordProperty("ParentRequirement", "7, 0, 2");
    RecordProperty("Description", "FindService dies, if InstanceSpecifier cannot be resolved.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    InstanceSpecifier unknown_instance_specifier{InstanceSpecifier::Create("/not/existing/specifier").value()};

    // Given a Service Discovery,
    ServiceDiscovery unit{runtime_};

    // When finding for this service instance via an instance specifier that does not exist
    // Then the application crashes
    EXPECT_DEATH(amp::ignore = unit.FindService(unknown_instance_specifier).has_value(), ".*");
}

using ServiceDiscoveryStartFindServiceInstanceSpecifierFixture = ServiceDiscoveryTest;
TEST_F(ServiceDiscoveryStartFindServiceInstanceSpecifierFixture,
       StartFindServiceCallsBindingSpecificStartFindServiceForEachIdentifer)
{
    // @todo: Right now this is only testing the Lola binding, no other. Can be extended in future.
    RecordProperty("ParentRequirement", "2, 5");
    RecordProperty("Description", "All instance identifiers specific to a Instance specifier are called.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");
    ServiceDiscovery unit{runtime_};

    EXPECT_CALL(service_discovery_client_,
                StartFindService(::testing::_, ::testing::_, enriched_instance_identifier_1_));
    EXPECT_CALL(service_discovery_client_,
                StartFindService(::testing::_, ::testing::_, enriched_instance_identifier_2_));

    unit.StartFindService([](auto, auto) noexcept {}, instance_specifier_);
}

TEST_F(ServiceDiscoveryStartFindServiceInstanceSpecifierFixture, StartFindServiceReturnsHandleIfSuccessful)
{
    ServiceDiscovery unit{runtime_};

    auto handle = unit.StartFindService([](auto, auto) noexcept {}, instance_specifier_);
    EXPECT_TRUE(handle.has_value());
}

TEST_F(ServiceDiscoveryStartFindServiceInstanceSpecifierFixture,
       StartFindServiceCallsBindingSpecificStopFindServiceOnFailure)
{
    ServiceDiscovery unit{runtime_};

    ::testing::InSequence seq{};
    FindServiceHandle handle{make_FindServiceHandle(0)};
    EXPECT_CALL(service_discovery_client_,
                StartFindService(::testing::_, ::testing::_, enriched_instance_identifier_1_))
        .WillOnce(
            ::testing::DoAll(::testing::SaveArg<0>(&handle), ::testing::Return(Unexpected{ComErrc::kBindingFailure})));
    EXPECT_CALL(service_discovery_client_,
                StartFindService(::testing::_, ::testing::_, enriched_instance_identifier_2_))
        .Times(0);
    EXPECT_CALL(service_discovery_client_, StopFindService(::testing::Eq(::testing::ByRef(handle))));
    EXPECT_CALL(service_discovery_client_, StopFindService(::testing::_)).Times(0);

    unit.StartFindService([](auto, auto) noexcept {}, instance_specifier_);
}

TEST_F(ServiceDiscoveryStartFindServiceInstanceSpecifierFixture, StartFindServiceForwardsCorrectHandler)
{
    ServiceDiscovery unit{runtime_};

    ON_CALL(service_discovery_client_, StartFindService(::testing::_, ::testing::_, ::testing::_))
        .WillByDefault([](auto handle, auto handler, auto) {
            handler({}, handle);
            return ResultBlank{};
        });

    std::int32_t value{0};
    unit.StartFindService([&value](auto, auto) noexcept { value++; }, instance_specifier_);

    EXPECT_EQ(value, 2);
}

TEST_F(ServiceDiscoveryStartFindServiceInstanceSpecifierFixture, StartFindServiceReturnsWorkingHandle)
{
    ServiceDiscovery unit{runtime_};

    FindServiceHandle handle_1{make_FindServiceHandle(0)};
    EXPECT_CALL(service_discovery_client_,
                StartFindService(::testing::_, ::testing::_, enriched_instance_identifier_1_))
        .WillOnce(::testing::DoAll(::testing::SaveArg<0>(&handle_1), ::testing::Return(ResultBlank{})));
    EXPECT_CALL(
        service_discovery_client_,
        StartFindService(::testing::Eq(::testing::ByRef(handle_1)), ::testing::_, enriched_instance_identifier_2_))
        .WillOnce(::testing::Return(ResultBlank{}));
    EXPECT_CALL(service_discovery_client_, StopFindService(::testing::Eq(::testing::ByRef(handle_1)))).Times(2);

    auto start_result = unit.StartFindService([](auto, auto) noexcept {}, instance_specifier_);
    ASSERT_TRUE(start_result.has_value());
    auto stop_result = unit.StopFindService(start_result.value());
    EXPECT_TRUE(stop_result.has_value());
}

TEST_F(ServiceDiscoveryStartFindServiceInstanceSpecifierFixture,
       StartFindServiceWillUseTheSameFindServiceHandleForAllFoundInstanceIdentifiers)
{
    // Given a ServiceDiscovery with a mocked ServiceDiscoveryClient
    ServiceDiscovery unit{runtime_};

    // Expecting that StartFindService will be called on the binding for each InstanceIdentifier
    amp::optional<FindServiceHandle> find_service_handle_1{};
    amp::optional<FindServiceHandle> find_service_handle_2{};
    ON_CALL(service_discovery_client_, StartFindService(_, _, enriched_instance_identifier_1_))
        .WillByDefault(DoAll(SaveArg<0>(&find_service_handle_1), Return(ResultBlank{})));
    ON_CALL(service_discovery_client_, StartFindService(_, _, enriched_instance_identifier_2_))
        .WillByDefault(DoAll(SaveArg<0>(&find_service_handle_2), Return(ResultBlank{})));

    // When calling StartFindService with an InstanceSpecifier and a handler
    auto returned_find_service_handle = unit.StartFindService([](auto, auto) noexcept {}, instance_specifier_);

    // Then the same handler should be used for each call to StartFindService on the binding
    ASSERT_TRUE(find_service_handle_1.has_value());
    ASSERT_TRUE(find_service_handle_2.has_value());
    EXPECT_EQ(find_service_handle_1.value(), find_service_handle_2.value());
    EXPECT_EQ(find_service_handle_1.value(), returned_find_service_handle.value());
}

TEST_F(ServiceDiscoveryStartFindServiceInstanceSpecifierFixture,
       StartFindServiceWillStoreRegisteredReceiveHandlerWithGeneratedHandle)
{
    RecordProperty("ParentRequirement", "6");
    RecordProperty("Description",
                   "Checks that the registered FindServiceHandler is stored when StartFindService with an "
                   "InstanceSpecifier is called. Since the handler is move-only, it will only be stored in one place.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    std::promise<void> handler_destruction_barrier{};

    // Given a ServiceDiscovery with a mocked ServiceDiscoveryClient
    ServiceDiscovery unit{runtime_};

    // and a FindServiceHandler which contains a DestructorNotifier object which will set the value of the
    // handler_destruction_barrier promise on destruction
    DestructorNotifier destructor_notifier{&handler_destruction_barrier};
    auto find_service_handler = [destructor_notifier = std::move(destructor_notifier)](auto, auto) noexcept {};

    // When calling StartFindService with an InstanceSpecifier and the handler
    amp::ignore = unit.StartFindService(std::move(find_service_handler), instance_specifier_);

    // Then the handler should not have been destroyed by StartFindService, indicating that the handler has been stored
    // internally (since it's move-only)
    const auto future_status = handler_destruction_barrier.get_future().wait_for(std::chrono::milliseconds{1U});
    EXPECT_EQ(future_status, std::future_status::timeout);
}

using ServiceDiscoveryStartFindServiceInstanceIdentifierFixture = ServiceDiscoveryTest;
TEST_F(ServiceDiscoveryStartFindServiceInstanceIdentifierFixture, StartFindServiceCallsBindingSpecificStartFindService)
{
    ServiceDiscovery unit{runtime_};

    EXPECT_CALL(service_discovery_client_,
                StartFindService(::testing::_, ::testing::_, enriched_instance_identifier_1_));

    unit.StartFindService([](auto, auto) noexcept {}, instance_identifier_1_);
}

TEST_F(ServiceDiscoveryStartFindServiceInstanceIdentifierFixture, StartFindServiceReturnsHandleIfSuccessful)
{
    ServiceDiscovery unit{runtime_};

    auto handle = unit.StartFindService([](auto, auto) noexcept {}, instance_identifier_1_);
    EXPECT_TRUE(handle.has_value());
}

TEST_F(ServiceDiscoveryStartFindServiceInstanceIdentifierFixture,
       StartFindServiceCallsStopFindServiceIfBindingSpecificStartFindServiceFailed)
{
    ServiceDiscovery unit{runtime_};

    ::testing::InSequence seq{};
    FindServiceHandle handle_1{make_FindServiceHandle(0)};
    EXPECT_CALL(service_discovery_client_,
                StartFindService(::testing::_, ::testing::_, enriched_instance_identifier_1_))
        .WillOnce(::testing::DoAll(::testing::SaveArg<0>(&handle_1),
                                   ::testing::Return(Unexpected{ComErrc::kBindingFailure})));
    EXPECT_CALL(service_discovery_client_, StopFindService(::testing::Eq(::testing::ByRef(handle_1))));

    auto error = unit.StartFindService([](auto, auto) noexcept {}, instance_identifier_1_);
    ASSERT_FALSE(error.has_value());
    EXPECT_EQ(error.error(), ComErrc::kBindingFailure);
}

TEST_F(ServiceDiscoveryStartFindServiceInstanceIdentifierFixture, StartFindServiceForwardsCorrectHandler)
{
    ServiceDiscovery unit{runtime_};

    ON_CALL(service_discovery_client_, StartFindService(::testing::_, ::testing::_, enriched_instance_identifier_1_))
        .WillByDefault([](auto handle, auto handler, auto) {
            handler({}, handle);
            return ResultBlank{};
        });

    bool value{false};
    unit.StartFindService([&value](auto, auto) noexcept { value = true; }, instance_identifier_1_);

    EXPECT_TRUE(value);
}

TEST_F(ServiceDiscoveryStartFindServiceInstanceIdentifierFixture, StartFindServiceReturnsWorkingHandle)
{
    ServiceDiscovery unit{runtime_};

    FindServiceHandle handle_1{make_FindServiceHandle(0)};
    EXPECT_CALL(service_discovery_client_,
                StartFindService(::testing::_, ::testing::_, enriched_instance_identifier_1_))
        .WillOnce(::testing::DoAll(::testing::SaveArg<0>(&handle_1), ::testing::Return(ResultBlank{})));
    EXPECT_CALL(service_discovery_client_, StopFindService(::testing::Eq(::testing::ByRef(handle_1))));

    auto start_result = unit.StartFindService([](auto, auto) noexcept {}, instance_identifier_1_);
    ASSERT_TRUE(start_result.has_value());
    auto stop_result = unit.StopFindService(start_result.value());
    EXPECT_TRUE(stop_result.has_value());
}

TEST_F(ServiceDiscoveryStartFindServiceInstanceIdentifierFixture,
       StartFindServiceWillStoreRegisteredReceiveHandlerWithGeneratedHandle)
{
    RecordProperty("ParentRequirement", "6");
    RecordProperty(
        "Description",
        "Checks that the registered FindServiceHandler is stored when StartFindService with an "
        "InstanceIdentifier is called. Since the handler is move-only, it will only be stored in one place.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    std::promise<void> handler_destruction_barrier{};

    // Given a ServiceDiscovery with a mocked ServiceDiscoveryClient
    ServiceDiscovery unit{runtime_};

    // and a FindServiceHandler which contains a DestructorNotifier object which will set the value of the
    // handler_destruction_barrier promise on destruction
    DestructorNotifier destructor_notifier{&handler_destruction_barrier};
    auto find_service_handler = [destructor_notifier = std::move(destructor_notifier)](auto, auto) noexcept {};

    // When calling StartFindService with an InstanceSpecifier and the handler
    amp::ignore = unit.StartFindService(std::move(find_service_handler), instance_identifier_1_);

    // Then the handler should not have been destroyed by StartFindService, indicating that the handler has been stored
    // internally (since it's move-only)
    const auto future_status = handler_destruction_barrier.get_future().wait_for(std::chrono::milliseconds{1U});
    EXPECT_EQ(future_status, std::future_status::timeout);
}

using ServiceDiscoveryStopFindServiceFixture = ServiceDiscoveryTest;
TEST_F(ServiceDiscoveryStopFindServiceFixture, StopFindServiceInvokedIfForgottenByUser)
{
    ServiceDiscovery unit{runtime_};

    FindServiceHandle handle_1{make_FindServiceHandle(0)};
    EXPECT_CALL(service_discovery_client_,
                StartFindService(::testing::_, ::testing::_, enriched_instance_identifier_1_))
        .WillOnce(::testing::DoAll(::testing::SaveArg<0>(&handle_1), ::testing::Return(ResultBlank{})));
    EXPECT_CALL(
        service_discovery_client_,
        StartFindService(::testing::Eq(::testing::ByRef(handle_1)), ::testing::_, enriched_instance_identifier_2_))
        .WillOnce(::testing::Return(ResultBlank{}));
    EXPECT_CALL(service_discovery_client_, StopFindService(::testing::Eq(::testing::ByRef(handle_1)))).Times(2);

    auto start_result = unit.StartFindService([](auto, auto) noexcept {}, instance_specifier_);
    ASSERT_TRUE(start_result.has_value());
}

TEST_F(ServiceDiscoveryStopFindServiceFixture,
       StopFindServiceCallsBindingSpecificStopFindServiceForAllAssociatedInstanceIdentifiers)
{
    ServiceDiscovery unit{runtime_};

    FindServiceHandle handle_1{make_FindServiceHandle(0)};
    EXPECT_CALL(service_discovery_client_,
                StartFindService(::testing::_, ::testing::_, enriched_instance_identifier_1_))
        .WillOnce(::testing::DoAll(::testing::SaveArg<0>(&handle_1), ::testing::Return(ResultBlank{})));
    EXPECT_CALL(
        service_discovery_client_,
        StartFindService(::testing::Eq(::testing::ByRef(handle_1)), ::testing::_, enriched_instance_identifier_2_))
        .WillOnce(::testing::Return(ResultBlank{}));
    EXPECT_CALL(service_discovery_client_, StopFindService(::testing::Eq(::testing::ByRef(handle_1)))).Times(2);

    auto start_result = unit.StartFindService([](auto, auto) noexcept {}, instance_specifier_);
    ASSERT_TRUE(start_result.has_value());
    auto stop_result = unit.StopFindService(start_result.value());
    EXPECT_TRUE(stop_result.has_value());
}

TEST_F(ServiceDiscoveryStopFindServiceFixture, StopFindServiceCallsBindingSpecificStopFindServiceEvenWhenOneFailed)
{
    ServiceDiscovery unit{runtime_};

    FindServiceHandle handle_1{make_FindServiceHandle(0)};
    EXPECT_CALL(service_discovery_client_,
                StartFindService(::testing::_, ::testing::_, enriched_instance_identifier_1_))
        .WillOnce(::testing::DoAll(::testing::SaveArg<0>(&handle_1), ::testing::Return(ResultBlank{})));
    EXPECT_CALL(service_discovery_client_, StartFindService(handle_1, ::testing::_, enriched_instance_identifier_2_))
        .WillOnce(::testing::Return(ResultBlank{}));
    EXPECT_CALL(service_discovery_client_, StopFindService(::testing::Eq(::testing::ByRef(handle_1))))
        .Times(2)
        .WillOnce(::testing::Return(Unexpected{ComErrc::kBindingFailure}));

    auto start_result = unit.StartFindService([](auto, auto) noexcept {}, instance_specifier_);
    ASSERT_TRUE(start_result.has_value());
    auto stop_result = unit.StopFindService(start_result.value());
    ASSERT_FALSE(stop_result.has_value());
    EXPECT_EQ(stop_result.error(), ComErrc::kBindingFailure);
}

TEST_F(ServiceDiscoveryStopFindServiceFixture, StopFindServiceWillDestroyRegisteredFindServiceHandler)
{
    RecordProperty("ParentRequirement", "6");
    RecordProperty(
        "Description",
        "Checks that the registered FindServiceHandler is destroyed when StopFindService is called. Since the "
        "handler is move-only, it will only have been stored in one place.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    std::promise<void> handler_destruction_barrier{};

    // Given a ServiceDiscovery with a mocked ServiceDiscoveryClient
    ServiceDiscovery unit{runtime_};

    // and a FindServiceHandler which contains a DestructorNotifier object which will set the value of the
    // handler_destruction_barrier promise on destruction
    DestructorNotifier destructor_notifier{&handler_destruction_barrier};
    auto find_service_handler = [destructor_notifier = std::move(destructor_notifier)](auto, auto) noexcept {};

    // and given that StartFindService has been called with an InstanceSpecifier and the handler
    const auto find_service_handle = unit.StartFindService(std::move(find_service_handler), instance_specifier_);

    // When calling StopFindService with the returned handle
    amp::ignore = unit.StopFindService(find_service_handle.value());

    // Then the handler passed to StartFindService should be destroyed
    handler_destruction_barrier.get_future().wait();
}

}  // namespace bmw::mw::com::impl
