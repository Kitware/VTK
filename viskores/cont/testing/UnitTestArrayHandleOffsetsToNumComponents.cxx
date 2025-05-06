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

#include <viskores/cont/ArrayHandleOffsetsToNumComponents.h>

#include <viskores/cont/ArrayHandleConstant.h>
#include <viskores/cont/ArrayHandleCounting.h>
#include <viskores/cont/CellSetExplicit.h>
#include <viskores/cont/ConvertNumComponentsToOffsets.h>

#include <viskores/cont/testing/Testing.h>

namespace
{

constexpr viskores::Id ARRAY_SIZE = 20;

template <typename OffsetsArray, typename ExpectedNumComponents>
void TestOffsetsToNumComponents(const OffsetsArray& offsetsArray,
                                const ExpectedNumComponents& expectedNumComponents)
{
  VISKORES_TEST_ASSERT(offsetsArray.GetNumberOfValues() ==
                       expectedNumComponents.GetNumberOfValues() + 1);

  auto numComponents = viskores::cont::make_ArrayHandleOffsetsToNumComponents(offsetsArray);
  VISKORES_TEST_ASSERT(numComponents.GetNumberOfValues() ==
                       expectedNumComponents.GetNumberOfValues());
  VISKORES_TEST_ASSERT(
    test_equal_portals(numComponents.ReadPortal(), expectedNumComponents.ReadPortal()));
}

void TryNormalOffsets()
{
  std::cout << "Normal offset array." << std::endl;

  viskores::cont::ArrayHandle<viskores::IdComponent> numComponents;
  numComponents.Allocate(ARRAY_SIZE);
  auto numComponentsPortal = numComponents.WritePortal();
  for (viskores::IdComponent i = 0; i < ARRAY_SIZE; ++i)
  {
    numComponentsPortal.Set(i, i % 5);
  }

  auto offsets = viskores::cont::ConvertNumComponentsToOffsets(numComponents);
  TestOffsetsToNumComponents(offsets, numComponents);
}

void TryFancyOffsets()
{
  std::cout << "Fancy offset array." << std::endl;
  viskores::cont::ArrayHandleCounting<viskores::Id> offsets(0, 3, ARRAY_SIZE + 1);
  TestOffsetsToNumComponents(
    offsets, viskores::cont::ArrayHandleConstant<viskores::IdComponent>(3, ARRAY_SIZE));
}

void Run()
{
  TryNormalOffsets();
  TryFancyOffsets();
}

} // anonymous namespace

int UnitTestArrayHandleOffsetsToNumComponents(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(Run, argc, argv);
}
