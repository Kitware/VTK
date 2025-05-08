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

#include <viskores/cont/ArrayHandleReverse.h>

#include <viskores/cont/ArrayHandleIndex.h>
#include <viskores/cont/serial/DeviceAdapterSerial.h>
#include <viskores/cont/testing/Testing.h>

namespace UnitTestArrayHandleReverseNamespace
{

const viskores::Id ARRAY_SIZE = 10;

void TestArrayHandleReverseRead()
{
  viskores::cont::ArrayHandleIndex array(ARRAY_SIZE);
  VISKORES_TEST_ASSERT(array.GetNumberOfValues() == ARRAY_SIZE, "Bad size.");
  VISKORES_TEST_ASSERT(array.GetNumberOfComponentsFlat() == 1);

  auto portal = array.ReadPortal();
  for (viskores::Id index = 0; index < ARRAY_SIZE; index++)
  {
    VISKORES_TEST_ASSERT(portal.Get(index) == index, "Index array has unexpected value.");
  }

  viskores::cont::ArrayHandleReverse<viskores::cont::ArrayHandleIndex> reverse =
    viskores::cont::make_ArrayHandleReverse(array);

  auto reversedPortal = reverse.ReadPortal();
  for (viskores::Id index = 0; index < ARRAY_SIZE; index++)
  {
    VISKORES_TEST_ASSERT(reversedPortal.Get(index) == portal.Get(9 - index),
                         "ArrayHandleReverse does not reverse array");
  }
}

void TestArrayHandleReverseWrite()
{
  std::vector<viskores::Id> ids(ARRAY_SIZE, 0);
  viskores::cont::ArrayHandle<viskores::Id> handle =
    viskores::cont::make_ArrayHandle(ids, viskores::CopyFlag::Off);

  viskores::cont::ArrayHandleReverse<viskores::cont::ArrayHandle<viskores::Id>> reverse =
    viskores::cont::make_ArrayHandleReverse(handle);

  for (viskores::Id index = 0; index < ARRAY_SIZE; index++)
  {
    reverse.WritePortal().Set(index, index);
  }

  auto portal = handle.ReadPortal();
  for (viskores::Id index = 0; index < ARRAY_SIZE; index++)
  {
    VISKORES_TEST_ASSERT(portal.Get(index) == (9 - index),
                         "ArrayHandleReverse does not reverse array");
  }
}

void TestArrayHandleReverseScanInclusiveByKey()
{
  viskores::cont::ArrayHandle<viskores::Id> values =
    viskores::cont::make_ArrayHandle<viskores::Id>({ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 });
  viskores::cont::ArrayHandle<viskores::Id> keys =
    viskores::cont::make_ArrayHandle<viskores::Id>({ 0, 0, 0, 0, 1, 1, 2, 3, 3, 4 });

  viskores::cont::ArrayHandle<viskores::Id> output;
  viskores::cont::ArrayHandleReverse<viskores::cont::ArrayHandle<viskores::Id>> reversed =
    viskores::cont::make_ArrayHandleReverse(output);

  using Algorithm = viskores::cont::DeviceAdapterAlgorithm<viskores::cont::DeviceAdapterTagSerial>;
  Algorithm::ScanInclusiveByKey(keys, values, reversed);

  viskores::cont::ArrayHandleReverse<viskores::cont::ArrayHandle<viskores::Id>> expected_reversed =
    viskores::cont::make_ArrayHandleReverse(
      viskores::cont::make_ArrayHandle<viskores::Id>({ 0, 1, 3, 6, 4, 9, 6, 7, 15, 9 }));
  auto outputPortal = output.ReadPortal();
  auto reversePortal = expected_reversed.ReadPortal();
  for (int i = 0; i < 10; i++)
  {
    VISKORES_TEST_ASSERT(outputPortal.Get(i) == reversePortal.Get(i),
                         "ArrayHandleReverse as output of ScanInclusiveByKey");
  }
  std::cout << std::endl;
}

void TestArrayHandleReverseFill()
{
  viskores::cont::ArrayHandle<viskores::Id> handle;
  auto reverse = viskores::cont::make_ArrayHandleReverse(handle);

  reverse.AllocateAndFill(ARRAY_SIZE, 20, viskores::CopyFlag::Off);
  VISKORES_TEST_ASSERT(reverse.GetNumberOfValues() == ARRAY_SIZE);
  auto portal = reverse.ReadPortal();
  for (viskores::Id index = 0; index < ARRAY_SIZE; ++index)
  {
    VISKORES_TEST_ASSERT(portal.Get(index) == 20);
  }
}

void TestArrayHandleReverse()
{
  TestArrayHandleReverseRead();
  TestArrayHandleReverseWrite();
  TestArrayHandleReverseScanInclusiveByKey();
  TestArrayHandleReverseFill();
}

}; // namespace UnitTestArrayHandleReverseNamespace

int UnitTestArrayHandleReverse(int argc, char* argv[])
{
  using namespace UnitTestArrayHandleReverseNamespace;
  return viskores::cont::testing::Testing::Run(TestArrayHandleReverse, argc, argv);
}
