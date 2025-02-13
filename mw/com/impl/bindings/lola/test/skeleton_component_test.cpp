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


#include "platform/aas/mw/com/impl/bindings/lola/skeleton.h"

#include "platform/aas/lib/filesystem/factory/filesystem_factory_fake.h"
#include "platform/aas/lib/os/mman.h"
#include "platform/aas/lib/os/mocklib/acl_mock.h"
#include "platform/aas/mw/com/impl/bindings/lola/test/skeleton_test_resources.h"
#include "platform/aas/mw/com/impl/bindings/mock_binding/skeleton_event.h"
#include "platform/aas/mw/com/impl/runtime.h"

#include <gtest/gtest.h>

#include <sys/stat.h>
#include <unistd.h>
#include <string>
#include <string_view>
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

#if defined(__QNXNTO__)
constexpr auto data_shm = "/dev/shmem/lola-data-0000000000000001-00016";
constexpr auto control_shm = "/dev/shmem/lola-ctl-0000000000000001-00016";
constexpr auto asil_control_shm = "/dev/shmem/lola-ctl-0000000000000001-00016-b";
#else
constexpr auto data_shm = "/dev/shm/lola-data-0000000000000001-00016";
constexpr auto control_shm = "/dev/shm/lola-ctl-0000000000000001-00016";
constexpr auto asil_control_shm = "/dev/shm/lola-ctl-0000000000000001-00016-b";
#endif

const auto kInstanceSpecifier = InstanceSpecifier::Create("abc/abc/TirePressurePort").value();

std::size_t GetSize(const std::string& file_path)
{
    struct stat data
    {
    };
    const auto result = stat(file_path.c_str(), &data);
    if (result == 0 && data.st_size > 0)
    {
        return static_cast<std::size_t>(data.st_size);
    }

    return 0;
}

bool IsWriteableForOwner(const std::string& filePath)
{
    struct stat data
    {
    };
    const auto result = stat(filePath.c_str(), &data);
    if (result == 0)
    {
        return data.st_mode & S_IWUSR;
    }

    std::cerr << "File does not exist" << std::endl;
    std::abort();
}

bool IsWriteableForOthers(const std::string& filePath)
{
    struct stat data
    {
    };
    const auto result = stat(filePath.c_str(), &data);
    if (result == 0)
    {
        bool group_write_permission = data.st_mode & S_IWGRP;
        bool others_write_permission = data.st_mode & S_IWOTH;
        return group_write_permission || others_write_permission;
    }

    std::cerr << "File does not exist" << std::endl;
    std::abort();
}

struct EventInfo
{
    std::size_t event_size;
    std::size_t max_samples;
};

std::size_t CalculateLowerBoundControlShmSize(const std::vector<EventInfo>& events)
{
    std::size_t lower_bound{sizeof(ServiceDataControl)};
    for (const auto event_info : events)
    {
        lower_bound += sizeof(decltype(ServiceDataControl::event_controls_)::value_type);
        lower_bound += event_info.max_samples * sizeof(EventDataControl::EventControlSlots::value_type);
    }
    return lower_bound;
}

std::size_t CalculateLowerBoundDataShmSize(const std::vector<EventInfo>& events)
{
    std::size_t lower_bound{sizeof(ServiceDataStorage)};
    for (const auto event_info : events)
    {
        lower_bound += sizeof(decltype(ServiceDataStorage::events_)::value_type);
        lower_bound += event_info.max_samples * event_info.event_size;
        lower_bound += sizeof(decltype(ServiceDataStorage::events_metainfo_)::value_type);
    }
    return lower_bound;
}

using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::StrEq;

