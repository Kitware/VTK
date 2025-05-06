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

#include <viskores/filter/entity_extraction/ThresholdPoints.h>

using viskores::cont::testing::MakeTestDataSet;

namespace
{

class TestingThresholdPoints
{
public:
  static void TestRegular2D()
  {
    std::cout << "Testing threshold points on 2D regular dataset" << std::endl;
    viskores::cont::DataSet dataset = MakeTestDataSet().Make2DUniformDataSet1();

    viskores::filter::entity_extraction::ThresholdPoints thresholdPoints;
    thresholdPoints.SetThresholdBetween(40.0f, 71.0f);
    thresholdPoints.SetActiveField("pointvar");
    thresholdPoints.SetFieldsToPass("pointvar");
    auto output = thresholdPoints.Execute(dataset);

    VISKORES_TEST_ASSERT(test_equal(output.GetNumberOfCells(), 11),
                         "Wrong result for ThresholdPoints");
    VISKORES_TEST_ASSERT(test_equal(output.GetField("pointvar").GetNumberOfValues(), 25),
                         "Wrong number of points for ThresholdPoints");

    viskores::cont::Field pointField = output.GetField("pointvar");
    viskores::cont::ArrayHandle<viskores::Float32> pointFieldArray;
    pointField.GetData().AsArrayHandle(pointFieldArray);
    VISKORES_TEST_ASSERT(pointFieldArray.ReadPortal().Get(12) == 50.0f, "Wrong point field data");
  }

  static void TestRegular3D()
  {
    std::cout << "Testing threshold points on 3D regular dataset" << std::endl;
    viskores::cont::DataSet dataset = MakeTestDataSet().Make3DUniformDataSet1();

    viskores::filter::entity_extraction::ThresholdPoints thresholdPoints;
    thresholdPoints.SetThresholdAbove(1.0f);
    thresholdPoints.SetCompactPoints(true);
    thresholdPoints.SetActiveField("pointvar");
    thresholdPoints.SetFieldsToPass("pointvar");
    auto output = thresholdPoints.Execute(dataset);

    VISKORES_TEST_ASSERT(test_equal(output.GetNumberOfCells(), 27),
                         "Wrong result for ThresholdPoints");
    VISKORES_TEST_ASSERT(test_equal(output.GetField("pointvar").GetNumberOfValues(), 27),
                         "Wrong number of points for ThresholdPoints");

    viskores::cont::Field pointField = output.GetField("pointvar");
    viskores::cont::ArrayHandle<viskores::Float32> pointFieldArray;
    pointField.GetData().AsArrayHandle(pointFieldArray);
    VISKORES_TEST_ASSERT(pointFieldArray.ReadPortal().Get(0) == 99.0f, "Wrong point field data");
  }

  static void TestExplicit3D()
  {
    std::cout << "Testing threshold points on 3D explicit dataset" << std::endl;
    viskores::cont::DataSet dataset = MakeTestDataSet().Make3DExplicitDataSet5();

    viskores::filter::entity_extraction::ThresholdPoints thresholdPoints;
    thresholdPoints.SetThresholdBelow(50.0);
    thresholdPoints.SetCompactPoints(true);
    thresholdPoints.SetActiveField("pointvar");
    thresholdPoints.SetFieldsToPass("pointvar");
    auto output = thresholdPoints.Execute(dataset);

    VISKORES_TEST_ASSERT(test_equal(output.GetNumberOfCells(), 6),
                         "Wrong result for ThresholdPoints");
    VISKORES_TEST_ASSERT(test_equal(output.GetField("pointvar").GetNumberOfValues(), 6),
                         "Wrong number of points for ThresholdPoints");

    viskores::cont::Field pointField = output.GetField("pointvar");
    viskores::cont::ArrayHandle<viskores::Float32> pointFieldArray;
    pointField.GetData().AsArrayHandle(pointFieldArray);
    VISKORES_TEST_ASSERT(pointFieldArray.ReadPortal().Get(4) == 10.f, "Wrong point field data");
  }

  static void TestExplicit3DZeroResults()
  {
    std::cout << "Testing threshold on 3D explicit dataset with empty results" << std::endl;
    viskores::cont::DataSet dataset = MakeTestDataSet().Make3DExplicitDataSet1();

    viskores::filter::entity_extraction::ThresholdPoints thresholdPoints;

    thresholdPoints.SetThresholdBetween(500.0, 600.0);
    thresholdPoints.SetActiveField("pointvar");
    thresholdPoints.SetFieldsToPass("pointvar");
    auto output = thresholdPoints.Execute(dataset);
    VISKORES_TEST_ASSERT(output.GetNumberOfFields() == 2,
                         "Wrong number of fields in the output dataset");
    VISKORES_TEST_ASSERT(test_equal(output.GetNumberOfCells(), 0),
                         "Wrong result for ThresholdPoints");
  }

  void operator()() const
  {
    TestingThresholdPoints::TestRegular2D();
    TestingThresholdPoints::TestRegular3D();
    TestingThresholdPoints::TestExplicit3D();
    TestingThresholdPoints::TestExplicit3DZeroResults();
  }
};
}

int UnitTestThresholdPointsFilter(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestingThresholdPoints(), argc, argv);
}
