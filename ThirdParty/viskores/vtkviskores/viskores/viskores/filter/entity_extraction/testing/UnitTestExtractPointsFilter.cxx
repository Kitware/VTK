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

#include <viskores/filter/entity_extraction/ExtractPoints.h>

using viskores::cont::testing::MakeTestDataSet;

namespace
{

class TestingExtractPoints
{
public:
  static void TestUniformByBox0()
  {
    std::cout << "Testing extract points with implicit function (box):" << std::endl;
    viskores::cont::DataSet dataset = MakeTestDataSet().Make3DUniformDataSet1();

    // Implicit function
    viskores::Vec3f minPoint(1.f, 1.f, 1.f);
    viskores::Vec3f maxPoint(3.f, 3.f, 3.f);
    viskores::Box box(minPoint, maxPoint);

    // Setup and run filter to extract by volume of interest
    viskores::filter::entity_extraction::ExtractPoints extractPoints;
    extractPoints.SetImplicitFunction(box);
    extractPoints.SetExtractInside(true);
    extractPoints.SetCompactPoints(true);

    viskores::cont::DataSet output = extractPoints.Execute(dataset);
    VISKORES_TEST_ASSERT(test_equal(output.GetNumberOfCells(), 27),
                         "Wrong result for ExtractPoints");

    viskores::cont::ArrayHandle<viskores::Float32> outPointData;
    output.GetField("pointvar").GetData().AsArrayHandle(outPointData);

    VISKORES_TEST_ASSERT(
      test_equal(output.GetCellSet().GetNumberOfPoints(), outPointData.GetNumberOfValues()),
      "Data/Geometry mismatch for ExtractPoints filter");

    VISKORES_TEST_ASSERT(outPointData.ReadPortal().Get(0) == 99.0f, "Wrong point field data");
    VISKORES_TEST_ASSERT(outPointData.ReadPortal().Get(26) == 97.0f, "Wrong point field data");
  }

  static void TestUniformByBox1()
  {
    std::cout << "Testing extract points with implicit function (box):" << std::endl;
    viskores::cont::DataSet dataset = MakeTestDataSet().Make3DUniformDataSet1();

    // Implicit function
    viskores::Vec3f minPoint(1.f, 1.f, 1.f);
    viskores::Vec3f maxPoint(3.f, 3.f, 3.f);
    viskores::Box box(minPoint, maxPoint);

    // Setup and run filter to extract by volume of interest
    viskores::filter::entity_extraction::ExtractPoints extractPoints;
    extractPoints.SetImplicitFunction(box);
    extractPoints.SetExtractInside(false);
    extractPoints.SetCompactPoints(true);

    viskores::cont::DataSet output = extractPoints.Execute(dataset);
    VISKORES_TEST_ASSERT(test_equal(output.GetNumberOfCells(), 98),
                         "Wrong result for ExtractPoints");

    viskores::cont::ArrayHandle<viskores::Float32> outPointData;
    output.GetField("pointvar").GetData().AsArrayHandle(outPointData);

    VISKORES_TEST_ASSERT(
      test_equal(output.GetCellSet().GetNumberOfPoints(), outPointData.GetNumberOfValues()),
      "Data/Geometry mismatch for ExtractPoints filter");

    for (viskores::Id i = 0; i < output.GetCellSet().GetNumberOfPoints(); i++)
    {
      VISKORES_TEST_ASSERT(outPointData.ReadPortal().Get(i) == 0.0f, "Wrong point field data");
    }
  }

  static void TestUniformBySphere()
  {
    std::cout << "Testing extract points with implicit function (sphere):" << std::endl;
    viskores::cont::DataSet dataset = MakeTestDataSet().Make3DUniformDataSet1();

    // Implicit function
    viskores::Vec3f center(2.f, 2.f, 2.f);
    viskores::FloatDefault radius(1.8f);
    viskores::Sphere sphere(center, radius);

    // Setup and run filter to extract by volume of interest
    viskores::filter::entity_extraction::ExtractPoints extractPoints;
    extractPoints.SetImplicitFunction(sphere);
    extractPoints.SetExtractInside(true);

    viskores::cont::DataSet output = extractPoints.Execute(dataset);
    VISKORES_TEST_ASSERT(test_equal(output.GetNumberOfCells(), 27),
                         "Wrong result for ExtractPoints");
  }

  static void TestExplicitByBox0()
  {
    std::cout << "Testing extract points with implicit function (box):" << std::endl;
    viskores::cont::DataSet dataset = MakeTestDataSet().Make3DExplicitDataSet5();

    // Implicit function
    viskores::Vec3f minPoint(0.f, 0.f, 0.f);
    viskores::Vec3f maxPoint(1.f, 1.f, 1.f);
    viskores::Box box(minPoint, maxPoint);

    // Setup and run filter to extract by volume of interest
    viskores::filter::entity_extraction::ExtractPoints extractPoints;
    extractPoints.SetImplicitFunction(box);
    extractPoints.SetExtractInside(true);

    viskores::cont::DataSet output = extractPoints.Execute(dataset);
    VISKORES_TEST_ASSERT(test_equal(output.GetNumberOfCells(), 8),
                         "Wrong result for ExtractPoints");
  }

  static void TestExplicitByBox1()
  {
    std::cout << "Testing extract points with implicit function (box):" << std::endl;
    viskores::cont::DataSet dataset = MakeTestDataSet().Make3DExplicitDataSet5();

    // Implicit function
    viskores::Vec3f minPoint(0.f, 0.f, 0.f);
    viskores::Vec3f maxPoint(1.f, 1.f, 1.f);
    viskores::Box box(minPoint, maxPoint);

    // Setup and run filter to extract by volume of interest
    viskores::filter::entity_extraction::ExtractPoints extractPoints;
    extractPoints.SetImplicitFunction(box);
    extractPoints.SetExtractInside(false);

    viskores::cont::DataSet output = extractPoints.Execute(dataset);
    VISKORES_TEST_ASSERT(test_equal(output.GetNumberOfCells(), 3),
                         "Wrong result for ExtractPoints");
  }

  void operator()() const
  {
    TestingExtractPoints::TestUniformByBox0();
    TestingExtractPoints::TestUniformByBox1();
    TestingExtractPoints::TestUniformBySphere();
    TestingExtractPoints::TestExplicitByBox0();
    TestingExtractPoints::TestExplicitByBox1();
  }
};
}

int UnitTestExtractPointsFilter(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestingExtractPoints(), argc, argv);
}
