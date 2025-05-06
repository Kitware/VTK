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

#include <viskores/CellShape.h>

#include <viskores/cont/CellSetStructured.h>
#include <viskores/cont/DataSet.h>
#include <viskores/cont/serial/DeviceAdapterSerial.h>

#include <viskores/exec/ConnectivityStructured.h>

#include <viskores/cont/testing/MakeTestDataSet.h>
#include <viskores/cont/testing/Testing.h>

static void TwoDimUniformTest();
static void ThreeDimUniformTest();

void TestDataSet_Uniform()
{
  std::cout << std::endl;
  std::cout << "--TestDataSet_Uniform--" << std::endl << std::endl;

  TwoDimUniformTest();
  ThreeDimUniformTest();
}

static void TwoDimUniformTest()
{
  std::cout << "2D Uniform data set" << std::endl;
  viskores::cont::testing::MakeTestDataSet testDataSet;

  viskores::cont::DataSet dataSet = testDataSet.Make2DUniformDataSet0();

  dataSet.PrintSummary(std::cout);

  viskores::cont::CellSetStructured<2> cellSet;
  dataSet.GetCellSet().AsCellSet(cellSet);

  VISKORES_TEST_ASSERT(dataSet.GetNumberOfFields() == 3, "Incorrect number of fields");
  VISKORES_TEST_ASSERT(dataSet.GetNumberOfCoordinateSystems() == 1,
                       "Incorrect number of coordinate systems");
  VISKORES_TEST_ASSERT(cellSet.GetNumberOfPoints() == 6, "Incorrect number of points");
  VISKORES_TEST_ASSERT(cellSet.GetNumberOfCells() == 2, "Incorrect number of cells");
  VISKORES_TEST_ASSERT(cellSet.GetPointDimensions() == viskores::Id2(3, 2),
                       "Incorrect point dimensions");
  VISKORES_TEST_ASSERT(cellSet.GetCellDimensions() == viskores::Id2(2, 1),
                       "Incorrect cell dimensions");

  // test various field-getting methods and associations
  try
  {
    dataSet.GetCellField("cellvar");
  }
  catch (...)
  {
    VISKORES_TEST_FAIL("Failed to get field 'cellvar' with Association::Cells.");
  }

  try
  {
    dataSet.GetPointField("pointvar");
  }
  catch (...)
  {
    VISKORES_TEST_FAIL("Failed to get field 'pointvar' with ASSOC_POINT_SET.");
  }

  viskores::Id numCells = cellSet.GetNumberOfCells();
  for (viskores::Id cellIndex = 0; cellIndex < numCells; cellIndex++)
  {
    VISKORES_TEST_ASSERT(cellSet.GetNumberOfPointsInCell(cellIndex) == 4,
                         "Incorrect number of cell indices");
    viskores::IdComponent shape = cellSet.GetCellShape();
    VISKORES_TEST_ASSERT(shape == viskores::CELL_SHAPE_QUAD, "Incorrect element type.");
  }

  viskores::cont::Token token;
  viskores::exec::
    ConnectivityStructured<viskores::TopologyElementTagCell, viskores::TopologyElementTagPoint, 2>
      pointToCell = cellSet.PrepareForInput(viskores::cont::DeviceAdapterTagSerial(),
                                            viskores::TopologyElementTagCell(),
                                            viskores::TopologyElementTagPoint(),
                                            token);
  viskores::exec::
    ConnectivityStructured<viskores::TopologyElementTagPoint, viskores::TopologyElementTagCell, 2>
      cellToPoint = cellSet.PrepareForInput(viskores::cont::DeviceAdapterTagSerial(),
                                            viskores::TopologyElementTagPoint(),
                                            viskores::TopologyElementTagCell(),
                                            token);

  viskores::Id cells[2][4] = { { 0, 1, 4, 3 }, { 1, 2, 5, 4 } };
  for (viskores::Id cellIndex = 0; cellIndex < 2; cellIndex++)
  {
    viskores::Id4 pointIds = pointToCell.GetIndices(pointToCell.FlatToLogicalVisitIndex(cellIndex));
    for (viskores::IdComponent localPointIndex = 0; localPointIndex < 4; localPointIndex++)
    {
      VISKORES_TEST_ASSERT(pointIds[localPointIndex] == cells[cellIndex][localPointIndex],
                           "Incorrect point ID for cell");
    }
  }

  viskores::Id expectedCellIds[6][4] = { { 0, -1, -1, -1 }, { 0, 1, -1, -1 }, { 1, -1, -1, -1 },
                                         { 0, -1, -1, -1 }, { 0, 1, -1, -1 }, { 1, -1, -1, -1 } };

  for (viskores::Id pointIndex = 0; pointIndex < 6; pointIndex++)
  {
    viskores::VecVariable<viskores::Id, 4> retrievedCellIds =
      cellToPoint.GetIndices(cellToPoint.FlatToLogicalVisitIndex(pointIndex));
    VISKORES_TEST_ASSERT(retrievedCellIds.GetNumberOfComponents() <= 4,
                         "Got wrong number of cell ids.");
    for (viskores::IdComponent cellIndex = 0; cellIndex < retrievedCellIds.GetNumberOfComponents();
         cellIndex++)
    {
      VISKORES_TEST_ASSERT(retrievedCellIds[cellIndex] == expectedCellIds[pointIndex][cellIndex],
                           "Incorrect cell ID for point");
    }
  }
}

static void ThreeDimUniformTest()
{
  std::cout << "3D Uniform data set" << std::endl;
  viskores::cont::testing::MakeTestDataSet testDataSet;

  viskores::cont::DataSet dataSet = testDataSet.Make3DUniformDataSet0();

  dataSet.PrintSummary(std::cout);

  viskores::cont::CellSetStructured<3> cellSet;
  dataSet.GetCellSet().AsCellSet(cellSet);

  VISKORES_TEST_ASSERT(dataSet.GetNumberOfFields() == 3, "Incorrect number of fields");

  VISKORES_TEST_ASSERT(dataSet.GetNumberOfCoordinateSystems() == 1,
                       "Incorrect number of coordinate systems");

  VISKORES_TEST_ASSERT(cellSet.GetNumberOfPoints() == 18, "Incorrect number of points");

  VISKORES_TEST_ASSERT(cellSet.GetNumberOfCells() == 4, "Incorrect number of cells");

  VISKORES_TEST_ASSERT(cellSet.GetPointDimensions() == viskores::Id3(3, 2, 3),
                       "Incorrect point dimensions");

  VISKORES_TEST_ASSERT(cellSet.GetCellDimensions() == viskores::Id3(2, 1, 2),
                       "Incorrect cell dimensions");

  try
  {
    dataSet.GetCellField("cellvar");
  }
  catch (...)
  {
    VISKORES_TEST_FAIL("Failed to get field 'cellvar' with Association::Cells.");
  }

  try
  {
    dataSet.GetPointField("pointvar");
  }
  catch (...)
  {
    VISKORES_TEST_FAIL("Failed to get field 'pointvar' with ASSOC_POINT_SET.");
  }

  viskores::Id numCells = cellSet.GetNumberOfCells();
  for (viskores::Id cellIndex = 0; cellIndex < numCells; cellIndex++)
  {
    VISKORES_TEST_ASSERT(cellSet.GetNumberOfPointsInCell(cellIndex) == 8,
                         "Incorrect number of cell indices");
    viskores::IdComponent shape = cellSet.GetCellShape();
    VISKORES_TEST_ASSERT(shape == viskores::CELL_SHAPE_HEXAHEDRON, "Incorrect element type.");
  }

  viskores::cont::Token token;

  //Test uniform connectivity.
  viskores::exec::
    ConnectivityStructured<viskores::TopologyElementTagCell, viskores::TopologyElementTagPoint, 3>
      pointToCell = cellSet.PrepareForInput(viskores::cont::DeviceAdapterTagSerial(),
                                            viskores::TopologyElementTagCell(),
                                            viskores::TopologyElementTagPoint(),
                                            token);
  viskores::Id expectedPointIds[8] = { 0, 1, 4, 3, 6, 7, 10, 9 };
  viskores::Vec<viskores::Id, 8> retrievedPointIds = pointToCell.GetIndices(viskores::Id3(0));
  for (viskores::IdComponent localPointIndex = 0; localPointIndex < 8; localPointIndex++)
  {
    VISKORES_TEST_ASSERT(retrievedPointIds[localPointIndex] == expectedPointIds[localPointIndex],
                         "Incorrect point ID for cell");
  }

  viskores::exec::
    ConnectivityStructured<viskores::TopologyElementTagPoint, viskores::TopologyElementTagCell, 3>
      cellToPoint = cellSet.PrepareForInput(viskores::cont::DeviceAdapterTagSerial(),
                                            viskores::TopologyElementTagPoint(),
                                            viskores::TopologyElementTagCell(),
                                            token);
  viskores::Id retrievedCellIds[6] = { 0, -1, -1, -1, -1, -1 };
  viskores::VecVariable<viskores::Id, 6> expectedCellIds = cellToPoint.GetIndices(viskores::Id3(0));
  VISKORES_TEST_ASSERT(expectedCellIds.GetNumberOfComponents() <= 6,
                       "Got unexpected number of cell ids");
  for (viskores::IdComponent localPointIndex = 0;
       localPointIndex < expectedCellIds.GetNumberOfComponents();
       localPointIndex++)
  {
    VISKORES_TEST_ASSERT(expectedCellIds[localPointIndex] == retrievedCellIds[localPointIndex],
                         "Incorrect cell ID for point");
  }
}

int UnitTestDataSetUniform(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestDataSet_Uniform, argc, argv);
}