/// \brief Test fixture for lola::Skeleton tests, which are generally based on "real" shared-mem access.
class SkeletonComponentTestFixture : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        impl::Runtime::InjectMock(&runtime_mock_);
        EXPECT_CALL(runtime_mock_, GetBindingRuntime(BindingType::kLoLa))
            .WillRepeatedly(::testing::Return(&lola_runtime_mock_));
    }

    void TearDown() override
    {
        bmw::memory::shared::MemoryResourceRegistry::getInstance().clear();
        bmw::filesystem::IStandardFilesystem::instance().Remove("/tmp/lola-data-0000000000000001-00016_lock");
        bmw::filesystem::IStandardFilesystem::instance().Remove("/tmp/lola-ctl-0000000000000001-00016_lock");
        bmw::filesystem::IStandardFilesystem::instance().Remove("/tmp/lola-ctl-0000000000000001-00016-b_lock");

        bmw::filesystem::IStandardFilesystem::instance().Remove(data_shm);
        bmw::filesystem::IStandardFilesystem::instance().Remove(control_shm);
        bmw::filesystem::IStandardFilesystem::instance().Remove(asil_control_shm);

        EXPECT_FALSE(fileExists(data_shm));
        EXPECT_FALSE(fileExists(control_shm));
        EXPECT_FALSE(fileExists(asil_control_shm));

        bmw::memory::shared::MemoryResourceRegistry::getInstance().clear();
        impl::Runtime::InjectMock(nullptr);
    }

    std::unique_ptr<Skeleton> CreateSkeleton(
        const InstanceIdentifier& instance_identifier,
        bmw::filesystem::Filesystem filesystem = filesystem::FilesystemFactory{}.CreateInstance()) noexcept
    {
        auto shm_path_builder = std::make_unique<ShmPathBuilder>(test::kLolaServiceId);
        auto partial_restart_path_builder = std::make_unique<PartialRestartPathBuilder>(test::kLolaServiceId);

        auto unit = Skeleton::Create(
            instance_identifier, filesystem, std::move(shm_path_builder), std::move(partial_restart_path_builder));
        return unit;
    }

    /// mocks used by test
    impl::RuntimeMock runtime_mock_{};
    lola::RuntimeMock lola_runtime_mock_{};
};

TEST_F(SkeletonComponentTestFixture, ACLPermissionsSetCorrectly)
{
    RecordProperty("Verifies", "");
    RecordProperty("Description", "Ensure that the correct ACLs are set that are configured.");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("Priority", "1");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    // Given a valid instance identifier and constructed unit
    const auto instance_identifier = GetValidASILInstanceIdentifierWithACL();

    // from which we create our UoT
    auto unit = CreateSkeleton(instance_identifier);
    ASSERT_NE(unit, nullptr);

    // Expecting that the ACL Levels are set correctly for the QM and ASIL split segments
    bmw::os::MockGuard<bmw::os::AclMock> acl_mock{};

    EXPECT_CALL(*acl_mock, acl_add_perm(_, bmw::os::Acl::Permission::kRead)).Times(4);
    EXPECT_CALL(*acl_mock, acl_add_perm(_, bmw::os::Acl::Permission::kWrite)).Times(2);
    EXPECT_CALL(*acl_mock,
                acl_set_qualifier(_,
                                  ::testing::MatcherCast<const void*>(::testing::SafeMatcherCast<const uint32_t*>(
                                      ::testing::Pointee(::testing::Eq(42))))))
        .Times(3);
    EXPECT_CALL(*acl_mock,
                acl_set_qualifier(_,
                                  ::testing::MatcherCast<const void*>(::testing::SafeMatcherCast<const uint32_t*>(
                                      ::testing::Pointee(::testing::Eq(43))))))
        .Times(3);
    char acl_text[] =
        "user::rw-\n"
        "user:foresightmapprovisiond:rw-\n"
        "user:aascomhandlerd:rw-\n"
        "user:senseassessmentd:rw-\n"
        "group::---\n"
        "mask::rw-\n"
        "other::---";
    EXPECT_CALL(*acl_mock, acl_to_text(_, _))
        .WillRepeatedly(Invoke(
            [&acl_text](const ::bmw::os::Acl::AclCollection&, ssize_t* size) -> amp::expected<char*, bmw::os::Error> {
                *size = static_cast<ssize_t>(strlen(acl_text));
                return acl_text;
            }));

    // When preparing to offer a service
    SkeletonBinding::SkeletonEventBindings events{};
    SkeletonBinding::SkeletonFieldBindings fields{};
    unit->PrepareOffer(events, fields, {});
}

