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

#include <viskores/cont/arg/TypeCheckTagKeys.h>

#include <viskores/worklet/Keys.h>

#include <viskores/cont/ArrayHandle.h>

#include <viskores/cont/testing/Testing.h>

namespace
{

struct TestNotKeys
{
};

void TestCheckKeys()
{
  std::cout << "Checking reporting of type checking keys." << std::endl;

  using viskores::cont::arg::TypeCheck;
  using viskores::cont::arg::TypeCheckTagKeys;

  VISKORES_TEST_ASSERT((TypeCheck<TypeCheckTagKeys, viskores::worklet::Keys<viskores::Id>>::value),
                       "Type check failed.");
  VISKORES_TEST_ASSERT(
    (TypeCheck<TypeCheckTagKeys, viskores::worklet::Keys<viskores::Float32>>::value),
    "Type check failed.");
  VISKORES_TEST_ASSERT((TypeCheck<TypeCheckTagKeys, viskores::worklet::Keys<viskores::Id3>>::value),
                       "Type check failed.");

  VISKORES_TEST_ASSERT(!(TypeCheck<TypeCheckTagKeys, TestNotKeys>::value), "Type check failed.");
  VISKORES_TEST_ASSERT(!(TypeCheck<TypeCheckTagKeys, viskores::Id>::value), "Type check failed.");
  VISKORES_TEST_ASSERT(
    !(TypeCheck<TypeCheckTagKeys, viskores::cont::ArrayHandle<viskores::Id>>::value),
    "Type check failed.");
}

} // anonymous namespace

int UnitTestTypeCheckKeys(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestCheckKeys, argc, argv);
}
