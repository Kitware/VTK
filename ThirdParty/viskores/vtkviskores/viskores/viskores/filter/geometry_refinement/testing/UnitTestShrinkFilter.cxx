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
#include <viskores/filter/geometry_refinement/Shrink.h>

#include <viskores/cont/DataSetBuilderExplicit.h>
#include <viskores/cont/DataSetBuilderUniform.h>
#include <viskores/cont/testing/MakeTestDataSet.h>
#include <viskores/cont/testing/Testing.h>

using viskores::cont::testing::MakeTestDataSet;

namespace
{

const std::array<viskores::FloatDefault, 7> expectedPointVar{
  { 10.1f, 20.1f, 30.2f, 30.2f, 20.1f, 40.2f, 50.3f }
};

const std::array<viskores::Id, 7> expectedConnectivityArray{ { 0, 1, 2, 3, 4, 5, 6 } };

const viskores::Vec3f expectedCoords[7]{
  { 0.333333f, 0.166666f, 0.0f }, { 0.833333f, 0.166666f, 0.0f }, { 0.833333f, 0.666666f, 0.0f },
  { 1.25f, 1.0f, 0.0f },          { 1.25f, 0.5f, 0.0f },          { 1.75f, 1.0f, 0.0f },
  { 1.75f, 1.5f, 0.0f }
};

const viskores::FloatDefault expectedPointValueCube1[8]{ 10.1f, 20.1f, 50.2f,  40.1f,
                                                         70.2f, 80.2f, 110.3f, 100.3f };
const viskores::Vec3f expectedCoordsCell1[8]{ { 0.4f, 0.4f, 0.4f }, { 0.6f, 0.4f, 0.4f },
                                              { 0.6f, 0.6f, 0.4f }, { 0.4f, 0.6f, 0.4f },
                                              { 0.4f, 0.4f, 0.6f }, { 0.6f, 0.4f, 0.6f },
                                              { 0.6f, 0.6f, 0.6f }, { 0.4f, 0.6f, 0.6f } };


void TestWithExplicitData()
{
  viskores::cont::DataSet dataSet = MakeTestDataSet().Make3DExplicitDataSet0();

  viskores::filter::geometry_refinement::Shrink shrink;
  shrink.SetFieldsToPass({ "pointvar", "cellvar" });

  VISKORES_TEST_ASSERT(test_equal(shrink.GetShrinkFactor(), 0.5f),
                       "Wrong shrink factor default value");

  // Test shrink factor clamping
  shrink.SetShrinkFactor(1.5f);
  VISKORES_TEST_ASSERT(test_equal(shrink.GetShrinkFactor(), 1.0f),
                       "Shrink factor not limited to 1");

  shrink.SetShrinkFactor(-0.5f);
  VISKORES_TEST_ASSERT(test_equal(shrink.GetShrinkFactor(), 0.0f),
                       "Shrink factor is not always positive");

  shrink.SetShrinkFactor(0.5f);

  viskores::cont::DataSet output = shrink.Execute(dataSet);
  VISKORES_TEST_ASSERT(test_equal(output.GetNumberOfCells(), dataSet.GetNumberOfCells()),
                       "Wrong number of cells for Shrink filter");
  VISKORES_TEST_ASSERT(test_equal(output.GetNumberOfPoints(), 7),
                       "Wrong number of points for Shrink");


  viskores::cont::ArrayHandle<viskores::Float32> outCellData =
    output.GetField("cellvar")
      .GetData()
      .AsArrayHandle<viskores::cont::ArrayHandle<viskores::Float32>>();

  VISKORES_TEST_ASSERT(test_equal(outCellData.ReadPortal().Get(0), 100.1f),
                       "Wrong cell field data");
  VISKORES_TEST_ASSERT(test_equal(outCellData.ReadPortal().Get(1), 100.2f),
                       "Wrong cell field data");

  viskores::cont::ArrayHandle<viskores::Float32> outPointData =
    output.GetField("pointvar")
      .GetData()
      .AsArrayHandle<viskores::cont::ArrayHandle<viskores::Float32>>();

  for (viskores::IdComponent i = 0; i < outPointData.GetNumberOfValues(); i++)
  {
    VISKORES_TEST_ASSERT(test_equal(outPointData.ReadPortal().Get(i), expectedPointVar[i]),
                         "Wrong point field data");
  }

  {
    const auto connectivityArray =
      output.GetCellSet().AsCellSet<viskores::cont::CellSetExplicit<>>().GetConnectivityArray(
        viskores::TopologyElementTagCell(), viskores::TopologyElementTagPoint());
    auto connectivityArrayPortal = connectivityArray.ReadPortal();

    for (viskores::IdComponent i = 0; i < connectivityArray.GetNumberOfValues(); i++)
    {
      VISKORES_TEST_ASSERT(test_equal(connectivityArrayPortal.Get(i), expectedConnectivityArray[i]),
                           "Wrong connectivity array value");
    }
  }

  auto newCoords = output.GetCoordinateSystem().GetDataAsMultiplexer();
  auto newCoordsP = newCoords.ReadPortal();

  for (viskores::IdComponent i = 0; i < newCoords.GetNumberOfValues(); i++)
  {
    VISKORES_TEST_ASSERT(test_equal(newCoordsP.Get(i)[0], expectedCoords[i][0]),
                         "Wrong point coordinates");
    VISKORES_TEST_ASSERT(test_equal(newCoordsP.Get(i)[1], expectedCoords[i][1]),
                         "Wrong point coordinates");
    VISKORES_TEST_ASSERT(test_equal(newCoordsP.Get(i)[2], expectedCoords[i][2]),
                         "Wrong point coordinates");
  }
}


void TestWithUniformData()
{
  viskores::cont::DataSet dataSet = MakeTestDataSet().Make3DUniformDataSet0();

  viskores::filter::geometry_refinement::Shrink shrink;
  shrink.SetFieldsToPass({ "pointvar", "cellvar" });

  shrink.SetShrinkFactor(0.2f);

  viskores::cont::DataSet output = shrink.Execute(dataSet);

  VISKORES_TEST_ASSERT(test_equal(output.GetNumberOfCells(), dataSet.GetNumberOfCells()),
                       "Number of cells changed after filtering");
  VISKORES_TEST_ASSERT(test_equal(output.GetNumberOfPoints(), 4 * 8), "Wrong number of points");

  viskores::cont::ArrayHandle<viskores::Float32> outCellData =
    output.GetField("cellvar")
      .GetData()
      .AsArrayHandle<viskores::cont::ArrayHandle<viskores::Float32>>();

  VISKORES_TEST_ASSERT(test_equal(outCellData.ReadPortal().Get(0), 100.1), "Wrong cell field data");
  VISKORES_TEST_ASSERT(test_equal(outCellData.ReadPortal().Get(1), 100.2), "Wrong cell field data");
  VISKORES_TEST_ASSERT(test_equal(outCellData.ReadPortal().Get(2), 100.3), "Wrong cell field data");
  VISKORES_TEST_ASSERT(test_equal(outCellData.ReadPortal().Get(3), 100.4), "Wrong cell field data");

  viskores::cont::ArrayHandle<viskores::Float32> outPointData =
    output.GetField("pointvar")
      .GetData()
      .AsArrayHandle<viskores::cont::ArrayHandle<viskores::Float32>>();

  for (viskores::IdComponent i = 0; i < 8; i++) // Test for the first cell only
  {
    VISKORES_TEST_ASSERT(test_equal(outPointData.ReadPortal().Get(i), expectedPointValueCube1[i]),
                         "Wrong cell field data");
  }

  auto newCoords = output.GetCoordinateSystem().GetDataAsMultiplexer();
  auto newCoordsP = newCoords.ReadPortal();

  for (viskores::IdComponent i = 0; i < 8; i++)
  {
    std::cout << newCoordsP.Get(i)[0] << " " << expectedCoordsCell1[i][0] << std::endl;
    std::cout << newCoordsP.Get(i)[1] << " " << expectedCoordsCell1[i][1] << std::endl;
    std::cout << newCoordsP.Get(i)[2] << " " << expectedCoordsCell1[i][2] << std::endl;

    VISKORES_TEST_ASSERT(test_equal(newCoordsP.Get(i)[0], expectedCoordsCell1[i][0]),
                         "Wrong point coordinates");
    VISKORES_TEST_ASSERT(test_equal(newCoordsP.Get(i)[1], expectedCoordsCell1[i][1]),
                         "Wrong point coordinates");
    VISKORES_TEST_ASSERT(test_equal(newCoordsP.Get(i)[2], expectedCoordsCell1[i][2]),
                         "Wrong point coordinates");
  }
}


void TestShrinkFilter()
{
  TestWithExplicitData();
  TestWithUniformData();
}

} // anonymous namespace

int UnitTestShrinkFilter(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestShrinkFilter, argc, argv);
}
