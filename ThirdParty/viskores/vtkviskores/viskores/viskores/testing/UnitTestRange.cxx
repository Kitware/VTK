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

#include <viskores/Range.h>
#include <viskores/VecTraits.h>

#include <viskores/testing/Testing.h>

namespace
{

void TestRange()
{
  std::cout << "Empty range." << std::endl;
  viskores::Range emptyRange;
  VISKORES_TEST_ASSERT(!emptyRange.IsNonEmpty(), "Non empty range not empty.");
  VISKORES_TEST_ASSERT(test_equal(emptyRange.Length(), 0.0), "Bad length.");

  viskores::Range emptyRange2;
  VISKORES_TEST_ASSERT(!emptyRange2.IsNonEmpty(), "2nd empty range not empty.");
  VISKORES_TEST_ASSERT(!emptyRange.Union(emptyRange2).IsNonEmpty(),
                       "Union of empty ranges not empty.");
  emptyRange2.Include(emptyRange);
  VISKORES_TEST_ASSERT(!emptyRange2.IsNonEmpty(), "Include empty in empty is not empty.");

  std::cout << "Single value range." << std::endl;
  viskores::Range singleValueRange(5.0, 5.0);
  VISKORES_TEST_ASSERT(singleValueRange.IsNonEmpty(), "Empty?");
  VISKORES_TEST_ASSERT(test_equal(singleValueRange.Length(), 0.0), "Bad length.");
  VISKORES_TEST_ASSERT(test_equal(singleValueRange.Center(), 5), "Bad center.");
  VISKORES_TEST_ASSERT(singleValueRange.Contains(5.0), "Does not contain value");
  VISKORES_TEST_ASSERT(!singleValueRange.Contains(0.0), "Contains outside");
  VISKORES_TEST_ASSERT(!singleValueRange.Contains(10), "Contains outside");

  viskores::Range unionRange = emptyRange + singleValueRange;
  VISKORES_TEST_ASSERT(unionRange.IsNonEmpty(), "Empty?");
  VISKORES_TEST_ASSERT(test_equal(unionRange.Length(), 0.0), "Bad length.");
  VISKORES_TEST_ASSERT(test_equal(unionRange.Center(), 5), "Bad center.");
  VISKORES_TEST_ASSERT(unionRange.Contains(5.0), "Does not contain value");
  VISKORES_TEST_ASSERT(!unionRange.Contains(0.0), "Contains outside");
  VISKORES_TEST_ASSERT(!unionRange.Contains(10), "Contains outside");
  VISKORES_TEST_ASSERT(singleValueRange == unionRange, "Union not equal");
  VISKORES_TEST_ASSERT(!(singleValueRange != unionRange), "Union not equal");

  std::cout << "Low range." << std::endl;
  viskores::Range lowRange(-10.0, -5.0);
  VISKORES_TEST_ASSERT(lowRange.IsNonEmpty(), "Empty?");
  VISKORES_TEST_ASSERT(test_equal(lowRange.Length(), 5.0), "Bad length.");
  VISKORES_TEST_ASSERT(test_equal(lowRange.Center(), -7.5), "Bad center.");
  VISKORES_TEST_ASSERT(!lowRange.Contains(-20), "Contains fail");
  VISKORES_TEST_ASSERT(lowRange.Contains(-7), "Contains fail");
  VISKORES_TEST_ASSERT(!lowRange.Contains(0), "Contains fail");
  VISKORES_TEST_ASSERT(!lowRange.Contains(10), "Contains fail");

  unionRange = singleValueRange + lowRange;
  VISKORES_TEST_ASSERT(unionRange.IsNonEmpty(), "Empty?");
  VISKORES_TEST_ASSERT(test_equal(unionRange.Length(), 15.0), "Bad length.");
  VISKORES_TEST_ASSERT(test_equal(unionRange.Center(), -2.5), "Bad center.");
  VISKORES_TEST_ASSERT(!unionRange.Contains(-20), "Contains fail");
  VISKORES_TEST_ASSERT(unionRange.Contains(-7), "Contains fail");
  VISKORES_TEST_ASSERT(unionRange.Contains(0), "Contains fail");
  VISKORES_TEST_ASSERT(!unionRange.Contains(10), "Contains fail");

  std::cout << "High range." << std::endl;
  viskores::Range highRange(15.0, 20.0);
  VISKORES_TEST_ASSERT(highRange.IsNonEmpty(), "Empty?");
  VISKORES_TEST_ASSERT(test_equal(highRange.Length(), 5.0), "Bad length.");
  VISKORES_TEST_ASSERT(test_equal(highRange.Center(), 17.5), "Bad center.");
  VISKORES_TEST_ASSERT(!highRange.Contains(-20), "Contains fail");
  VISKORES_TEST_ASSERT(!highRange.Contains(-7), "Contains fail");
  VISKORES_TEST_ASSERT(!highRange.Contains(0), "Contains fail");
  VISKORES_TEST_ASSERT(!highRange.Contains(10), "Contains fail");
  VISKORES_TEST_ASSERT(highRange.Contains(17), "Contains fail");
  VISKORES_TEST_ASSERT(!highRange.Contains(25), "Contains fail");

  unionRange = highRange.Union(singleValueRange);
  VISKORES_TEST_ASSERT(unionRange.IsNonEmpty(), "Empty?");
  VISKORES_TEST_ASSERT(test_equal(unionRange.Length(), 15.0), "Bad length.");
  VISKORES_TEST_ASSERT(test_equal(unionRange.Center(), 12.5), "Bad center.");
  VISKORES_TEST_ASSERT(!unionRange.Contains(-20), "Contains fail");
  VISKORES_TEST_ASSERT(!unionRange.Contains(-7), "Contains fail");
  VISKORES_TEST_ASSERT(!unionRange.Contains(0), "Contains fail");
  VISKORES_TEST_ASSERT(unionRange.Contains(10), "Contains fail");
  VISKORES_TEST_ASSERT(unionRange.Contains(17), "Contains fail");
  VISKORES_TEST_ASSERT(!unionRange.Contains(25), "Contains fail");

  unionRange.Include(-1);
  VISKORES_TEST_ASSERT(unionRange.IsNonEmpty(), "Empty?");
  VISKORES_TEST_ASSERT(test_equal(unionRange.Length(), 21.0), "Bad length.");
  VISKORES_TEST_ASSERT(test_equal(unionRange.Center(), 9.5), "Bad center.");
  VISKORES_TEST_ASSERT(!unionRange.Contains(-20), "Contains fail");
  VISKORES_TEST_ASSERT(!unionRange.Contains(-7), "Contains fail");
  VISKORES_TEST_ASSERT(unionRange.Contains(0), "Contains fail");
  VISKORES_TEST_ASSERT(unionRange.Contains(10), "Contains fail");
  VISKORES_TEST_ASSERT(unionRange.Contains(17), "Contains fail");
  VISKORES_TEST_ASSERT(!unionRange.Contains(25), "Contains fail");

  unionRange.Include(lowRange);
  VISKORES_TEST_ASSERT(unionRange.IsNonEmpty(), "Empty?");
  VISKORES_TEST_ASSERT(test_equal(unionRange.Length(), 30.0), "Bad length.");
  VISKORES_TEST_ASSERT(test_equal(unionRange.Center(), 5), "Bad center.");
  VISKORES_TEST_ASSERT(!unionRange.Contains(-20), "Contains fail");
  VISKORES_TEST_ASSERT(unionRange.Contains(-7), "Contains fail");
  VISKORES_TEST_ASSERT(unionRange.Contains(0), "Contains fail");
  VISKORES_TEST_ASSERT(unionRange.Contains(10), "Contains fail");
  VISKORES_TEST_ASSERT(unionRange.Contains(17), "Contains fail");
  VISKORES_TEST_ASSERT(!unionRange.Contains(25), "Contains fail");

  std::cout << "Try adding infinity." << std::endl;
  unionRange.Include(viskores::Infinity64());
  VISKORES_TEST_ASSERT(unionRange.IsNonEmpty(), "Empty?");
  VISKORES_TEST_ASSERT(!unionRange.Contains(-20), "Contains fail");
  VISKORES_TEST_ASSERT(unionRange.Contains(-7), "Contains fail");
  VISKORES_TEST_ASSERT(unionRange.Contains(0), "Contains fail");
  VISKORES_TEST_ASSERT(unionRange.Contains(10), "Contains fail");
  VISKORES_TEST_ASSERT(unionRange.Contains(17), "Contains fail");
  VISKORES_TEST_ASSERT(unionRange.Contains(25), "Contains fail");

  std::cout << "Try adding NaN." << std::endl;
  // Turn off floating point exceptions. This is only for conditions that allow NaNs.
  viskores::testing::FloatingPointExceptionTrapDisable();
  unionRange.Include(viskores::Nan64());
  VISKORES_TEST_ASSERT(unionRange.IsNonEmpty(), "Empty?");
  VISKORES_TEST_ASSERT(!unionRange.Contains(-20), "Contains fail");
  VISKORES_TEST_ASSERT(unionRange.Contains(-7), "Contains fail");
  VISKORES_TEST_ASSERT(unionRange.Contains(0), "Contains fail");
  VISKORES_TEST_ASSERT(unionRange.Contains(10), "Contains fail");
  VISKORES_TEST_ASSERT(unionRange.Contains(17), "Contains fail");
  VISKORES_TEST_ASSERT(unionRange.Contains(25), "Contains fail");

  std::cout << "Try VecTraits." << std::endl;
  using VTraits = viskores::VecTraits<viskores::Range>;
  VISKORES_TEST_ASSERT(VTraits::NUM_COMPONENTS == 2);
  viskores::Range simpleRange(2.0, 4.0);
  VISKORES_TEST_ASSERT(VTraits::GetNumberOfComponents(simpleRange) == 2);
  VISKORES_TEST_ASSERT(VTraits::GetComponent(simpleRange, 0) == 2.0);
  VISKORES_TEST_ASSERT(VTraits::GetComponent(simpleRange, 1) == 4.0);
  viskores::Vec2f_64 simpleRangeCopy;
  VTraits::CopyInto(simpleRange, simpleRangeCopy);
  VISKORES_TEST_ASSERT(simpleRangeCopy == viskores::Vec2f_64{ 2.0, 4.0 });
  VTraits::SetComponent(simpleRange, 0, 1.0);
  VTraits::SetComponent(simpleRange, 1, 2.0);
  VISKORES_TEST_ASSERT(!simpleRange.Contains(0.0));
  VISKORES_TEST_ASSERT(simpleRange.Contains(1.5));
  VISKORES_TEST_ASSERT(!simpleRange.Contains(3.0));
}

} // anonymous namespace

int UnitTestRange(int argc, char* argv[])
{
  return viskores::testing::Testing::Run(TestRange, argc, argv);
}