TEST_F(SkeletonComponentTestFixture, CannotCreateTheSameSkeletonTwice)
{
    RecordProperty("Verifies", ", ");  // , 
    RecordProperty("Description", "Tries to offer the same service twice");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("Priority", "1");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    auto filesystem = filesystem::FilesystemFactory{}.CreateInstance();

    // Given a valid instance identifier
    const auto instance_identifier = GetValidInstanceIdentifier();

    // from which we create our UoT
    auto unit = CreateSkeleton(instance_identifier, filesystem);
    ASSERT_NE(unit, nullptr);

    auto second_unit = CreateSkeleton(instance_identifier, filesystem);
    ASSERT_EQ(second_unit, nullptr);
}

/// \brief Test verifies, that the skeleton, when created from a valid InstanceIdentifier, creates the expected
/// shared-memory objects.
/// \details In this case - as the deployment contained in the valid InstanceIdentifier defines only QM - we expect one
/// data and one control shm-object for QM and NO shm-object for ASIL-B!
TEST_F(SkeletonComponentTestFixture, ShmObjectsAreCreated)
{
    // 
    RecordProperty("Verifies",
                   ", , , , , , , "
                   ", 2908703");
    RecordProperty("Description",
                   "Ensure that QM Control segment and Data segment are created. Maximum memory allocation is "
                   "configured on runtime and allocated on offer. Thus, it is ensured that "
                   "enough resources are available after subscribe.");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("Priority", "1");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    bmw::os::Mman::restore_instance();

    // Given a valid instance identifier of an QM only instance
    const auto instance_identifier = GetValidInstanceIdentifier();

    // from which we create our UoT
    auto unit = CreateSkeleton(instance_identifier);
    ASSERT_NE(unit, nullptr);

    // When offering the service
    SkeletonBinding::SkeletonEventBindings events{};
    SkeletonBinding::SkeletonFieldBindings fields{};
    auto result = unit->PrepareOffer(events, fields, {});

    // Then this PrepareOffer succeeds
    EXPECT_TRUE(result.has_value());

    // Then the respective Shared Memory file for data is created
    EXPECT_TRUE(fileExists(data_shm));
    EXPECT_FALSE(IsWriteableForOthers(data_shm));
    EXPECT_TRUE(IsWriteableForOwner(data_shm));

    // .... and the respective Shared Memory file for QM control is created
    EXPECT_TRUE(fileExists(control_shm));
    // ... and the control shm-object is writeable for others
    // (our instance_identifier is based on a deployment without ACLs)
    EXPECT_TRUE(IsWriteableForOthers(control_shm));
    EXPECT_TRUE(IsWriteableForOwner(control_shm));

    // .... and NO Shared Memory file for control for ASIL-B is created
    EXPECT_FALSE(fileExists(asil_control_shm));

    // and we expect, that the size of the shm-data file is at least test::kConfiguredDeploymentShmSize as the
    // instance_identifier had a configured shm-size test::kConfiguredDeploymentShmSize.
    EXPECT_GT(GetSize(data_shm), test::kConfiguredDeploymentShmSize);
}

/// \brief Test verifies, that the skeleton, when created from a valid InstanceIdentifier defining an ASIL-B enabled
/// service, creates also the expected ASIL-B shared-memory object for control.
/// \details Thios test is basically an extension to the test "ShmObjectsAreCreated" above!
TEST_F(SkeletonComponentTestFixture, ASILShmIsCreated)
{
    RecordProperty("Verifies", ", , , , , 2908703");
    RecordProperty("Description", "Ensure that ASIL Control segment is created");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("Priority", "1");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    // Given a valid instance identifier
    const auto instance_identifier = GetValidASILInstanceIdentifier();

    // from which we create our UoT
    auto unit = CreateSkeleton(instance_identifier);
    ASSERT_NE(unit, nullptr);

    // When offering the service
    SkeletonBinding::SkeletonEventBindings events{};
    SkeletonBinding::SkeletonFieldBindings fields{};
    auto result = unit->PrepareOffer(events, fields, {});
    EXPECT_TRUE(result.has_value());

    // Then the respective Shared Memory file is created
    EXPECT_TRUE(fileExists(asil_control_shm));
    // ... and the control shm-object is writeable for others
    // (our instance_identifier is based on a deployment without ACLs)
    EXPECT_TRUE(IsWriteableForOthers(asil_control_shm));
}

