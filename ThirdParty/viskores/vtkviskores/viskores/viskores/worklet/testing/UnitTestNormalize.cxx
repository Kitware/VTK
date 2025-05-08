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

#include <viskores/worklet/DispatcherMapField.h>
#include <viskores/worklet/Normalize.h>

#include <viskores/cont/testing/Testing.h>

namespace
{

template <typename T>
void createVectors(std::vector<viskores::Vec<T, 3>>& vecs)
{
  vecs.push_back(viskores::make_Vec(2, 0, 0));
  vecs.push_back(viskores::make_Vec(0, 2, 0));
  vecs.push_back(viskores::make_Vec(0, 0, 2));
  vecs.push_back(viskores::make_Vec(1, 1, 1));
  vecs.push_back(viskores::make_Vec(2, 2, 2));
  vecs.push_back(viskores::make_Vec(2, 1, 1));

  vecs.push_back(viskores::make_Vec(1000000, 0, 0));

  vecs.push_back(viskores::make_Vec(static_cast<T>(.1), static_cast<T>(0), static_cast<T>(0)));
  vecs.push_back(viskores::make_Vec(static_cast<T>(.001), static_cast<T>(0), static_cast<T>(0)));
}

template <typename T>
void createVectors(std::vector<viskores::Vec<T, 2>>& vecs)
{
  vecs.push_back(viskores::make_Vec(1, 0));
  vecs.push_back(viskores::make_Vec(0, 1));
  vecs.push_back(viskores::make_Vec(1, 1));
  vecs.push_back(viskores::make_Vec(2, 0));
  vecs.push_back(viskores::make_Vec(0, 2));
  vecs.push_back(viskores::make_Vec(2, 2));

  vecs.push_back(viskores::make_Vec(1000000, 0));

  vecs.push_back(viskores::make_Vec(static_cast<T>(.1), static_cast<T>(0)));
  vecs.push_back(viskores::make_Vec(static_cast<T>(.001), static_cast<T>(0)));
}

template <typename T, int N>
void TestNormal()
{
  std::vector<viskores::Vec<T, N>> inputVecs;
  createVectors(inputVecs);

  viskores::cont::ArrayHandle<viskores::Vec<T, N>> inputArray;
  viskores::cont::ArrayHandle<viskores::Vec<T, N>> outputArray;
  inputArray = viskores::cont::make_ArrayHandle(inputVecs, viskores::CopyFlag::On);

  viskores::worklet::Normal normalWorklet;
  viskores::worklet::DispatcherMapField<viskores::worklet::Normal> dispatcherNormal(normalWorklet);
  dispatcherNormal.Invoke(inputArray, outputArray);

  //Validate results.

  //Make sure the number of values match.
  VISKORES_TEST_ASSERT(outputArray.GetNumberOfValues() == inputArray.GetNumberOfValues(),
                       "Wrong number of results for Normalize worklet");

  //Make sure each vector is correct.
  for (viskores::Id i = 0; i < inputArray.GetNumberOfValues(); i++)
  {
    //Make sure that the value is correct.
    viskores::Vec<T, N> v = inputArray.ReadPortal().Get(i);
    viskores::Vec<T, N> vN = outputArray.ReadPortal().Get(i);
    T len = viskores::Magnitude(v);
    VISKORES_TEST_ASSERT(test_equal(v / len, vN), "Wrong result for Normalize worklet");

    //Make sure the magnitudes are all 1.0
    len = viskores::Magnitude(vN);
    VISKORES_TEST_ASSERT(test_equal(len, 1), "Wrong magnitude for Normalize worklet");
  }
}

template <typename T, int N>
void TestNormalize()
{
  std::vector<viskores::Vec<T, N>> inputVecs;
  createVectors(inputVecs);

  viskores::cont::ArrayHandle<viskores::Vec<T, N>> inputArray;
  inputArray = viskores::cont::make_ArrayHandle(inputVecs, viskores::CopyFlag::On);

  viskores::worklet::Normalize normalizeWorklet;
  viskores::worklet::DispatcherMapField<viskores::worklet::Normalize> dispatcherNormalize(
    normalizeWorklet);
  dispatcherNormalize.Invoke(inputArray);

  //Make sure each vector is correct.
  for (viskores::Id i = 0; i < inputArray.GetNumberOfValues(); i++)
  {
    //Make sure that the value is correct.
    viskores::Vec<T, N> v = inputVecs[static_cast<std::size_t>(i)];
    viskores::Vec<T, N> vN = inputArray.ReadPortal().Get(i);
    T len = viskores::Magnitude(v);
    VISKORES_TEST_ASSERT(test_equal(v / len, vN), "Wrong result for Normalize worklet");

    //Make sure the magnitudes are all 1.0
    len = viskores::Magnitude(vN);
    VISKORES_TEST_ASSERT(test_equal(len, 1), "Wrong magnitude for Normalize worklet");
  }
}

void TestNormalWorklets()
{
  std::cout << "Testing Normal Worklet" << std::endl;

  TestNormal<viskores::Float32, 2>();
  TestNormal<viskores::Float64, 2>();
  TestNormal<viskores::Float32, 3>();
  TestNormal<viskores::Float64, 3>();

  std::cout << "Testing Normalize Worklet" << std::endl;
  TestNormalize<viskores::Float32, 2>();
  TestNormalize<viskores::Float64, 2>();
  TestNormalize<viskores::Float32, 3>();
  TestNormalize<viskores::Float64, 3>();
}
}

int UnitTestNormalize(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestNormalWorklets, argc, argv);
}
