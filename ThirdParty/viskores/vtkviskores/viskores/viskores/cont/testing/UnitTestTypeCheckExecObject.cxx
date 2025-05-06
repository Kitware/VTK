//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

// This test does not really need a device compiler
#define VISKORES_NO_ERROR_ON_MIXED_CUDA_CXX_TAG

#include <viskores/cont/arg/TypeCheckTagExecObject.h>

#include <viskores/cont/ArrayHandle.h>

#include <viskores/cont/testing/Testing.h>

namespace
{

struct TestExecutionObject : viskores::cont::ExecutionObjectBase
{
};
struct TestNotExecutionObject
{
};

void TestCheckExecObject()
{
  std::cout << "Checking reporting of type checking exec object." << std::endl;

  using viskores::cont::arg::TypeCheck;
  using viskores::cont::arg::TypeCheckTagExecObject;

  VISKORES_TEST_ASSERT((TypeCheck<TypeCheckTagExecObject, TestExecutionObject>::value),
                       "Type check failed.");

  VISKORES_TEST_ASSERT(!(TypeCheck<TypeCheckTagExecObject, TestNotExecutionObject>::value),
                       "Type check failed.");

  VISKORES_TEST_ASSERT(!(TypeCheck<TypeCheckTagExecObject, viskores::Id>::value),
                       "Type check failed.");

  VISKORES_TEST_ASSERT(
    !(TypeCheck<TypeCheckTagExecObject, viskores::cont::ArrayHandle<viskores::Id>>::value),
    "Type check failed.");
}

} // anonymous namespace

int UnitTestTypeCheckExecObject(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestCheckExecObject, argc, argv);
}