class SkeletonComponentTestParameterisedFixture : public SkeletonComponentTestFixture,
                                                  public ::testing::WithParamInterface<ElementType>
{
};

TEST_P(SkeletonComponentTestParameterisedFixture, DataShmObjectSizeCalc_Simulation)
{
    RecordProperty("Verifies", "");
    RecordProperty("Description", "Check if the data_shm is calculated correctly.");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("Priority", "1");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    constexpr std::size_t number_of_slots = 10U;

    const ElementType element_type = GetParam();

    // Given a skeleton with one event "fooEvent" registered
    ServiceTypeDeployment service_type_depl = CreateTypeDeployment(1U, {{test::kFooEventName, test::kFooEventId}});

    mock_binding::SkeletonEvent<std::string> event{};
    SkeletonBinding::SkeletonEventBindings events{};
    SkeletonBinding::SkeletonFieldBindings fields{};

    std::vector<std::pair<std::string, LolaEventInstanceDeployment>> lola_event_inst_depls;
    std::vector<std::pair<std::string, LolaFieldInstanceDeployment>> lola_field_inst_depls;
    if (element_type == ElementType::EVENT)
    {
        events.emplace(test::kFooEventName, event);
        lola_event_inst_depls.push_back(
            {test::kFooEventName, LolaEventInstanceDeployment{number_of_slots, 10U, 1U, true}});
    }
    else
    {
        fields.emplace(test::kFooEventName, event);
        lola_field_inst_depls.push_back(
            {test::kFooEventName, LolaEventInstanceDeployment{number_of_slots, 10U, 1U, true}});
    }
    ServiceInstanceDeployment service_instance_deployment{
        test::kFooService,
        CreateLolaServiceInstanceDeployment(
            test::kDefaultLolaInstanceId, lola_event_inst_depls, lola_field_inst_depls, {}, {}),
        QualityType::kASIL_QM,
        kInstanceSpecifier};

    const auto instance_identifier = make_InstanceIdentifier(service_instance_deployment, service_type_depl);
    auto unit = CreateSkeleton(instance_identifier);
    ASSERT_NE(unit, nullptr);

    const auto* const lola_service_type_deployment =
        amp::get_if<LolaServiceTypeDeployment>(&test::kValidMinimalTypeDeployment.binding_info_);
    ASSERT_NE(lola_service_type_deployment, nullptr);

    // Expect, that the LoLa runtime returns that ShmSize calculation shall be done via simulation
    EXPECT_CALL(lola_runtime_mock_, GetShmSizeCalculationMode()).WillOnce(Return(ShmSizeCalculationMode::kSimulation));

    // Expecting that the event is offered during the simulation dry run
    EXPECT_CALL(event, PrepareOffer())
        .WillOnce(testing::Invoke([&unit, lola_service_type_deployment, element_type]() -> ResultBlank {
            ElementFqId event_fqn{lola_service_type_deployment->service_id_,
                                  test::kFooEventId,
                                  test::kDefaultLolaInstanceId,
                                  element_type};
            unit->Register<uint8_t>(event_fqn, test::kDefaultEventProperties);
            return {};
        }));

    // When preparing to offer a service
    const auto val = unit->PrepareOffer(events, fields, {});

    // then expect, that it has a value!
    EXPECT_TRUE(val.has_value());

    // Then the respective Shared Memory file for Data is created with a size larger than already the pure payload
    // within data-shm-object would occupy (this is a lower bound for consistency)
    EXPECT_GE(GetSize(data_shm), CalculateLowerBoundDataShmSize({{sizeof(std::uint8_t), number_of_slots}}));

    // Then the respective Shared Memory file for Control is created with a size larger than already the pure payload
    // within control-shm-object would occupy (this is a lower bound for consistency)
    EXPECT_GE(GetSize(control_shm), CalculateLowerBoundControlShmSize({{sizeof(std::uint8_t), number_of_slots}}));
}

