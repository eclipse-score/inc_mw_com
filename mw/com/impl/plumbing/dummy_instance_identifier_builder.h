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


#ifndef PLATFORM_AAS_MW_COM_IMPL_PLUMBING_TEST_DUMMY_INSTANCE_IDENTIFIER_BUILDER
#define PLATFORM_AAS_MW_COM_IMPL_PLUMBING_TEST_DUMMY_INSTANCE_IDENTIFIER_BUILDER

#include "platform/aas/mw/com/impl/bindings/lola/proxy.h"
#include "platform/aas/mw/com/impl/instance_specifier.h"
#include <memory>

namespace bmw
{
namespace mw
{
namespace com
{
namespace impl
{
namespace test
{

class DummyInstanceIdentifierBuilder
{
  public:
    DummyInstanceIdentifierBuilder();

    InstanceIdentifier CreateValidLolaInstanceIdentifier();
    InstanceIdentifier CreateValidLolaInstanceIdentifierWithEvent();
    InstanceIdentifier CreateValidLolaInstanceIdentifierWithEvent(
        const LolaServiceInstanceDeployment::EventInstanceMapping& events);
    InstanceIdentifier CreateValidLolaInstanceIdentifierWithField();
    InstanceIdentifier CreateValidLolaInstanceIdentifierWithField(
        const LolaServiceInstanceDeployment::EventInstanceMapping& events);
    InstanceIdentifier CreateLolaInstanceIdentifierWithoutInstanceId();
    InstanceIdentifier CreateLolaInstanceIdentifierWithoutTypeDeployment();
    InstanceIdentifier CreateBlankBindingInstanceIdentifier();
    InstanceIdentifier CreateSomeIpBindingInstanceIdentifier();

  private:
    LolaServiceInstanceDeployment service_instance_deployment_;
    SomeIpServiceInstanceDeployment some_ip_service_instance_deployment_information_;

    LolaServiceTypeDeployment service_type_deployment_;
    ServiceTypeDeployment type_deployment_;
    ServiceIdentifierType type_;
    InstanceSpecifier instance_specifier_;
    std::unique_ptr<ServiceInstanceDeployment> instance_deployment_;
};

}  // namespace test
}  // namespace impl
}  // namespace com
}  // namespace mw
}  // namespace bmw

#endif  // PLATFORM_AAS_MW_COM_IMPL_PLUMBING_TEST_DUMMY_INSTANCE_IDENTIFIER_BUILDER
