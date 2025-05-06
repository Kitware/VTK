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

#include <viskores/cont/ArrayHandleUniformPointCoordinates.h>

#include <viskores/cont/testing/Testing.h>

namespace
{

using Vector3 = viskores::Vec3f;

const viskores::Id3 DIMENSIONS(16, 18, 5);
const viskores::Id NUM_POINTS = 1440;

const Vector3 ORIGIN(-20, 5, -10);
const Vector3 SPACING(10, 1, 0.1f);

void TestArrayHandleUniformPointCoordinates()
{
  std::cout << "Creating ArrayHandleUniformPointCoordinates" << std::endl;

  viskores::cont::ArrayHandleUniformPointCoordinates arrayHandle(DIMENSIONS, ORIGIN, SPACING);
  VISKORES_TEST_ASSERT(arrayHandle.GetNumberOfValues() == NUM_POINTS,
                       "Array computed wrong number of points.");

  std::cout << "Getting array portal." << std::endl;
  auto portal = arrayHandle.ReadPortal();
  VISKORES_TEST_ASSERT(portal.GetNumberOfValues() == NUM_POINTS,
                       "Portal has wrong number of points.");
  VISKORES_TEST_ASSERT(portal.GetRange3() == DIMENSIONS, "Portal range is wrong.");

  std::cout << "Checking computed values of portal." << std::endl;
  Vector3 expectedValue;
  viskores::Id flatIndex = 0;
  viskores::Id3 blockIndex;
  expectedValue[2] = ORIGIN[2];
  for (blockIndex[2] = 0; blockIndex[2] < DIMENSIONS[2]; blockIndex[2]++)
  {
    expectedValue[1] = ORIGIN[1];
    for (blockIndex[1] = 0; blockIndex[1] < DIMENSIONS[1]; blockIndex[1]++)
    {
      expectedValue[0] = ORIGIN[0];
      for (blockIndex[0] = 0; blockIndex[0] < DIMENSIONS[0]; blockIndex[0]++)
      {
        VISKORES_TEST_ASSERT(test_equal(expectedValue, portal.Get(flatIndex)),
                             "Got wrong value for flat index.");

        VISKORES_TEST_ASSERT(test_equal(expectedValue, portal.Get(blockIndex)),
                             "Got wrong value for block index.");

        flatIndex++;
        expectedValue[0] += SPACING[0];
      }
      expectedValue[1] += SPACING[1];
    }
    expectedValue[2] += SPACING[2];
  }
}

} // anonymous namespace

int UnitTestArrayHandleUniformPointCoordinates(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestArrayHandleUniformPointCoordinates, argc, argv);
}
