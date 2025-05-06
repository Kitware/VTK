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

#include <viskores/filter/MapFieldMergeAverage.h>

#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleCartesianProduct.h>
#include <viskores/cont/ArrayHandleUniformPointCoordinates.h>
#include <viskores/cont/Field.h>

#include <viskores/cont/testing/Testing.h>

namespace
{

constexpr viskores::Id ARRAY_SIZE = 26;
constexpr viskores::Id3 ARRAY3_DIM = { 3, 3, 3 };
constexpr viskores::Id REDUCED_SIZE = 7;

viskores::worklet::Keys<viskores::Id> MakeKeys(viskores::Id originalArraySize)
{
  viskores::cont::ArrayHandle<viskores::Id> keyArray;
  keyArray.Allocate(originalArraySize);
  {
    auto portal = keyArray.WritePortal();
    for (viskores::Id i = 0; i < originalArraySize; ++i)
    {
      portal.Set(i, i % REDUCED_SIZE);
    }
  }

  return viskores::worklet::Keys<viskores::Id>(keyArray);
}

// Make an array of the expected output of mapping the given array using the keys returned from
// MakeKeys but with a different mechanism.
template <typename T, typename S>
viskores::cont::ArrayHandle<T> MakeExpectedOutput(
  const viskores::cont::ArrayHandle<T, S>& inputArray)
{
  using ComponentType = typename viskores::VecTraits<T>::ComponentType;

  auto inputPortal = inputArray.ReadPortal();

  viskores::cont::ArrayHandle<T> outputArray;
  outputArray.Allocate(REDUCED_SIZE);
  auto outputPortal = outputArray.WritePortal();

  for (viskores::Id reducedI = 0; reducedI < REDUCED_SIZE; ++reducedI)
  {
    T sum = viskores::TypeTraits<T>::ZeroInitialization();
    ComponentType num = 0;
    for (viskores::Id fullI = reducedI; fullI < inputArray.GetNumberOfValues();
         fullI += REDUCED_SIZE)
    {
      sum = static_cast<T>(sum + inputPortal.Get(fullI));
      num = static_cast<ComponentType>(num + ComponentType(1));
    }
    outputPortal.Set(reducedI, sum / T(num));
  }

  return outputArray;
}

template <typename T, typename S>
void TryArray(const viskores::cont::ArrayHandle<T, S>& inputArray)
{
  std::cout << "Input" << std::endl;
  viskores::cont::printSummary_ArrayHandle(inputArray, std::cout);

  viskores::cont::Field::Association association =
    ((sizeof(T) < 8) ? viskores::cont::Field::Association::Points
                     : viskores::cont::Field::Association::Cells);

  viskores::cont::Field inputField("my-array", association, inputArray);

  viskores::worklet::Keys<viskores::Id> keys = MakeKeys(inputArray.GetNumberOfValues());

  viskores::cont::ArrayHandle<T> expectedOutputArray = MakeExpectedOutput(inputArray);
  std::cout << "Expected output" << std::endl;
  viskores::cont::printSummary_ArrayHandle(expectedOutputArray, std::cout);

  viskores::cont::Field outputField;
  bool result = viskores::filter::MapFieldMergeAverage(inputField, keys, outputField);
  VISKORES_TEST_ASSERT(result, "Could not map the array.");

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

int UnitTestMapFieldMergeAverage(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(DoTest, argc, argv);
}
