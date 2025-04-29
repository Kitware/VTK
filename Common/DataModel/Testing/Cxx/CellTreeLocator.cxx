// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include <iostream>

#include "vtkCellCenters.h"
#include "vtkCellTreeLocator.h"
#include "vtkCellTypeSource.h"
#include "vtkDataArray.h"
#include "vtkDebugLeaks.h"
#include "vtkGenericCell.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkSphereSource.h"
#include "vtkTransform.h"
#include "vtkTransformFilter.h"
#include "vtkUnstructuredGrid.h"

int TestWithCachedCellBoundsParameter(int cachedCellBounds)
{
  // kuhnan's sample code used to test
  // vtkCellLocator::IntersectWithLine(...9 params...)

  // sphere1: the outer sphere
  vtkNew<vtkSphereSource> sphere1;
  sphere1->SetThetaResolution(100);
  sphere1->SetPhiResolution(100);
  sphere1->SetRadius(1);
  sphere1->Update();

  // sphere2: the inner sphere
  vtkNew<vtkSphereSource> sphere2;
  sphere2->SetThetaResolution(100);
  sphere2->SetPhiResolution(100);
  sphere2->SetRadius(0.8);
  sphere2->Update();

  // the normals obtained from the outer sphere
  vtkDataArray* sphereNormals = sphere1->GetOutput()->GetPointData()->GetNormals();

  // the cell locator
  vtkNew<vtkCellTreeLocator> locator;
  locator->SetDataSet(sphere2->GetOutput());
  locator->SetCacheCellBounds(cachedCellBounds);
  locator->AutomaticOn();
  locator->BuildLocator();

  // init the counter and ray length
  int numIntersected = 0;
  double rayLen = 0.2000001; // = 1 - 0.8 + error tolerance
  int sub_id;
  vtkIdType cell_id;
  double param_t, intersect[3], paraCoord[3];
  double sourcePnt[3], destinPnt[3], normalVec[3];
  vtkNew<vtkGenericCell> cell;

  // this loop traverses each point on the outer sphere (sphere1)
  // and  looks for an intersection on the inner sphere (sphere2)
  for (int i = 0; i < sphere1->GetOutput()->GetNumberOfPoints(); i++)
  {
    sphere1->GetOutput()->GetPoint(i, sourcePnt);
    sphereNormals->GetTuple(i, normalVec);

    // cast a ray in the negative direction toward sphere1
    destinPnt[0] = sourcePnt[0] - rayLen * normalVec[0];
    destinPnt[1] = sourcePnt[1] - rayLen * normalVec[1];
    destinPnt[2] = sourcePnt[2] - rayLen * normalVec[2];

    if (locator->IntersectWithLine(
          sourcePnt, destinPnt, 0.0010, param_t, intersect, paraCoord, sub_id, cell_id, cell))
    {
      numIntersected++;
    }
  }

  if (numIntersected != 9802)
  {
    int numMissed = 9802 - numIntersected;
    vtkGenericWarningMacro("ERROR: " << numMissed << " ray-sphere intersections missed! "
                                     << "If on a non-WinTel32 platform, try rayLen = 0.200001"
                                     << " or 0.20001 for a new test.");
    return EXIT_FAILURE;
  }
  else
  {
    std::cout << "Passed: a total of 9802 ray-sphere intersections detected." << std::endl;
  }

  sphereNormals = nullptr;

  return EXIT_SUCCESS;
}

bool isPointInList(double const* const point, std::vector<std::array<double, 3>> const& list)
{
  for (auto const& pt : list)
  {
    bool match = true;
    for (int comp = 0; comp < 3; ++comp)
    {
      if (std::abs(point[comp] - pt[comp]) > 1e-14)
      {
        match = false;
        break;
      }
    }

    if (match)
    {
      return true;
    }
  }

  return false;
}

