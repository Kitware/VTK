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

#include <viskores/cont/ArrayHandleIndex.h>

#include <viskores/cont/testing/Testing.h>

namespace UnitTestArrayHandleIndexNamespace
{

const viskores::Id ARRAY_SIZE = 10;

void TestArrayHandleIndex()
{
  viskores::cont::ArrayHandleIndex array(ARRAY_SIZE);
  VISKORES_TEST_ASSERT(array.GetNumberOfValues() == ARRAY_SIZE, "Bad size.");
  VISKORES_TEST_ASSERT(array.GetNumberOfComponentsFlat() == 1);
  auto portal = array.ReadPortal();
  for (viskores::Id index = 0; index < ARRAY_SIZE; index++)
  {
    VISKORES_TEST_ASSERT(portal.Get(index) == index, "Index array has unexpected value.");
  }
}

} // namespace UnitTestArrayHandleIndexNamespace

int UnitTestArrayHandleIndex(int argc, char* argv[])
{
  using namespace UnitTestArrayHandleIndexNamespace;
  return viskores::cont::testing::Testing::Run(TestArrayHandleIndex, argc, argv);
}
