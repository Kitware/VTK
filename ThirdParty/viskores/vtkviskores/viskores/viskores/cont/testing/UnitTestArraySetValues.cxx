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
#include <viskores/cont/ArrayHandleCast.h>
#include <viskores/cont/ArrayHandleIndex.h>
#include <viskores/cont/ArraySetValues.h>

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

template <typename ValueType>
void TryCopy()
{
  std::cout << "Trying type: " << viskores::testing::TypeName<ValueType>::Name() << std::endl;

  auto createData = []() -> viskores::cont::ArrayHandle<ValueType>
  {
    viskores::cont::ArrayHandle<ValueType> data;
    // Create and initialize the ValueType array
    viskores::cont::ArrayHandleIndex values(ARRAY_SIZE);
    viskores::cont::ArrayCopy(values, data);
    return data;
  };

  { // ArrayHandle ids
    const auto ids = viskores::cont::make_ArrayHandle<viskores::Id>({ 3, 8, 7 });
    { // Pass vector
      const auto data = createData();
      std::vector<ValueType> values{ 30, 80, 70 };
      viskores::cont::ArraySetValues(ids, values, data);
      TestValues<ValueType>(data, { 0, 1, 2, 30, 4, 5, 6, 70, 80, 9 });
    }
    { // Pass Handle
      const auto data = createData();
      const auto newValues = viskores::cont::make_ArrayHandle<ValueType>({ 30, 80, 70 });
      viskores::cont::ArraySetValues(ids, newValues, data);
      TestValues<ValueType>(data, { 0, 1, 2, 30, 4, 5, 6, 70, 80, 9 });
    }
    { // Test the specialization for ArrayHandleCast
      const auto data = createData();
      auto castedData = viskores::cont::make_ArrayHandleCast<viskores::Float64>(data);
      const auto doubleValues =
        viskores::cont::make_ArrayHandle<viskores::Float64>({ 3.0, 8.0, 7.0 });
      viskores::cont::ArraySetValues(ids, doubleValues, castedData);
      TestValues<ValueType>(data, { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 });
    }
  }

  { // vector ids
    const std::vector<viskores::Id> ids{ 3, 8, 7 };
    { // Pass vector
      const auto data = createData();
      const std::vector<ValueType> values{ 30, 80, 70 };
      viskores::cont::ArraySetValues(ids, values, data);
      TestValues<ValueType>(data, { 0, 1, 2, 30, 4, 5, 6, 70, 80, 9 });
    }
    { // Pass handle
      const auto data = createData();
      const auto newValues = viskores::cont::make_ArrayHandle<ValueType>({ 30, 80, 70 });
      viskores::cont::ArraySetValues(ids, newValues, data);
      TestValues<ValueType>(data, { 0, 1, 2, 30, 4, 5, 6, 70, 80, 9 });
    }
  }

  {   // Initializer list ids
    { // Pass vector:
      const auto data = createData();
  const std::vector<ValueType> values{ 30, 80, 70 };
  viskores::cont::ArraySetValues({ 3, 8, 7 }, values, data);
  TestValues<ValueType>(data, { 0, 1, 2, 30, 4, 5, 6, 70, 80, 9 });
}
{ // Pass initializer list
  const auto data = createData();
  viskores::cont::ArraySetValues(
    { 3, 8, 7 },
    { static_cast<ValueType>(30), static_cast<ValueType>(80), static_cast<ValueType>(70) },
    data);
  TestValues<ValueType>(data, { 0, 1, 2, 30, 4, 5, 6, 70, 80, 9 });
}
{ // Pass handle:
  const auto data = createData();
  const auto newValues = viskores::cont::make_ArrayHandle<ValueType>({ 30, 80, 70 });
  viskores::cont::ArraySetValues({ 3, 8, 7 }, newValues, data);
  TestValues<ValueType>(data, { 0, 1, 2, 30, 4, 5, 6, 70, 80, 9 });
}
}

{ // c-array ids
  const std::vector<viskores::Id> idVec{ 3, 8, 7 };
  const viskores::Id* ids = idVec.data();
  const auto numIds = static_cast<viskores::Id>(idVec.size());
  const std::vector<ValueType> valueVec{ 30, 80, 70 };
  const ValueType* values = valueVec.data();
  const auto nValues = static_cast<viskores::Id>(valueVec.size());
  { // Pass c-array
    const auto data = createData();
    viskores::cont::ArraySetValues(ids, numIds, values, nValues, data);
    TestValues<ValueType>(data, { 0, 1, 2, 30, 4, 5, 6, 70, 80, 9 });
  }
  { // Pass vector
    const auto data = createData();
    viskores::cont::ArraySetValues(ids, numIds, valueVec, data);
    TestValues<ValueType>(data, { 0, 1, 2, 30, 4, 5, 6, 70, 80, 9 });
  }
  { // Pass Handle
    const auto data = createData();
    const auto newValues =
      viskores::cont::make_ArrayHandle<ValueType>(valueVec, viskores::CopyFlag::Off);
    viskores::cont::ArraySetValues(ids, numIds, newValues, data);
    TestValues<ValueType>(data, { 0, 1, 2, 30, 4, 5, 6, 70, 80, 9 });
  }
}

{ // single value
  const auto data = createData();
  viskores::cont::ArraySetValue(8, static_cast<ValueType>(88), data);
  TestValues<ValueType>(data, { 0, 1, 2, 3, 4, 5, 6, 7, 88, 9 });
}
}

void TryRange()
{
  std::cout << "Trying viskores::Range" << std::endl;

  viskores::cont::ArrayHandle<viskores::Range> values =
    viskores::cont::make_ArrayHandle<viskores::Range>({ { 0.0, 1.0 }, { 1.0, 2.0 }, { 2.0, 4.0 } });

  viskores::cont::ArraySetValue(1, viskores::Range{ 5.0, 6.0 }, values);
  auto portal = values.ReadPortal();
  VISKORES_TEST_ASSERT(portal.Get(1) == viskores::Range{ 5.0, 6.0 });
}

void TryBounds()
{
  std::cout << "Trying viskores::Bounds" << std::endl;

  viskores::cont::ArrayHandle<viskores::Bounds> values =
    viskores::cont::make_ArrayHandle<viskores::Bounds>(
      { { { 0.0, 1.0 }, { 0.0, 1.0 }, { 0.0, 1.0 } },
        { { 1.0, 2.0 }, { 1.0, 2.0 }, { 1.0, 2.0 } },
        { { 2.0, 4.0 }, { 2.0, 4.0 }, { 2.0, 4.0 } } });

  viskores::cont::ArraySetValue(
    1, viskores::Bounds{ { 5.0, 6.0 }, { 5.0, 6.0 }, { 5.0, 6.0 } }, values);
  auto portal = values.ReadPortal();
  VISKORES_TEST_ASSERT(portal.Get(1) ==
                       viskores::Bounds{ { 5.0, 6.0 }, { 5.0, 6.0 }, { 5.0, 6.0 } });
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

int UnitTestArraySetValues(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(Test, argc, argv);
}