TEST_P(SkeletonComponentTestParameterisedFixture, DataShmObjectSizeCalc_Estimation)
{
    RecordProperty("Verifies", "");
    RecordProperty("Description", "Check if the data_shm is calculated correctly.");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("Priority", "1");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    constexpr std::size_t number_of_slots = 10U;

    const ElementType element_type = GetParam();

    // Given a skeleton with one event "fooEvent" registered
    ServiceTypeDeployment service_type_depl = CreateTypeDeployment(1U, {{test::kFooEventName, test::kFooEventId}});

    mock_binding::SkeletonEvent<std::string> event{};
    SkeletonBinding::SkeletonEventBindings events{};
    SkeletonBinding::SkeletonFieldBindings fields{};

    std::vector<std::pair<std::string, LolaEventInstanceDeployment>> lola_event_inst_depls;
    std::vector<std::pair<std::string, LolaFieldInstanceDeployment>> lola_field_inst_depls;
    if (element_type == ElementType::EVENT)
    {
        events.emplace(test::kFooEventName, event);
        lola_event_inst_depls.push_back(
            {test::kFooEventName, LolaEventInstanceDeployment{number_of_slots, 10U, 1U, true}});
    }
    else
    {
        fields.emplace(test::kFooEventName, event);
        lola_field_inst_depls.push_back(
            {test::kFooEventName, LolaEventInstanceDeployment{number_of_slots, 10U, 1U, true}});
    }
    ServiceInstanceDeployment service_instance_deployment{
        test::kFooService,
        CreateLolaServiceInstanceDeployment(
            test::kDefaultLolaInstanceId, lola_event_inst_depls, lola_field_inst_depls, {}, {}),
        QualityType::kASIL_QM,
        kInstanceSpecifier};

    const auto instance_identifier = make_InstanceIdentifier(service_instance_deployment, service_type_depl);
    auto unit = CreateSkeleton(instance_identifier);
    ASSERT_NE(unit, nullptr);

    const auto* const lola_service_type_deployment =
        amp::get_if<LolaServiceTypeDeployment>(&test::kValidMinimalTypeDeployment.binding_info_);
    ASSERT_NE(lola_service_type_deployment, nullptr);

    // Expect, that the LoLa runtime returns that ShmSize calculation shall be done via estimation
    EXPECT_CALL(lola_runtime_mock_, GetShmSizeCalculationMode()).WillOnce(Return(ShmSizeCalculationMode::kEstimation));

    // When preparing to offer a service
    const auto val = unit->PrepareOffer(events, fields, {});
    // then expect, that it has a value!
    EXPECT_TRUE(val.has_value());

    // Then the respective Shared Memory file for Data is created with a size larger than already the pure payload
    // within data-shm-object would occupy (this is a lower bound for consistency)
    EXPECT_GE(GetSize(data_shm), CalculateLowerBoundDataShmSize({{sizeof(std::uint8_t), number_of_slots}}));

    // Then the respective Shared Memory file for Control is created with a size larger than already the pure payload
    // within control-shm-object would occupy (this is a lower bound for consistency)
    EXPECT_GE(GetSize(control_shm), CalculateLowerBoundControlShmSize({{sizeof(std::uint8_t), number_of_slots}}));
}

