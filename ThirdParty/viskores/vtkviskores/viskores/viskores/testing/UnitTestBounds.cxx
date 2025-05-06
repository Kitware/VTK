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

#include <viskores/Bounds.h>
#include <viskores/VecTraits.h>

#include <viskores/testing/Testing.h>

namespace
{

void TestBounds()
{
  using Vec3 = viskores::Vec3f_64;

  std::cout << "Empty bounds." << std::endl;
  viskores::Bounds emptyBounds;
  VISKORES_TEST_ASSERT(!emptyBounds.IsNonEmpty(), "Non empty bounds not empty.");

  viskores::Bounds emptyBounds2;
  VISKORES_TEST_ASSERT(!emptyBounds2.IsNonEmpty(), "2nd empty bounds not empty.");
  VISKORES_TEST_ASSERT(!emptyBounds.Union(emptyBounds2).IsNonEmpty(),
                       "Union of empty bounds not empty.");
  emptyBounds2.Include(emptyBounds);
  VISKORES_TEST_ASSERT(!emptyBounds2.IsNonEmpty(), "Include empty in empty is not empty.");

  std::cout << "Single value bounds." << std::endl;
  viskores::Bounds singleValueBounds(1.0, 1.0, 2.0, 2.0, 3.0, 3.0);
  VISKORES_TEST_ASSERT(singleValueBounds.IsNonEmpty(), "Empty?");
  VISKORES_TEST_ASSERT(test_equal(singleValueBounds.Center(), Vec3(1, 2, 3)), "Bad center");
  VISKORES_TEST_ASSERT(singleValueBounds.Contains(Vec3(1, 2, 3)), "Contains fail");
  VISKORES_TEST_ASSERT(!singleValueBounds.Contains(Vec3(0, 0, 0)), "Contains fail");
  VISKORES_TEST_ASSERT(!singleValueBounds.Contains(Vec3(2, 2, 2)), "contains fail");
  VISKORES_TEST_ASSERT(!singleValueBounds.Contains(Vec3(5, 5, 5)), "contains fail");

  viskores::Bounds unionBounds = emptyBounds + singleValueBounds;
  VISKORES_TEST_ASSERT(unionBounds.IsNonEmpty(), "Empty?");
  VISKORES_TEST_ASSERT(test_equal(unionBounds.Center(), Vec3(1, 2, 3)), "Bad center");
  VISKORES_TEST_ASSERT(unionBounds.Contains(Vec3(1, 2, 3)), "Contains fail");
  VISKORES_TEST_ASSERT(!unionBounds.Contains(Vec3(0, 0, 0)), "Contains fail");
  VISKORES_TEST_ASSERT(!unionBounds.Contains(Vec3(2, 2, 2)), "contains fail");
  VISKORES_TEST_ASSERT(!unionBounds.Contains(Vec3(5, 5, 5)), "contains fail");
  VISKORES_TEST_ASSERT(singleValueBounds == unionBounds, "Union not equal");

  std::cout << "Low bounds." << std::endl;
  viskores::Bounds lowBounds(Vec3(-10, -5, -1), Vec3(-5, -2, 0));
  VISKORES_TEST_ASSERT(lowBounds.IsNonEmpty(), "Empty?");
  VISKORES_TEST_ASSERT(test_equal(lowBounds.Center(), Vec3(-7.5, -3.5, -0.5)), "Bad center");
  VISKORES_TEST_ASSERT(!lowBounds.Contains(Vec3(-20)), "Contains fail");
  VISKORES_TEST_ASSERT(!lowBounds.Contains(Vec3(-2)), "Contains fail");
  VISKORES_TEST_ASSERT(lowBounds.Contains(Vec3(-7, -2, -0.5)), "Contains fail");
  VISKORES_TEST_ASSERT(!lowBounds.Contains(Vec3(0)), "Contains fail");
  VISKORES_TEST_ASSERT(!lowBounds.Contains(Vec3(10)), "Contains fail");

  unionBounds = singleValueBounds + lowBounds;
  VISKORES_TEST_ASSERT(unionBounds.IsNonEmpty(), "Empty?");
  VISKORES_TEST_ASSERT(!unionBounds.Contains(Vec3(-20)), "Contains fail");
  VISKORES_TEST_ASSERT(!unionBounds.Contains(Vec3(-2)), "Contains fail");
  VISKORES_TEST_ASSERT(unionBounds.Contains(Vec3(-7, -2, -0.5)), "Contains fail");
  VISKORES_TEST_ASSERT(unionBounds.Contains(Vec3(0)), "Contains fail");
  VISKORES_TEST_ASSERT(!unionBounds.Contains(Vec3(10)), "Contains fail");

  std::cout << "High bounds." << std::endl;
  viskores::Float64 highBoundsArray[6] = { 15.0, 20.0, 2.0, 5.0, 5.0, 10.0 };
  viskores::Bounds highBounds(highBoundsArray);
  VISKORES_TEST_ASSERT(highBounds.IsNonEmpty(), "Empty?");
  VISKORES_TEST_ASSERT(!highBounds.Contains(Vec3(-20)), "Contains fail");
  VISKORES_TEST_ASSERT(!highBounds.Contains(Vec3(-2)), "Contains fail");
  VISKORES_TEST_ASSERT(!highBounds.Contains(Vec3(-7, -2, -0.5)), "Contains fail");
  VISKORES_TEST_ASSERT(!highBounds.Contains(Vec3(0)), "Contains fail");
  VISKORES_TEST_ASSERT(!highBounds.Contains(Vec3(4)), "Contains fail");
  VISKORES_TEST_ASSERT(highBounds.Contains(Vec3(17, 3, 7)), "Contains fail");
  VISKORES_TEST_ASSERT(!highBounds.Contains(Vec3(25)), "Contains fail");

  unionBounds = highBounds.Union(singleValueBounds);
  VISKORES_TEST_ASSERT(unionBounds.IsNonEmpty(), "Empty?");
  VISKORES_TEST_ASSERT(!unionBounds.Contains(Vec3(-20)), "Contains fail");
  VISKORES_TEST_ASSERT(!unionBounds.Contains(Vec3(-2)), "Contains fail");
  VISKORES_TEST_ASSERT(!unionBounds.Contains(Vec3(-7, -2, -0.5)), "Contains fail");
  VISKORES_TEST_ASSERT(!unionBounds.Contains(Vec3(0)), "Contains fail");
  VISKORES_TEST_ASSERT(unionBounds.Contains(Vec3(4)), "Contains fail");
  VISKORES_TEST_ASSERT(unionBounds.Contains(Vec3(17, 3, 7)), "Contains fail");
  VISKORES_TEST_ASSERT(!unionBounds.Contains(Vec3(25)), "Contains fail");

  unionBounds.Include(Vec3(-1));
  VISKORES_TEST_ASSERT(unionBounds.IsNonEmpty(), "Empty?");
  VISKORES_TEST_ASSERT(!unionBounds.Contains(Vec3(-20)), "Contains fail");
  VISKORES_TEST_ASSERT(!unionBounds.Contains(Vec3(-2)), "Contains fail");
  VISKORES_TEST_ASSERT(!unionBounds.Contains(Vec3(-7, -2, -0.5)), "Contains fail");
  VISKORES_TEST_ASSERT(unionBounds.Contains(Vec3(0)), "Contains fail");
  VISKORES_TEST_ASSERT(unionBounds.Contains(Vec3(4)), "Contains fail");
  VISKORES_TEST_ASSERT(unionBounds.Contains(Vec3(17, 3, 7)), "Contains fail");
  VISKORES_TEST_ASSERT(!unionBounds.Contains(Vec3(25)), "Contains fail");

  unionBounds.Include(lowBounds);
  VISKORES_TEST_ASSERT(unionBounds.IsNonEmpty(), "Empty?");
  VISKORES_TEST_ASSERT(!unionBounds.Contains(Vec3(-20)), "Contains fail");
  VISKORES_TEST_ASSERT(!unionBounds.Contains(Vec3(-2)), "Contains fail");
  VISKORES_TEST_ASSERT(unionBounds.Contains(Vec3(-7, -2, -0.5)), "Contains fail");
  VISKORES_TEST_ASSERT(unionBounds.Contains(Vec3(0)), "Contains fail");
  VISKORES_TEST_ASSERT(unionBounds.Contains(Vec3(4)), "Contains fail");
  VISKORES_TEST_ASSERT(unionBounds.Contains(Vec3(17, 3, 7)), "Contains fail");
  VISKORES_TEST_ASSERT(!unionBounds.Contains(Vec3(25)), "Contains fail");

  std::cout << "Try adding infinity." << std::endl;
  unionBounds.Include(Vec3(viskores::Infinity64()));
  VISKORES_TEST_ASSERT(unionBounds.IsNonEmpty(), "Empty?");
  VISKORES_TEST_ASSERT(!unionBounds.Contains(Vec3(-20)), "Contains fail");
  VISKORES_TEST_ASSERT(!unionBounds.Contains(Vec3(-2)), "Contains fail");
  VISKORES_TEST_ASSERT(unionBounds.Contains(Vec3(-7, -2, -0.5)), "Contains fail");
  VISKORES_TEST_ASSERT(unionBounds.Contains(Vec3(0)), "Contains fail");
  VISKORES_TEST_ASSERT(unionBounds.Contains(Vec3(4)), "Contains fail");
  VISKORES_TEST_ASSERT(unionBounds.Contains(Vec3(17, 3, 7)), "Contains fail");
  VISKORES_TEST_ASSERT(unionBounds.Contains(Vec3(25)), "Contains fail");

  std::cout << "Try adding NaN." << std::endl;
  // Turn off floating point exceptions. This is only for conditions that allow NaNs.
  viskores::testing::FloatingPointExceptionTrapDisable();
  unionBounds.Include(Vec3(viskores::Nan64()));
  VISKORES_TEST_ASSERT(!unionBounds.Contains(Vec3(-20)), "Contains fail");
  VISKORES_TEST_ASSERT(!unionBounds.Contains(Vec3(-2)), "Contains fail");
  VISKORES_TEST_ASSERT(unionBounds.Contains(Vec3(-7, -2, -0.5)), "Contains fail");
  VISKORES_TEST_ASSERT(unionBounds.Contains(Vec3(0)), "Contains fail");
  VISKORES_TEST_ASSERT(unionBounds.Contains(Vec3(4)), "Contains fail");
  VISKORES_TEST_ASSERT(unionBounds.Contains(Vec3(17, 3, 7)), "Contains fail");
  VISKORES_TEST_ASSERT(unionBounds.Contains(Vec3(25)), "Contains fail");

  std::cout << "Try VecTraits." << std::endl;
  using VTraits = viskores::VecTraits<viskores::Bounds>;
  VISKORES_TEST_ASSERT(VTraits::NUM_COMPONENTS == 3);
  viskores::Bounds simpleBounds{ { 0.0, 1.0 }, { 2.0, 4.0 }, { 8.0, 16.0 } };
  VISKORES_TEST_ASSERT(VTraits::GetNumberOfComponents(simpleBounds) == 3);
  VISKORES_TEST_ASSERT(VTraits::GetComponent(simpleBounds, 0) == viskores::Range{ 0.0, 1.0 });
  VISKORES_TEST_ASSERT(VTraits::GetComponent(simpleBounds, 1) == viskores::Range{ 2.0, 4.0 });
  VISKORES_TEST_ASSERT(VTraits::GetComponent(simpleBounds, 2) == viskores::Range{ 8.0, 16.0 });
  viskores::Vec<viskores::Range, 3> simpleBoundsCopy;
  VTraits::CopyInto(simpleBounds, simpleBoundsCopy);
  VISKORES_TEST_ASSERT(simpleBoundsCopy ==
                       viskores::Vec<viskores::Range, 3>{ { 0, 1 }, { 2, 4 }, { 8, 16 } });
  VTraits::SetComponent(simpleBounds, 0, { 8.0, 16.0 });
  VTraits::SetComponent(simpleBounds, 2, { 2.0, 4.0 });
  VTraits::SetComponent(simpleBounds, 1, { 0.0, 1.0 });
  VISKORES_TEST_ASSERT(!simpleBounds.Contains(viskores::Vec3f_64{ 0.5, 3.0, 12.0 }));
  VISKORES_TEST_ASSERT(simpleBounds.Contains(viskores::Vec3f_64{ 12.0, 0.5, 3.0 }));
}

} // anonymous namespace

int UnitTestBounds(int argc, char* argv[])
{
  return viskores::testing::Testing::Run(TestBounds, argc, argv);
}
