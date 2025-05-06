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

#include <viskores/cont/testing/MakeTestDataSet.h>
#include <viskores/cont/testing/Testing.h>

#include <viskores/filter/entity_extraction/Mask.h>

using viskores::cont::testing::MakeTestDataSet;

namespace
{

class TestingMask
{
public:
  static void TestUniform2D()
  {
    std::cout << "Testing mask cells uniform grid :" << std::endl;
    viskores::cont::DataSet dataset = MakeTestDataSet().Make2DUniformDataSet1();

    // Setup and run filter to extract by stride
    viskores::filter::entity_extraction::Mask mask;
    viskores::Id stride = 2;
    mask.SetStride(stride);

    viskores::cont::DataSet output = mask.Execute(dataset);

    VISKORES_TEST_ASSERT(test_equal(output.GetNumberOfCells(), 8), "Wrong result for Mask");


    viskores::cont::ArrayHandle<viskores::Float32> cellFieldArray;
    output.GetField("cellvar").GetData().AsArrayHandle(cellFieldArray);

    VISKORES_TEST_ASSERT(cellFieldArray.GetNumberOfValues() == 8 &&
                           cellFieldArray.ReadPortal().Get(7) == 14.f,
                         "Wrong mask data");
  }

  static void TestUniform3D()
  {
    std::cout << "Testing mask cells uniform grid :" << std::endl;
    viskores::cont::DataSet dataset = MakeTestDataSet().Make3DUniformDataSet1();

    // Setup and run filter to extract by stride
    viskores::filter::entity_extraction::Mask mask;
    viskores::Id stride = 9;
    mask.SetStride(stride);

    viskores::cont::DataSet output = mask.Execute(dataset);
    VISKORES_TEST_ASSERT(test_equal(output.GetNumberOfCells(), 7), "Wrong result for Mask");

    viskores::cont::ArrayHandle<viskores::Float32> cellFieldArray;
    output.GetField("cellvar").GetData().AsArrayHandle(cellFieldArray);

    VISKORES_TEST_ASSERT(cellFieldArray.GetNumberOfValues() == 7 &&
                           cellFieldArray.ReadPortal().Get(2) == 18.f,
                         "Wrong mask data");
  }

  static void TestExplicit()
  {
    std::cout << "Testing mask cells explicit:" << std::endl;
    viskores::cont::DataSet dataset = MakeTestDataSet().Make3DExplicitDataSet5();

    // Setup and run filter to extract by stride
    viskores::filter::entity_extraction::Mask mask;
    viskores::Id stride = 2;
    mask.SetStride(stride);

    viskores::cont::DataSet output = mask.Execute(dataset);
    VISKORES_TEST_ASSERT(test_equal(output.GetNumberOfCells(), 2), "Wrong result for Mask");

    viskores::cont::ArrayHandle<viskores::Float32> cellFieldArray;
    output.GetField("cellvar").GetData().AsArrayHandle(cellFieldArray);

    VISKORES_TEST_ASSERT(cellFieldArray.GetNumberOfValues() == 2 &&
                           cellFieldArray.ReadPortal().Get(1) == 120.2f,
                         "Wrong mask data");
  }

  void operator()() const
  {
    TestingMask::TestUniform2D();
    TestingMask::TestUniform3D();
    TestingMask::TestExplicit();
  }
};
}

int UnitTestMaskFilter(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestingMask(), argc, argv);
}