/// \brief Testcase test once a calculation of shm-object size by "estimation" algo and then directly afterwards
///        for the very same deployment a calculation by "simulation". We expect, that the sizes of the shm-objects
///        based on "simulation" are always smaller, than the "estimated" sizes as during estimation we add a lot of
///        "security buffers".
TEST_P(SkeletonComponentTestParameterisedFixture, DataShmObjectSizeCalc_EstimationVsSimulation)
{
    RecordProperty("Verifies", "");
    RecordProperty("Description", "Check if the data_shm is calculated correctly.");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("Priority", "1");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    constexpr std::size_t number_of_slots = 10U;

    const ElementType element_type = GetParam();

    // Given a skeleton with one event "fooEvent" registered
    ServiceTypeDeployment service_type_depl = CreateTypeDeployment(1U, {{test::kFooEventName, test::kFooEventId}});

    mock_binding::SkeletonEvent<std::string> event{};
    SkeletonBinding::SkeletonEventBindings events{};
    SkeletonBinding::SkeletonFieldBindings fields{};

    std::vector<std::pair<std::string, LolaEventInstanceDeployment>> lola_event_inst_depls;
    std::vector<std::pair<std::string, LolaFieldInstanceDeployment>> lola_field_inst_depls;
    if (element_type == ElementType::EVENT)
    {
        events.emplace(test::kFooEventName, event);
        lola_event_inst_depls.push_back(
            {test::kFooEventName, LolaEventInstanceDeployment{number_of_slots, 10U, 1U, true}});
    }
    else
    {
        fields.emplace(test::kFooEventName, event);
        lola_field_inst_depls.push_back(
            {test::kFooEventName, LolaEventInstanceDeployment{number_of_slots, 10U, 1U, true}});
    }
    ServiceInstanceDeployment service_instance_deployment{
        test::kFooService,
        CreateLolaServiceInstanceDeployment(
            test::kDefaultLolaInstanceId, lola_event_inst_depls, lola_field_inst_depls, {}, {}),
        QualityType::kASIL_QM,
        kInstanceSpecifier};

    const auto instance_identifier = make_InstanceIdentifier(service_instance_deployment, service_type_depl);
    std::size_t data_size_estimated{};
    std::size_t control_size_estimated{};

    {
        // When we instantiate it once
        auto unit = CreateSkeleton(instance_identifier);
        ASSERT_NE(unit, nullptr);

        const auto* const lola_service_type_deployment =
            amp::get_if<LolaServiceTypeDeployment>(&test::kValidMinimalTypeDeployment.binding_info_);
        ASSERT_NE(lola_service_type_deployment, nullptr);

        // where we expect, that the LoLa runtime returns that ShmSize calculation shall be done via estimation
        EXPECT_CALL(lola_runtime_mock_, GetShmSizeCalculationMode())
            .WillOnce(Return(ShmSizeCalculationMode::kEstimation));

        // When preparing to offer a service
        const auto val = unit->PrepareOffer(events, fields, {});
        // then expect, that it has a value!
        EXPECT_TRUE(val.has_value());

        // then we store the sizes of Data and Control shm-objects for later comparison
        data_size_estimated = GetSize(data_shm);
        control_size_estimated = GetSize(control_shm);

        // and call PrepareStopOffer so that also its shared-mem objects are cleaned up ...
        unit->PrepareStopOffer({});
    }

    ASSERT_FALSE(fileExists(data_shm));
    ASSERT_FALSE(fileExists(control_shm));
    ASSERT_FALSE(fileExists(asil_control_shm));

    {
        // and then we instantiate it a 2nd time
        auto unit = CreateSkeleton(instance_identifier);
        ASSERT_NE(unit, nullptr);

        const auto* const lola_service_type_deployment =
            amp::get_if<LolaServiceTypeDeployment>(&test::kValidMinimalTypeDeployment.binding_info_);
        ASSERT_NE(lola_service_type_deployment, nullptr);

        // where we now expect, that the LoLa runtime returns that ShmSize calculation shall be done via simulation
        EXPECT_CALL(lola_runtime_mock_, GetShmSizeCalculationMode())
            .WillOnce(Return(ShmSizeCalculationMode::kSimulation));

        // Expecting that the event is offered during the simulation dry run
        EXPECT_CALL(event, PrepareOffer())
            .WillOnce(testing::Invoke([&unit, lola_service_type_deployment, element_type]() -> ResultBlank {
                ElementFqId event_fqn{lola_service_type_deployment->service_id_,
                                      test::kFooEventId,
                                      test::kDefaultLolaInstanceId,
                                      element_type};
                unit->Register<uint8_t>(event_fqn, test::kDefaultEventProperties);
                return {};
            }));

        // When preparing to offer a service
        const auto val = unit->PrepareOffer(events, fields, {});
        // then expect, that it has a value!
        EXPECT_TRUE(val.has_value());

        // then the sizes for control and data are smaller than in the "estimation" case from the first instantiation
        EXPECT_LT(GetSize(data_shm), data_size_estimated);
        EXPECT_LT(GetSize(control_shm), control_size_estimated);
    }
}

INSTANTIATE_TEST_SUITE_P(SkeletonComponentTestParameterisedFixture,
                         SkeletonComponentTestParameterisedFixture,
                         ::testing::Values(ElementType::EVENT, ElementType::FIELD));

}  // namespace
}  // namespace lola
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw
