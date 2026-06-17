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

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayIsMonotonic.h>
#include <viskores/cont/testing/Testing.h>

#include <algorithm>
#include <vector>

namespace
{
template <typename T>
void CheckArray(const std::vector<T>& input)
{
  auto array = viskores::cont::make_ArrayHandle(input, viskores::CopyFlag::Off);
  bool isInc = viskores::cont::ArrayIsMonotonicIncreasing(array);
  bool isDec = viskores::cont::ArrayIsMonotonicDecreasing(array);

  if (input.empty() || input.size() == 1)
  {
    VISKORES_TEST_ASSERT(isInc && isDec,
                         "Array with zero or 1 should be both monotonic increasing and decreasing");
    return;
  }

  VISKORES_TEST_ASSERT(isInc, "Array is not monotonic increasing");
  VISKORES_TEST_ASSERT(!isDec, "Array should not be monotonic decreasing");

  //Check the reverse of the array
  auto copy = input;
  std::reverse(copy.begin(), copy.end());
  array = viskores::cont::make_ArrayHandle(copy, viskores::CopyFlag::Off);
  isInc = viskores::cont::ArrayIsMonotonicIncreasing(array);
  isDec = viskores::cont::ArrayIsMonotonicDecreasing(array);

  VISKORES_TEST_ASSERT(!isInc, "Reversed array should not be monotonic increasing");
  VISKORES_TEST_ASSERT(isDec, "Reversed array is not monotonic decreasing");
}

template <typename OutputType, typename InputType>
std::vector<OutputType> ConvertVec(const std::vector<InputType>& input)
{
  std::vector<OutputType> output;
  output.reserve(input.size());

  for (const auto& value : input)
    output.push_back(static_cast<OutputType>(value));
  return output;
}

void CheckTypes(const std::vector<viskores::Id>& input)
{
  CheckArray(input);
  CheckArray(ConvertVec<viskores::Float32>(input));
  CheckArray(ConvertVec<viskores::Float64>(input));
  CheckArray(ConvertVec<viskores::IdComponent>(input));
}

} //namespace anonymous

void TestArrayIsMonotonic()
{
  CheckTypes({ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 });
  CheckTypes({ -5, -4, -3, -2, -1, 0, 1, 2, 3, 4 });

  //check duplicate values
  CheckTypes({ 0, 1, 1, 2, 3, 4, 4, 5, 6 });
  CheckTypes({ -3, -2, -2, -1, 0, 0, 1, 2, 3 });

  //check empty and single element arrays
  CheckTypes({});
  CheckTypes({ 0 });
}

int UnitTestArrayIsMonotonic(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestArrayIsMonotonic, argc, argv);
}
