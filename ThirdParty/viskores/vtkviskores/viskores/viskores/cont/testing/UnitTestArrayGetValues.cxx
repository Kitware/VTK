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

#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayGetValues.h>
#include <viskores/cont/ArrayHandleCast.h>
#include <viskores/cont/ArrayHandleIndex.h>

#include <viskores/Bounds.h>
#include <viskores/Range.h>

#include <viskores/cont/testing/Testing.h>

namespace
{

static constexpr viskores::Id ARRAY_SIZE = 10;

template <typename T>
VISKORES_CONT void TestValues(const viskores::cont::ArrayHandle<T>& ah,
                              const std::initializer_list<T>& expected)
{
  auto portal = ah.ReadPortal();
  VISKORES_TEST_ASSERT(expected.size() == static_cast<size_t>(ah.GetNumberOfValues()));
  for (viskores::Id i = 0; i < ah.GetNumberOfValues(); ++i)
  {
    VISKORES_TEST_ASSERT(expected.begin()[static_cast<size_t>(i)] == portal.Get(i));
  }
}

template <typename T>
VISKORES_CONT void TestValues(const std::vector<T>& vec, const std::initializer_list<T>& expected)
{
  VISKORES_TEST_ASSERT(expected.size() == vec.size());
  for (std::size_t i = 0; i < vec.size(); ++i)
  {
    VISKORES_TEST_ASSERT(expected.begin()[static_cast<size_t>(i)] == vec[i]);
  }
}

template <typename ValueType>
void TryCopy()
{
  std::cout << "Trying type: " << viskores::testing::TypeName<ValueType>::Name() << std::endl;

  viskores::cont::ArrayHandle<ValueType> data;
  { // Create the ValueType array.
    viskores::cont::ArrayHandleIndex values(ARRAY_SIZE);
    viskores::cont::ArrayCopy(values, data);
  }

  { // ArrayHandle ids
    const auto ids = viskores::cont::make_ArrayHandle<viskores::Id>({ 3, 8, 7 });
    { // Return vector:
      const std::vector<ValueType> output = viskores::cont::ArrayGetValues(ids, data);
      TestValues<ValueType>(output, { 3, 8, 7 });
    }
    { // Pass vector:
      std::vector<ValueType> output;
      viskores::cont::ArrayGetValues(ids, data, output);
      TestValues<ValueType>(output, { 3, 8, 7 });
    }
    { // Pass handle:
      viskores::cont::ArrayHandle<ValueType> output;
      viskores::cont::ArrayGetValues(ids, data, output);
      TestValues<ValueType>(output, { 3, 8, 7 });
    }
    { // Test the specialization for ArrayHandleCast
      auto castedData = viskores::cont::make_ArrayHandleCast<viskores::Float64>(data);
      viskores::cont::ArrayHandle<viskores::Float64> output;
      viskores::cont::ArrayGetValues(ids, castedData, output);
      TestValues<viskores::Float64>(output, { 3.0, 8.0, 7.0 });
    }
  }

  { // vector ids
    const std::vector<viskores::Id> ids{ 1, 5, 3, 9 };
    { // Return vector:
      const std::vector<ValueType> output = viskores::cont::ArrayGetValues(ids, data);
      TestValues<ValueType>(output, { 1, 5, 3, 9 });
    }
    { // Pass vector:
      std::vector<ValueType> output;
      viskores::cont::ArrayGetValues(ids, data, output);
      TestValues<ValueType>(output, { 1, 5, 3, 9 });
    }
    { // Pass handle:
      viskores::cont::ArrayHandle<ValueType> output;
      viskores::cont::ArrayGetValues(ids, data, output);
      TestValues<ValueType>(output, { 1, 5, 3, 9 });
    }
  }

  {   // Initializer list ids
    { // Return vector:
      const std::vector<ValueType> output = viskores::cont::ArrayGetValues({ 4, 2, 0, 6, 9 }, data);
  TestValues<ValueType>(output, { 4, 2, 0, 6, 9 });
}
{ // Pass vector:
  std::vector<ValueType> output;
  viskores::cont::ArrayGetValues({ 4, 2, 0, 6, 9 }, data, output);
  TestValues<ValueType>(output, { 4, 2, 0, 6, 9 });
}
{ // Pass handle:
  viskores::cont::ArrayHandle<ValueType> output;
  viskores::cont::ArrayGetValues({ 4, 2, 0, 6, 9 }, data, output);
  TestValues<ValueType>(output, { 4, 2, 0, 6, 9 });
}
}

{ // c-array ids
  const std::vector<viskores::Id> idVec{ 8, 6, 7, 5, 3, 0, 9 };
  const viskores::Id* ids = idVec.data();
  const viskores::Id n = static_cast<viskores::Id>(idVec.size());
  { // Return vector:
    const std::vector<ValueType> output = viskores::cont::ArrayGetValues(ids, n, data);
    TestValues<ValueType>(output, { 8, 6, 7, 5, 3, 0, 9 });
  }
  { // Pass vector:
    std::vector<ValueType> output;
    viskores::cont::ArrayGetValues(ids, n, data, output);
    TestValues<ValueType>(output, { 8, 6, 7, 5, 3, 0, 9 });
  }
  { // Pass handle:
    viskores::cont::ArrayHandle<ValueType> output;
    viskores::cont::ArrayGetValues(ids, n, data, output);
    TestValues<ValueType>(output, { 8, 6, 7, 5, 3, 0, 9 });
  }
}

{ // single values
  {
    const ValueType output = viskores::cont::ArrayGetValue(8, data);
    VISKORES_TEST_ASSERT(output == static_cast<ValueType>(8));
  }
  {
    ValueType output;
    viskores::cont::ArrayGetValue(8, data, output);
    VISKORES_TEST_ASSERT(output == static_cast<ValueType>(8));
  }
}
}

void TryRange()
{
  std::cout << "Trying viskores::Range" << std::endl;

  viskores::cont::ArrayHandle<viskores::Range> values =
    viskores::cont::make_ArrayHandle<viskores::Range>({ { 0.0, 1.0 }, { 1.0, 2.0 }, { 2.0, 4.0 } });
  viskores::Range range = viskores::cont::ArrayGetValue(1, values);
  VISKORES_TEST_ASSERT(range == viskores::Range{ 1.0, 2.0 });
}

void TryBounds()
{
  std::cout << "Trying viskores::Bounds" << std::endl;

  viskores::cont::ArrayHandle<viskores::Bounds> values =
    viskores::cont::make_ArrayHandle<viskores::Bounds>(
      { { { 0.0, 1.0 }, { 0.0, 1.0 }, { 0.0, 1.0 } },
        { { 1.0, 2.0 }, { 1.0, 2.0 }, { 1.0, 2.0 } },
        { { 2.0, 4.0 }, { 2.0, 4.0 }, { 2.0, 4.0 } } });
  viskores::Bounds bounds = viskores::cont::ArrayGetValue(1, values);
  VISKORES_TEST_ASSERT(bounds == viskores::Bounds{ { 1.0, 2.0 }, { 1.0, 2.0 }, { 1.0, 2.0 } });
}

void Test()
{
  TryCopy<viskores::Id>();
  TryCopy<viskores::IdComponent>();
  TryCopy<viskores::Float32>();
  TryRange();
  TryBounds();
}

} // anonymous namespace

int UnitTestArrayGetValues(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(Test, argc, argv);
}
