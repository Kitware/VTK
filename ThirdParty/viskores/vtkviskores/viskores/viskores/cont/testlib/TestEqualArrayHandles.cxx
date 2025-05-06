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

#include <viskores/cont/testing/Testing.h>

namespace
{

struct TestEqualArrayHandleType2
{
  template <typename T, typename FirstArrayType>
  void operator()(T,
                  const FirstArrayType& array1,
                  const viskores::cont::UnknownArrayHandle& array2,
                  viskores::IdComponent cIndex,
                  TestEqualResult& result,
                  bool& called) const
  {
    if (!array2.IsBaseComponentType<T>())
    {
      return;
    }

    result = test_equal_ArrayHandles(array1, array2.ExtractComponent<T>(cIndex));

    called = true;
  }
};

struct TestEqualArrayHandleType1
{
  template <typename T>
  void operator()(T,
                  const viskores::cont::UnknownArrayHandle& array1,
                  const viskores::cont::UnknownArrayHandle& array2,
                  TestEqualResult& result,
                  bool& called) const
  {
    if (!array1.IsBaseComponentType<T>())
    {
      return;
    }

    for (viskores::IdComponent cIndex = 0; cIndex < array1.GetNumberOfComponentsFlat(); ++cIndex)
    {
      viskores::ListForEach(TestEqualArrayHandleType2{},
                            viskores::TypeListScalarAll{},
                            array1.ExtractComponent<T>(cIndex),
                            array2,
                            cIndex,
                            result,
                            called);
      if (!result)
      {
        break;
      }
    }
  }
};

} // anonymous namespace

TestEqualResult test_equal_ArrayHandles(const viskores::cont::UnknownArrayHandle& array1,
                                        const viskores::cont::UnknownArrayHandle& array2)
{
  TestEqualResult result;

  if (array1.GetNumberOfComponentsFlat() != array2.GetNumberOfComponentsFlat())
  {
    result.PushMessage("Arrays have different numbers of components.");
    return result;
  }

  bool called = false;

  viskores::ListForEach(
    TestEqualArrayHandleType1{}, viskores::TypeListScalarAll{}, array1, array2, result, called);

  if (!called)
  {
    result.PushMessage("Could not base component type for " + array1.GetBaseComponentTypeName() +
                       " or " + array2.GetBaseComponentTypeName());
  }
  return result;
}
