// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCellData.h"
#include "vtkDataArrayRange.h"
#include "vtkDataArraySelection.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkNetCDFUGRIDReader.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkTesting.h"
#include "vtkUnstructuredGrid.h"

#include <algorithm>
#include <cstring>

/**
 * Original data:
 *
 * Mesh2 = 0 ;
 * Mesh2_node_x = 0.0, 1.0, 1.0, 0.0,
 *                1.0, 2.0, 2.0 ;
 * Mesh2_node_y = 1.0, 1.0, 0.0, 0.0,
 *                1.0, 1.0, 0.0 ;
 * Mesh2_face_nodes = 1, 2, 3, 4,  //start_index == 1
 *                    5, 6, 7, _ ;
 * h = 0.0, 0.5, 0.2, _, 0.5, 0.0, 0.4, //_fillValue = -1.0
 *     0.2, 0.3, 0.3, _, 0.2, 0.2, 0.3 ;
 * area = 1.0, 0.5,
 *        0.5, 1.5 ;
 * nb_points (non temporal) = 4, 3 ;
 * time = 0, 31 ;
 */

const std::array<std::array<double, 3>, 7> expectedPoints{
  std::array<double, 3>{ 0.0, 1.0, 0.0 },
  std::array<double, 3>{ 1.0, 1.0, 0.0 },
  std::array<double, 3>{ 1.0, 0.0, 0.0 },
  std::array<double, 3>{ 0.0, 0.0, 0.0 },
  std::array<double, 3>{ 1.0, 1.0, 0.0 },
  std::array<double, 3>{ 2.0, 1.0, 0.0 },
  std::array<double, 3>{ 2.0, 0.0, 0.0 },
};

const std::array<vtkIdType, 4> expectedQuadIds{ 0, 1, 2, 3 };
const std::array<vtkIdType, 3> expectedTriangleIds{ 4, 5, 6 };

const std::array<double, 7> expectedPointData{ 0.2, 0.3, 0.3, vtkMath::Nan(), 0.2, 0.2, 0.3 };
const std::array<float, 2> expectedAreaCellData{ 0.5f, 1.5f };
const std::array<int, 2> expectedNbPointsCellData{ 4, 3 };

#define check(expr, message)                                                                       \
  if (!(expr))                                                                                     \
  {                                                                                                \
    vtkErrorWithObjectMacro(nullptr, << #expr << " | " << message);                                \
    return 1;                                                                                      \
  }                                                                                                \
  (void)0

int TestNetCDFUGRIDReader(int argc, char* argv[])
{
  vtkNew<vtkTesting> testHelper;
  testHelper->AddArguments(argc, argv);
  if (!testHelper->IsFlagSpecified("-D"))
  {
    std::cerr << "Error: -D /path/to/data was not specified.";
    return 1;
  }

  std::string root = testHelper->GetDataRoot();

  vtkNew<vtkNetCDFUGRIDReader> reader;
  reader->SetFileName((root + "/Data/NetCDF/ugrid.nc").c_str());
  reader->SetReplaceFillValueWithNan(true);
  reader->UpdateTimeStep(31.0); // use different time

  vtkUnstructuredGrid* ugrid = reader->GetOutput();

  // Check cells
  auto cells = ugrid->GetCells();
  check(cells->GetNumberOfCells() == 2, "Wrong number of cells");

  vtkIdType cellSize;
  const vtkIdType* cellIds;

  cells->GetCellAtId(0, cellSize, cellIds);
  check(cellSize == 4, "First cell must be a quad");
  check(
    std::equal(cellIds, cellIds + 4, expectedQuadIds.begin()), "Wrong point ids for first cell");

  cells->GetCellAtId(1, cellSize, cellIds);
  check(cellSize == 3, "Second cell must be a triangle");
  check(std::equal(cellIds, cellIds + 3, expectedTriangleIds.begin()),
    "Wrong point ids for second cell");

  // Check points
  auto points = ugrid->GetPoints();
  check(points->GetNumberOfPoints() == 7, "Wrong number of points");
  check(points->GetDataType() == VTK_DOUBLE, "Wrong data type for points");

  for (std::size_t i{}; i < expectedPoints.size(); ++i)
  {
    std::array<double, 3> point;
    points->GetPoint(i, point.data());

    check(point == expectedPoints[i], "Wrong point data at point #" << i);
  }

  // Check point data
  auto pointData = ugrid->GetPointData();
  check(pointData->GetNumberOfArrays() == 1, "Wrong number of array");
  check(pointData->HasArray("h"), "Wrong point data array name, must match variable name");
  auto h = pointData->GetArray("h");
  check(h->GetDataType() == VTK_DOUBLE, "Wrong point data array data type");
  check(h->GetNumberOfComponents() == 1, "Wrong point data array number of component");
  check(h->GetNumberOfTuples() == 7, "Wrong point data array number of tuples");
  auto hData = vtk::DataArrayValueRange(vtkDoubleArray::SafeDownCast(h));
  check(
    std::equal(hData.begin(), hData.begin() + 3, expectedPointData.begin()), "Wrong point data");
  check(std::isnan(hData[3]), "Fill value not correctly replaced by NaN");
  check(
    std::equal(hData.begin() + 4, hData.end(), expectedPointData.begin() + 4), "Wrong point data");

  // Check cell data
  auto cellData = ugrid->GetCellData();
  check(cellData->GetNumberOfArrays() == 2, "Wrong number of arrays");

  // Check first cell array
  check(cellData->HasArray("area"), "Wrong cell data array name, must match variable name");
  auto area = cellData->GetArray("area");
  check(area->GetDataType() == VTK_FLOAT, "Wrong cell data array data type");
  check(area->GetNumberOfComponents() == 1, "Wrong cell data array number of components");
  check(area->GetNumberOfTuples() == 2, "Wrong cell data array number of tuples");
  auto areaData = vtk::DataArrayValueRange(vtkFloatArray::SafeDownCast(area));
  check(
    std::equal(areaData.begin(), areaData.end(), expectedAreaCellData.begin()), "Wrong cell data");

  // Check second cell array
  check(cellData->HasArray("nb_points"), "Wrong cell data array name, must match variable name");
  auto nbPts = cellData->GetArray("nb_points");
  check(nbPts->GetDataType() == VTK_INT, "Wrong cell data array data type");
  check(nbPts->GetNumberOfComponents() == 1, "Wrong cell data array number of components");
  check(nbPts->GetNumberOfTuples() == 2, "Wrong cell data array number of tuples");
  auto nbPtsData = vtk::DataArrayValueRange(vtkIntArray::SafeDownCast(nbPts));
  check(std::equal(nbPtsData.begin(), nbPtsData.end(), expectedNbPointsCellData.begin()),
    "Wrong cell data");

  // check array selection
  check(reader->GetNumberOfCellArrays() == 2, "Wrong number of cell array");
  check(std::strcmp(reader->GetCellArrayName(0), "area") == 0, "Wrong cell array name");
  check(reader->GetCellArrayStatus("area") == 1, "Array must be enable by default");
  reader->SetCellArrayStatus("area", 0);
  reader->Update();
  check(!reader->GetOutput()->GetCellData()->HasArray("area"), "Array disable failed");
  check(reader->GetOutput()->GetPointData()->HasArray("h"), "Wrong array disabled");
  reader->SetCellArrayStatus("area", 1);

  check(reader->GetNumberOfPointArrays() == 1, "Wrong number of point array");
  check(std::strcmp(reader->GetPointArrayName(0), "h") == 0, "Wrong point array name");
  check(reader->GetPointArrayStatus("h") == 1, "Array must be enable by default");
  reader->SetPointArrayStatus("h", 0);
  reader->Update();
  check(!reader->GetOutput()->GetPointData()->HasArray("h"), "Array disable failed");
  check(reader->GetOutput()->GetCellData()->HasArray("area"), "Wrong array disabled");

  return 0;
}
