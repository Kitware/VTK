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

#include <viskores/filter/MapFieldPermutation.h>

#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleCartesianProduct.h>
#include <viskores/cont/ArrayHandleCounting.h>
#include <viskores/cont/ArrayHandlePermutation.h>
#include <viskores/cont/ArrayHandleUniformPointCoordinates.h>
#include <viskores/cont/Field.h>

#include <viskores/cont/testing/Testing.h>

namespace
{

constexpr viskores::Id ARRAY_SIZE = 26;
constexpr viskores::Id3 ARRAY3_DIM = { 3, 3, 3 };

template <typename T, typename S>
void TryArray(const viskores::cont::ArrayHandle<T, S>& inputArray)
{
  std::cout << "Input" << std::endl;
  viskores::cont::printSummary_ArrayHandle(inputArray, std::cout);

  viskores::cont::Field::Association association =
    ((sizeof(T) < 8) ? viskores::cont::Field::Association::Points
                     : viskores::cont::Field::Association::Cells);

  viskores::cont::Field inputField("my-array", association, inputArray);

  viskores::cont::ArrayHandle<viskores::Id> permutationArray;
  viskores::cont::ArrayCopy(viskores::cont::make_ArrayHandleCounting<viskores::Id>(
                              0, 2, inputArray.GetNumberOfValues() / 2),
                            permutationArray);

  viskores::cont::ArrayHandle<T> expectedOutputArray;
  viskores::cont::ArrayCopy(
    viskores::cont::make_ArrayHandlePermutation(permutationArray, inputArray), expectedOutputArray);
  std::cout << "Expected output" << std::endl;
  viskores::cont::printSummary_ArrayHandle(expectedOutputArray, std::cout);

  viskores::cont::Field outputField;
  bool result = viskores::filter::MapFieldPermutation(inputField, permutationArray, outputField);
  VISKORES_TEST_ASSERT(result, "Could not permute the array.");

  VISKORES_TEST_ASSERT(outputField.GetAssociation() == association);
  VISKORES_TEST_ASSERT(outputField.GetName() == "my-array");

  viskores::cont::ArrayHandle<T> outputArray;
  outputField.GetData().AsArrayHandle(outputArray);
  std::cout << "Actual output" << std::endl;
  viskores::cont::printSummary_ArrayHandle(outputArray, std::cout);

  VISKORES_TEST_ASSERT(
    test_equal_portals(expectedOutputArray.ReadPortal(), outputArray.ReadPortal()));
}

template <typename T>
void TryType(T)
{
  viskores::cont::ArrayHandle<T> inputArray;
  inputArray.Allocate(ARRAY_SIZE);
  SetPortal(inputArray.WritePortal());
  TryArray(inputArray);
}

struct TryTypeFunctor
{
  template <typename T>
  void operator()(T x) const
  {
    TryType(x);
  }
};

void TryCartesianProduct()
{
  viskores::cont::ArrayHandle<viskores::FloatDefault> axes[3];
  for (viskores::IdComponent i = 0; i < 3; ++i)
  {
    axes[i].Allocate(ARRAY3_DIM[i]);
    SetPortal(axes[i].WritePortal());
  }

  TryArray(viskores::cont::make_ArrayHandleCartesianProduct(axes[0], axes[1], axes[2]));
}

void DoTest()
{
  std::cout << "**** Test Basic Arrays *****" << std::endl;
  viskores::testing::Testing::TryTypes(TryTypeFunctor{});

  std::cout << std::endl << "**** Test Uniform Point Coordiantes *****" << std::endl;
  TryArray(viskores::cont::ArrayHandleUniformPointCoordinates(ARRAY3_DIM));

  std::cout << std::endl << "**** Test Cartesian Product *****" << std::endl;
  TryCartesianProduct();
}

} // anonymous namespace

int UnitTestMapFieldPermutation(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(DoTest, argc, argv);
}
