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

#include <viskores/Types.h>
#include <viskores/cont/CoordinateSystem.h>
#include <viskores/cont/Field.h>
#include <viskores/cont/testing/Testing.h>

#include <algorithm>
#include <iostream>
#include <random>

namespace
{

template <typename T>
void TestScalarField()
{
  const viskores::Id nvals = 11;
  T data[nvals] = { 1, 2, 3, 4, 5, -5, -4, -3, -2, -1, 0 };
  std::random_device rng;
  std::mt19937 urng(rng());
  std::shuffle(data, data + nvals, urng);
  auto field = viskores::cont::make_Field(
    "TestField", viskores::cont::Field::Association::Points, data, nvals, viskores::CopyFlag::Off);

  viskores::Range result;
  field.GetRange(&result);

  std::cout << result << std::endl;
  VISKORES_TEST_ASSERT((test_equal(result.Min, -5.0) && test_equal(result.Max, 5.0)),
                       "Unexpected scalar field range.");
}

template <typename T, viskores::IdComponent NumberOfComponents>
void TestVecField()
{
  const viskores::Id nvals = 11;
  T data[nvals] = { 1, 2, 3, 4, 5, -5, -4, -3, -2, -1, 0 };
  viskores::Vec<T, NumberOfComponents> fieldData[nvals];
  std::random_device rng;
  std::mt19937 urng(rng());
  for (viskores::IdComponent i = 0; i < NumberOfComponents; ++i)
  {
    std::shuffle(data, data + nvals, urng);
    for (viskores::Id j = 0; j < nvals; ++j)
    {
      fieldData[j][i] = data[j];
    }
  }
  auto field = viskores::cont::make_Field("TestField",
                                          viskores::cont::Field::Association::Points,
                                          fieldData,
                                          nvals,
                                          viskores::CopyFlag::Off);

  viskores::Range result[NumberOfComponents];
  field.GetRange(result);

  for (viskores::IdComponent i = 0; i < NumberOfComponents; ++i)
  {
    VISKORES_TEST_ASSERT((test_equal(result[i].Min, -5.0) && test_equal(result[i].Max, 5.0)),
                         "Unexpected vector field range.");
  }
}

void TestUniformCoordinateField()
{
  viskores::cont::CoordinateSystem field("TestField",
                                         viskores::Id3(10, 20, 5),
                                         viskores::Vec3f(0.0f, -5.0f, 4.0f),
                                         viskores::Vec3f(1.0f, 0.5f, 2.0f));

  viskores::Bounds result = field.GetBounds();

  VISKORES_TEST_ASSERT(test_equal(result.X.Min, 0.0), "Min x wrong.");
  VISKORES_TEST_ASSERT(test_equal(result.X.Max, 9.0), "Max x wrong.");
  VISKORES_TEST_ASSERT(test_equal(result.Y.Min, -5.0), "Min y wrong.");
  VISKORES_TEST_ASSERT(test_equal(result.Y.Max, 4.5), "Max y wrong.");
  VISKORES_TEST_ASSERT(test_equal(result.Z.Min, 4.0), "Min z wrong.");
  VISKORES_TEST_ASSERT(test_equal(result.Z.Max, 12.0), "Max z wrong.");
}

void TestAll()
{
  std::cout << "Testing (Int32, 1)..." << std::endl;
  TestScalarField<viskores::Int32>();
  std::cout << "Testing (Int64, 1)..." << std::endl;
  TestScalarField<viskores::Int64>();
  std::cout << "Testing (Float32, 1)..." << std::endl;
  TestScalarField<viskores::Float32>();
  std::cout << "Testing (Float64, 1)..." << std::endl;
  TestScalarField<viskores::Float64>();

  std::cout << "Testing (Int32, 3)..." << std::endl;
  TestVecField<viskores::Int32, 3>();
  std::cout << "Testing (Int64, 3)..." << std::endl;
  TestVecField<viskores::Int64, 3>();
  std::cout << "Testing (Float32, 3)..." << std::endl;
  TestVecField<viskores::Float32, 3>();
  std::cout << "Testing (Float64, 3)..." << std::endl;
  TestVecField<viskores::Float64, 3>();

  std::cout << "Testing (Int32, 9)..." << std::endl;
  TestVecField<viskores::Int32, 9>();
  std::cout << "Testing (Int64, 9)..." << std::endl;
  TestVecField<viskores::Int64, 9>();
  std::cout << "Testing (Float32, 9)..." << std::endl;
  TestVecField<viskores::Float32, 9>();
  std::cout << "Testing (Float64, 9)..." << std::endl;
  TestVecField<viskores::Float64, 9>();

  std::cout << "Testing UniformPointCoords..." << std::endl;
  TestUniformCoordinateField();
}

} // anonymous namespace

int UnitTestComputeRange(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestAll, argc, argv);
}