int Test2dFindMultipleCellsSingleTestCase(vtkSmartPointer<vtkUnstructuredGrid> grid,
  std::array<double, 3> coords, std::vector<std::array<double, 3>> const& expected_cell_centers,
  double const tolerance)
{
  vtkNew<vtkCellCenters> cellCentersFilter;
  cellCentersFilter->SetInputData(grid);
  cellCentersFilter->Update();
  auto* ccs = cellCentersFilter->GetOutput();

  vtkNew<vtkCellTreeLocator> locator;
  locator->SetDataSet(grid);
  locator->BuildLocator();

  {
    const vtkIdType cell_id = locator->FindCell(coords.data());
    double const* const cc_actual = ccs->GetPoint(cell_id);

    if (!isPointInList(cc_actual, expected_cell_centers))
    {
      vtkGenericWarningMacro("ERROR: did not expect to find cell with id "
        << cell_id << " and center (" << cc_actual[0] << ", " << cc_actual[1] << ", "
        << cc_actual[2] << ").");
      return EXIT_FAILURE;
    }
  }

  std::array<double, 6> bbox = { coords[0] - tolerance, coords[0] + tolerance,
    coords[1] - tolerance, coords[1] + tolerance, coords[2] - tolerance, coords[2] + tolerance };

  vtkNew<vtkIdList> cell_ids;
  locator->FindCellsWithinBounds(bbox.data(), cell_ids);

  if (static_cast<std::size_t>(cell_ids->GetNumberOfIds()) != expected_cell_centers.size())
  {
    vtkGenericWarningMacro("ERROR: expected to find "
      << expected_cell_centers.size() << " cells , but found " << cell_ids->GetNumberOfIds()
      << '.');
    return EXIT_FAILURE;
  }

  for (vtkIdType index = 0; index < cell_ids->GetNumberOfIds(); ++index)
  {
    auto const cell_id = cell_ids->GetId(index);
    double const* const cc_actual = ccs->GetPoint(cell_id);

    if (!isPointInList(cc_actual, expected_cell_centers))
    {
      vtkGenericWarningMacro("ERROR: did not expect to find cell #"
        << index << " with id " << cell_id << " and center (" << cc_actual[0] << ", "
        << cc_actual[1] << ", " << cc_actual[2] << ").");
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}

// Creates a 2d grid of quads in the x-y, x-z or y-z plane. no_extent_dim
// (0...2) selects the plane.
vtkSmartPointer<vtkUnstructuredGrid> create2dGrid(const int no_extent_dim)
{
  vtkNew<vtkCellTypeSource> source;
  source->SetCellType(VTK_QUAD);

  source->SetBlocksDimensions(10, 10, 1);

  source->SetOutputPrecision(vtkAlgorithm::DOUBLE_PRECISION);
  source->Update();

  vtkSmartPointer<vtkUnstructuredGrid> grid = source->GetOutput();

  if (no_extent_dim != 2)
  {
    vtkNew<vtkTransform> rotation;
    if (no_extent_dim == 0)
    {
      rotation->RotateY(-90);
    }
    else if (no_extent_dim == 1)
    {
      rotation->RotateX(90);
    }

    vtkNew<vtkTransformFilter> transformFilter;
    transformFilter->SetInputData(grid);
    transformFilter->SetTransform(rotation);
    transformFilter->Update();
    grid = transformFilter->GetUnstructuredGridOutput();
  }

  return grid;
}

// Converts the passed 2d array to 3d putting a zero at the index indicated by
// no_extent_dim
std::array<double, 3> to3d(std::array<double, 2> const& array_2d, const int no_extent_dim)
{
  auto const [x, y] = array_2d;
  if (no_extent_dim == 0)
  {
    return { 0, x, y };
  }
  if (no_extent_dim == 1)
  {
    return { x, 0, y };
  }

  return { x, y, 0 };
}

// Checks if vtkCellTreeLocator finds the right and right number of cells in a
// 2d grid in the x-y, x-z or y-z plane.
int Test2dFindMultipleCells()
{
  int retVal = EXIT_SUCCESS;

  double const tol = 0.01;
  for (int no_extent_dim = 0; no_extent_dim < 3; ++no_extent_dim)
  {
    auto const grid = create2dGrid(no_extent_dim);

    std::array<std::array<double, 3>, 3> const points_to_find = { to3d({ 0.5 + tol, 0.5 },
                                                                    no_extent_dim),
      to3d({ 0.5, 1 - tol }, no_extent_dim), to3d({ 1 + tol, 1 - tol }, no_extent_dim) };
    auto const cc00 = to3d({ 0.5, 0.5 }, no_extent_dim);
    auto const cc01 = to3d({ 1.5, 0.5 }, no_extent_dim);
    auto const cc10 = to3d({ 0.5, 1.5 }, no_extent_dim);
    auto const cc11 = to3d({ 1.5, 1.5 }, no_extent_dim);
    std::array<std::vector<std::array<double, 3>>, 3> const expected_cell_centers = {
      std::vector{ cc00 }, std::vector{ cc00, cc10 }, std::vector{ cc00, cc01, cc10, cc11 }
    };

    for (std::size_t index = 0; index < points_to_find.size(); ++index)
    {
      retVal += Test2dFindMultipleCellsSingleTestCase(
        grid, points_to_find[index], expected_cell_centers[index], 2 * tol);
    }
  }

  return retVal;
}

int CellTreeLocator(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  int retVal = TestWithCachedCellBoundsParameter(0);
  retVal += TestWithCachedCellBoundsParameter(1);
  retVal += Test2dFindMultipleCells();

  return retVal;
}
