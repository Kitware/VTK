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
#include <viskores/filter/field_transform/PointElevation.h>

#include <vector>

namespace
{

viskores::cont::DataSet MakePointElevationTestDataSet()
{
  viskores::cont::DataSet dataSet;

  std::vector<viskores::Vec3f_32> coordinates;
  const viskores::Id dim = 5;
  for (viskores::Id j = 0; j < dim; ++j)
  {
    viskores::Float32 z =
      static_cast<viskores::Float32>(j) / static_cast<viskores::Float32>(dim - 1);
    for (viskores::Id i = 0; i < dim; ++i)
    {
      viskores::Float32 x =
        static_cast<viskores::Float32>(i) / static_cast<viskores::Float32>(dim - 1);
      viskores::Float32 y = (x * x + z * z) / 2.0f;
      coordinates.push_back(viskores::make_Vec(x, y, z));
    }
  }

  viskores::Id numCells = (dim - 1) * (dim - 1);
  dataSet.AddCoordinateSystem(
    viskores::cont::make_CoordinateSystem("coordinates", coordinates, viskores::CopyFlag::On));

  viskores::cont::CellSetExplicit<> cellSet;
  cellSet.PrepareToAddCells(numCells, numCells * 4);
  for (viskores::Id j = 0; j < dim - 1; ++j)
  {
    for (viskores::Id i = 0; i < dim - 1; ++i)
    {
      cellSet.AddCell(viskores::CELL_SHAPE_QUAD,
                      4,
                      viskores::make_Vec<viskores::Id>(
                        j * dim + i, j * dim + i + 1, (j + 1) * dim + i + 1, (j + 1) * dim + i));
    }
  }
  cellSet.CompleteAddingCells(viskores::Id(coordinates.size()));

  dataSet.SetCellSet(cellSet);
  return dataSet;
}
}

void TestPointElevationNoPolicy()
{
  std::cout << "Testing PointElevation Filter With No Policy" << std::endl;

  viskores::cont::DataSet inputData = MakePointElevationTestDataSet();

  viskores::filter::field_transform::PointElevation filter;
  filter.SetLowPoint({ 0.0, 0.0, 0.0 });
  filter.SetHighPoint({ 0.0, 1.0, 0.0 });
  filter.SetRange(0.0, 2.0);

  filter.SetOutputFieldName("height");
  filter.SetUseCoordinateSystemAsField(true);
  auto result = filter.Execute(inputData);

  //verify the result
  VISKORES_TEST_ASSERT(result.HasPointField("height"), "Output field missing.");

  viskores::cont::ArrayHandle<viskores::Float64> resultArrayHandle;
  result.GetPointField("height").GetData().AsArrayHandle(resultArrayHandle);
  auto coordinates = inputData.GetCoordinateSystem().GetDataAsMultiplexer();
  auto coordsPortal = coordinates.ReadPortal();
  auto resultPortal = resultArrayHandle.ReadPortal();
  for (viskores::Id i = 0; i < resultArrayHandle.GetNumberOfValues(); ++i)
  {
    VISKORES_TEST_ASSERT(test_equal(coordsPortal.Get(i)[1] * 2.0, resultPortal.Get(i)),
                         "Wrong result for PointElevation worklet");
  }
}

void TestPointElevationWithPolicy()
{

  //simple test
  std::cout << "Testing PointElevation Filter With Explicit Policy" << std::endl;

  viskores::cont::DataSet inputData = MakePointElevationTestDataSet();

  viskores::filter::field_transform::PointElevation filter;
  filter.SetLowPoint({ 0.0, 0.0, 0.0 });
  filter.SetHighPoint({ 0.0, 1.0, 0.0 });
  filter.SetRange(0.0, 2.0);
  filter.SetUseCoordinateSystemAsField(true);

  auto result = filter.Execute(inputData);

  //verify the result
  VISKORES_TEST_ASSERT(result.HasPointField("elevation"), "Output field has wrong association");

  viskores::cont::ArrayHandle<viskores::Float64> resultArrayHandle;
  result.GetPointField("elevation").GetData().AsArrayHandle(resultArrayHandle);
  auto coordinates = inputData.GetCoordinateSystem().GetDataAsMultiplexer();
  auto coordsPortal = coordinates.ReadPortal();
  auto resultPortal = resultArrayHandle.ReadPortal();
  for (viskores::Id i = 0; i < resultArrayHandle.GetNumberOfValues(); ++i)
  {
    VISKORES_TEST_ASSERT(test_equal(coordsPortal.Get(i)[1] * 2.0, resultPortal.Get(i)),
                         "Wrong result for PointElevation worklet");
  }
}

void TestPointElevation()
{
  TestPointElevationNoPolicy();
  TestPointElevationWithPolicy();
}

int UnitTestPointElevationFilter(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestPointElevation, argc, argv);
}
