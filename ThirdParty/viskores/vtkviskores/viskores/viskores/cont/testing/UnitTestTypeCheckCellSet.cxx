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

#include <viskores/cont/arg/TypeCheckTagCellSet.h>

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/CellSetExplicit.h>
#include <viskores/cont/CellSetStructured.h>

#include <viskores/cont/testing/Testing.h>

namespace
{

struct TestNotCellSet
{
};

void TestCheckCellSet()
{
  std::cout << "Checking reporting of type checking cell set." << std::endl;

  using viskores::cont::arg::TypeCheck;
  using viskores::cont::arg::TypeCheckTagCellSet;

  VISKORES_TEST_ASSERT((TypeCheck<TypeCheckTagCellSet, viskores::cont::CellSetExplicit<>>::value),
                       "Type check failed.");

  VISKORES_TEST_ASSERT(
    (TypeCheck<TypeCheckTagCellSet, viskores::cont::CellSetStructured<2>>::value),
    "Type check failed.");

  VISKORES_TEST_ASSERT(
    (TypeCheck<TypeCheckTagCellSet, viskores::cont::CellSetStructured<3>>::value),
    "Type check failed.");

  VISKORES_TEST_ASSERT(!(TypeCheck<TypeCheckTagCellSet, TestNotCellSet>::value),
                       "Type check failed.");

  VISKORES_TEST_ASSERT(!(TypeCheck<TypeCheckTagCellSet, viskores::Id>::value),
                       "Type check failed.");

  VISKORES_TEST_ASSERT(
    !(TypeCheck<TypeCheckTagCellSet, viskores::cont::ArrayHandle<viskores::Id>>::value),
    "Type check failed.");
}

} // anonymous namespace

int UnitTestTypeCheckCellSet(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestCheckCellSet, argc, argv);
}
