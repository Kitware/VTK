// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// .NAME
// .SECTION Description
// this program tests vtkUniformGrid

#include "vtkCell.h"
#include "vtkDebugLeaks.h"
#include "vtkGenericCell.h"
#include "vtkMathUtilities.h"
#include "vtkNew.h"
#include "vtkUniformGrid.h"

#include <cstdlib>
#include <sstream>

int TestOUG_0d(ostream& strm)
{
  int i;
  vtkNew<vtkUniformGrid> ug0D;

  ug0D->SetDimensions(1, 1, 1);

  // Test GetCell
  vtkNew<vtkIdList> ids;
  int cellId;
  int ii;

  cellId = 0;
  vtkCell* cell0D = ug0D->GetCell(0);
  strm << "cell0D: " << *cell0D;
  ug0D->GetCellPoints(cellId, ids);
  strm << "Ids for cell " << cellId << " are ";
  for (ii = 0; ii < ids->GetNumberOfIds(); ii++)
  {
    strm << ids->GetId(ii) << " ";
  }
  strm << endl << endl;

  // Test GetCell(i,j,k)

  cellId = 0;
  cell0D = ug0D->GetCell(0, 0, 0);
  if (cell0D == nullptr)
  {
    std::cerr << "vtkUniformGrid::GetCell returned nullptr instead of a valid cell.\n";
    return EXIT_FAILURE;
  }
  if (cell0D->GetCellType() != VTK_VERTEX)
  {
    std::cerr << "vtkUniformGrid::GetCell returned the wrong cell type.\n"
              << "Expected: " << VTK_VERTEX << " Returned: " << cell0D->GetCellType() << '\n';
    return EXIT_FAILURE;
  }
  strm << "cell0D: " << *cell0D;
  ug0D->GetCellPoints(cellId, ids);
  strm << "Ids for cell " << cellId << " are ";
  for (ii = 0; ii < ids->GetNumberOfIds(); ii++)
  {
    strm << ids->GetId(ii) << " ";
  }
  strm << endl << endl;

  // Test Thread Safe GetCell
  vtkNew<vtkGenericCell> gcell0D;

  i = 10;
  ug0D->GetCell(0, gcell0D);
  strm << "gcell0D: " << *gcell0D;

  // Test GetCellBounds

  double bounds[6];
  ug0D->GetCellBounds(i, bounds);
  strm << "GetCellBounds(ug0D): " << bounds[0] << ", " << bounds[1] << ", " << bounds[2] << ", "
       << bounds[3] << ", " << bounds[4] << ", " << bounds[5] << endl;

  // Test GetPoint

  double point[6];

  ug0D->GetPoint(0, point);
  strm << "GetPoint(ug0D): " << point[0] << ", " << point[1] << ", " << point[2] << endl;

  // Test GetCellType

  strm << "GetCellType(ug0D): " << ug0D->GetCellType(0) << endl;

  // Test GetActualMemorySize

  strm << "GetActualMemorySize(ug0D): " << ug0D->GetActualMemorySize() << endl;

  return EXIT_SUCCESS;
}

int TestOUG_1dx(ostream& strm)
{
  int i;
  vtkNew<vtkUniformGrid> ug1Dx;

  ug1Dx->SetDimensions(20, 1, 1);

  // Test GetCell
  vtkNew<vtkIdList> ids;
  int cellId;
  int ii;

  i = 10;
  cellId = i;
  vtkCell* cell1D = ug1Dx->GetCell(i);
  strm << "cell1D: " << *cell1D;
  ug1Dx->GetCellPoints(cellId, ids);
  strm << "Ids for cell " << cellId << " are ";
  for (ii = 0; ii < ids->GetNumberOfIds(); ii++)
  {
    strm << ids->GetId(ii) << " ";
  }
  strm << endl << endl;

  i = 10;
  cellId = i;
  cell1D = ug1Dx->GetCell(i, 0, 0);
  if (cell1D == nullptr)
  {
    std::cerr << "vtkUniformGrid::GetCell returned nullptr instead of a valid cell.\n";
    return EXIT_FAILURE;
  }
  if (cell1D->GetCellType() != VTK_LINE)
  {
    std::cerr << "vtkUniformGrid::GetCell returned the wrong cell type.\n"
              << "Expected: " << VTK_LINE << " Returned: " << cell1D->GetCellType() << '\n';
    return EXIT_FAILURE;
  }
  double bounds[6];
  cell1D->GetBounds(bounds);
  if (!vtkMathUtilities::FuzzyCompare(bounds[2], bounds[3]))
  {
    std::cerr << "ug1Dx has finite width along y\n";
    return EXIT_FAILURE;
  }
  else if (!vtkMathUtilities::FuzzyCompare(bounds[4], bounds[5]))
  {
    std::cerr << "ug1Dx has finite width along z\n";
    return EXIT_FAILURE;
  }
  strm << "cell1D: " << *cell1D;
  ug1Dx->GetCellPoints(cellId, ids);
  strm << "Ids for cell " << cellId << " are ";
  for (ii = 0; ii < ids->GetNumberOfIds(); ii++)
  {
    strm << ids->GetId(ii) << " ";
  }
  strm << endl << endl;

  // Test Thread Safe GetCell
  vtkNew<vtkGenericCell> gcell1D;

  i = 10;
  ug1Dx->GetCell(i, gcell1D);
  strm << "gcell1D: " << *gcell1D;

  // Test GetCellBounds

  ug1Dx->GetCellBounds(i, bounds);
  strm << "GetCellBounds(rg1x): " << bounds[0] << ", " << bounds[1] << ", " << bounds[2] << ", "
       << bounds[3] << ", " << bounds[4] << ", " << bounds[5] << endl;

  // Test GetPoint

  double point[6];

  ug1Dx->GetPoint(i, point);
  strm << "GetPoint(rg1x): " << point[0] << ", " << point[1] << ", " << point[2] << endl;

  // Test FindPoint

  double point3D[3] = { 10, 12, 14 };

  point3D[0] = 10;
  point3D[1] = 0;
  point3D[2] = 0;
  ug1Dx->GetPoint(ug1Dx->FindPoint(point3D), point);
  strm << "FindPoint(" << point3D[0] << ", " << point3D[1] << ", " << point3D[2]
       << ") = " << point[0] << ", " << point[1] << ", " << point[2] << endl;

  // Test FindAndGetCell

  double pcoords[3], weights[8];
  int subId;
  vtkCell* dummyCell = nullptr;

  point3D[0] = 10.5;
  point3D[1] = 0.0;
  point3D[2] = 0.0;
  dummyCell = nullptr;
  vtkCell* found = ug1Dx->FindAndGetCell(point3D, dummyCell, 0, 0, subId, pcoords, weights);
  if (found == nullptr)
  {
    strm << "FindAndGetCell(ug1Dx) not found!" << endl;
    return EXIT_FAILURE;
  }

  strm << "FindAndGetCell(ug1Dx): " << *found;
  strm << "pcoords: " << pcoords[0] << endl;
  strm << "weights: " << weights[0] << ", " << weights[1] << endl;

  // Test GetCellType

  strm << "GetCellType(ug1Dx): " << ug1Dx->GetCellType(0) << endl;

  // Test GetActualMemorySize

  strm << "GetActualMemorySize(ug1Dx): " << ug1Dx->GetActualMemorySize() << endl;

  return EXIT_SUCCESS;
}

int TestOUG_1dy(ostream& strm)
{
  int i;
  vtkNew<vtkUniformGrid> ug1Dy;

  ug1Dy->SetDimensions(1, 20, 1);

  strm << *ug1Dy;

  // Test GetCell
  vtkNew<vtkIdList> ids;
  int cellId;
  int ii;

  i = 10;
  cellId = i;
  vtkCell* cell1D = ug1Dy->GetCell(i);
  strm << "cell1D: " << *cell1D;
  ug1Dy->GetCellPoints(cellId, ids);
  strm << "Ids for cell " << cellId << " are ";
  for (ii = 0; ii < ids->GetNumberOfIds(); ii++)
  {
    strm << ids->GetId(ii) << " ";
  }
  strm << endl << endl;

  i = 10;
  cellId = i;
  cell1D = ug1Dy->GetCell(0, i, 0);
  if (cell1D == nullptr)
  {
    std::cerr << "vtkUniformGrid::GetCell returned nullptr instead of a valid cell.\n";
    return EXIT_FAILURE;
  }
  if (cell1D->GetCellType() != VTK_LINE)
  {
    std::cerr << "vtkUniformGrid::GetCell returned the wrong cell type.\n"
              << "Expected: " << VTK_LINE << " Returned: " << cell1D->GetCellType() << '\n';
    return EXIT_FAILURE;
  }
  double bounds[6];
  cell1D->GetBounds(bounds);
  if (!vtkMathUtilities::FuzzyCompare(bounds[0], bounds[1]))
  {
    std::cerr << "ug1Dy has finite width along x\n";
    return EXIT_FAILURE;
  }
  else if (!vtkMathUtilities::FuzzyCompare(bounds[4], bounds[5]))
  {
    std::cerr << "ug1Dy has finite width along z\n";
    return EXIT_FAILURE;
  }
  strm << "cell1D: " << *cell1D;
  ug1Dy->GetCellPoints(cellId, ids);
  strm << "Ids for cell " << cellId << " are ";
  for (ii = 0; ii < ids->GetNumberOfIds(); ii++)
  {
    strm << ids->GetId(ii) << " ";
  }
  strm << endl << endl;

  // Test Thread Safe GetCell
  vtkNew<vtkGenericCell> gcell1D;

  i = 10;
  ug1Dy->GetCell(i, gcell1D);
  strm << "gcell1D: " << *gcell1D;

  // Test GetCellBounds

  ug1Dy->GetCellBounds(i, bounds);
  strm << "GetCellBounds(ug1Dy): " << bounds[0] << ", " << bounds[1] << ", " << bounds[2] << ", "
       << bounds[3] << ", " << bounds[4] << ", " << bounds[5] << endl;

  // Test GetPoint

  double point[6];

  ug1Dy->GetPoint(i, point);
  strm << "GetPoint(ug1Dy): " << point[0] << ", " << point[1] << ", " << point[2] << endl;

  // Test FindPoint

  double point3D[3] = { 10, 12, 14 };

  point3D[0] = 0;
  point3D[1] = 12;
  point3D[2] = 0;
  ug1Dy->GetPoint(ug1Dy->FindPoint(point3D), point);
  strm << "FindPoint(" << point3D[0] << ", " << point3D[1] << ", " << point3D[2]
       << ") = " << point[0] << ", " << point[1] << ", " << point[2] << endl;

  // Test FindAndGetCell

  double pcoords[3], weights[8];
  int subId;
  vtkCell* dummyCell = nullptr;

  point3D[0] = 0.0;
  point3D[1] = 12.1;
  point3D[2] = 0.0;
  dummyCell = nullptr;
  vtkCell* found = ug1Dy->FindAndGetCell(point3D, dummyCell, 0, 0, subId, pcoords, weights);
  if (found == nullptr)
  {
    strm << "FindAndGetCell(ug1Dy) not found!" << endl;
    return EXIT_FAILURE;
  }

  strm << "FindAndGetCell(ug1Dy): " << *found;
  strm << "pcoords: " << pcoords[0] << endl;
  strm << "weights: " << weights[0] << ", " << weights[1] << endl;

  // Test GetCellType

  strm << "GetCellType(ug1Dy): " << ug1Dy->GetCellType(0) << endl;

  // Test GetActualMemorySize

  strm << "GetActualMemorySize(ug1Dy): " << ug1Dy->GetActualMemorySize() << endl;

  return EXIT_SUCCESS;
}

int TestOUG_1dz(ostream& strm)
{
  int i;
  vtkNew<vtkUniformGrid> ug1Dz;

  ug1Dz->SetDimensions(1, 1, 20);

  // Test GetCell
  vtkNew<vtkIdList> ids;
  int cellId;
  int ii;

  i = 10;
  cellId = i;
  vtkCell* cell1D = ug1Dz->GetCell(i);
  strm << "cell1D: " << *cell1D;
  ug1Dz->GetCellPoints(cellId, ids);
  strm << "Ids for cell " << cellId << " are ";
  for (ii = 0; ii < ids->GetNumberOfIds(); ii++)
  {
    strm << ids->GetId(ii) << " ";
  }
  strm << endl << endl;

  i = 10;
  cellId = i;
  cell1D = ug1Dz->GetCell(0, 0, i);
  if (cell1D == nullptr)
  {
    std::cerr << "vtkUniformGrid::GetCell returned nullptr instead of a valid cell.\n";
    return EXIT_FAILURE;
  }
  if (cell1D->GetCellType() != VTK_LINE)
  {
    std::cerr << "vtkUniformGrid::GetCell returned the wrong cell type.\n"
              << "Expected: " << VTK_LINE << " Returned: " << cell1D->GetCellType() << '\n';
    return EXIT_FAILURE;
  }
  double bounds[6];
  cell1D->GetBounds(bounds);
  if (!vtkMathUtilities::FuzzyCompare(bounds[0], bounds[1]))
  {
    std::cerr << "ug1Dz has finite width along x\n";
    return EXIT_FAILURE;
  }
  else if (!vtkMathUtilities::FuzzyCompare(bounds[2], bounds[3]))
  {
    std::cerr << "ug1Dz has finite width along y\n";
    return EXIT_FAILURE;
  }
  strm << "cell1D: " << *cell1D;
  ug1Dz->GetCellPoints(cellId, ids);
  strm << "Ids for cell " << cellId << " are ";
  for (ii = 0; ii < ids->GetNumberOfIds(); ii++)
  {
    strm << ids->GetId(ii) << " ";
  }
  strm << endl << endl;

  // Test Thread Safe GetCell
  vtkNew<vtkGenericCell> gcell1D;

  i = 10;
  ug1Dz->GetCell(i, gcell1D);
  strm << "gcell1D: " << *gcell1D;

  // Test GetCellBounds

  ug1Dz->GetCellBounds(i, bounds);
  strm << "GetCellBounds(ug1Dz): " << bounds[0] << ", " << bounds[1] << ", " << bounds[2] << ", "
       << bounds[3] << ", " << bounds[4] << ", " << bounds[5] << endl;

  // Test GetPoint

  double point[6];

  ug1Dz->GetPoint(i, point);
  strm << "GetPoint(ug1Dz): " << point[0] << ", " << point[1] << ", " << point[2] << endl;

  // Test FindPoint

  double point3D[3] = { 10, 12, 14 };

  point3D[0] = 0;
  point3D[1] = 0;
  point3D[2] = 14;
  ug1Dz->GetPoint(ug1Dz->FindPoint(point3D), point);
  strm << "FindPoint(" << point3D[0] << ", " << point3D[1] << ", " << point3D[2]
       << ") = " << point[0] << ", " << point[1] << ", " << point[2] << endl;

  // Test FindAndGetCell

  double pcoords[3], weights[8];
  int subId;
  vtkCell* dummyCell = nullptr;

  point3D[0] = 0.0;
  point3D[1] = 0.0;
  point3D[2] = 14.7;
  dummyCell = nullptr;
  vtkCell* found = ug1Dz->FindAndGetCell(point3D, dummyCell, 0, 0, subId, pcoords, weights);
  if (found == nullptr)
  {
    strm << "FindAndGetCell(ug1Dz) not found!" << endl;
    return EXIT_FAILURE;
  }

  strm << "FindAndGetCell(ug1Dz): " << *found;
  strm << "pcoords: " << pcoords[0] << endl;
  strm << "weights: " << weights[0] << ", " << weights[1] << endl;

  // Test GetCellType

  strm << "GetCellType(ug1Dz): " << ug1Dz->GetCellType(0) << endl;

  // Test GetActualMemorySize

  strm << "GetActualMemorySize(ug1Dz): " << ug1Dz->GetActualMemorySize() << endl;

  return EXIT_SUCCESS;
}

int TestOUG_2dxy(ostream& strm)
{
  int i, j;
  vtkNew<vtkUniformGrid> ug2Dxy;

  ug2Dxy->SetDimensions(20, 20, 1);

  // Test GetCell
  vtkNew<vtkIdList> ids;
  int cellId;
  int ii;

  i = 10;
  j = 15;
  cellId = j * 19 + i;
  vtkCell* cell2D = ug2Dxy->GetCell(cellId);
  strm << "cell2D: " << *cell2D;
  ug2Dxy->GetCellPoints(cellId, ids);
  strm << "Ids for cell " << cellId << " are ";
  for (ii = 0; ii < ids->GetNumberOfIds(); ii++)
  {
    strm << ids->GetId(ii) << " ";
  }
  strm << endl << endl;

  // Test GetCell(i,j,k)

  i = 10;
  j = 15;
  cellId = j * 19 + i;
  cell2D = ug2Dxy->GetCell(i, j, 0);
  if (cell2D == nullptr)
  {
    std::cerr << "vtkUniformGrid::GetCell returned nullptr instead of a valid cell.\n";
    return EXIT_FAILURE;
  }
  if (cell2D->GetCellType() != VTK_PIXEL)
  {
    std::cerr << "vtkUniformGrid::GetCell returned the wrong cell type.\n"
              << "Expected: " << VTK_PIXEL << " Returned: " << cell2D->GetCellType() << '\n';
    return EXIT_FAILURE;
  }
  double bounds[6];
  cell2D->GetBounds(bounds);
  if (!vtkMathUtilities::FuzzyCompare(bounds[4], bounds[5]))
  {
    std::cerr << "ug2Dxy has finite width along z\n";
    return EXIT_FAILURE;
  }
  strm << "cell2D: " << *cell2D;
  ug2Dxy->GetCellPoints(cellId, ids);
  strm << "Ids for cell " << cellId << " are ";
  for (ii = 0; ii < ids->GetNumberOfIds(); ii++)
  {
    strm << ids->GetId(ii) << " ";
  }
  strm << endl << endl;

  // Test Thread Safe GetCell
  vtkNew<vtkGenericCell> gcell2D;

  i = 10;
  j = 15;
  ug2Dxy->GetCell(j * 19 + i, gcell2D);
  strm << "gcell2D: " << *gcell2D;

  // Test GetCellBounds

  ug2Dxy->GetCellBounds(j * 19 + i, bounds);
  strm << "GetCellBounds(ug2Dxy): " << bounds[0] << ", " << bounds[1] << ", " << bounds[2] << ", "
       << bounds[3] << ", " << bounds[4] << ", " << bounds[5] << endl;

  // Test GetPoint

  double point[6];

  ug2Dxy->GetPoint(j * 20 + i, point);
  strm << "GetPoint(ug2Dxy): " << point[0] << ", " << point[1] << ", " << point[2] << endl;

  // Test FindPoint

  double point3D[3] = { 10, 12, 14 };

  point3D[0] = 10;
  point3D[1] = 12;
  point3D[2] = 0;
  ug2Dxy->GetPoint(ug2Dxy->FindPoint(point3D), point);
  strm << "FindPoint(" << point3D[0] << ", " << point3D[1] << ", " << point3D[2]
       << ") = " << point[0] << ", " << point[1] << ", " << point[2] << endl;

  // Test FindAndGetCell

  double pcoords[3], weights[8];
  int subId;
  vtkCell* dummyCell = nullptr;

  point3D[0] = 10.5;
  point3D[1] = 12.1;
  point3D[2] = 0;
  pcoords[0] = pcoords[1] = pcoords[2] = 0.0;
  dummyCell = nullptr;
  vtkCell* found = ug2Dxy->FindAndGetCell(point3D, dummyCell, 0, 0, subId, pcoords, weights);
  if (found == nullptr)
  {
    strm << "FindAndGetCell(ug2Dxy) not found!" << endl;
    return EXIT_FAILURE;
  }

  strm << "FindAndGetCell(ug2Dxy): " << *found;
  strm << "pcoords: " << pcoords[0] << ", " << pcoords[1] << endl;
  strm << "weights: " << weights[0] << ", " << weights[1] << ", " << weights[2] << ", "
       << weights[3] << endl;

  // Test GetCellType

  strm << "GetCellType(ug2Dxy): " << ug2Dxy->GetCellType(0) << endl;

  // Test GetActualMemorySize

  strm << "GetActualMemorySize(ug2Dxy): " << ug2Dxy->GetActualMemorySize() << endl;

  return EXIT_SUCCESS;
}

int TestOUG_2dxz(ostream& strm)
{
  int i, j;
  vtkNew<vtkUniformGrid> ug2Dxz;

  ug2Dxz->SetDimensions(20, 1, 20);

  // Test GetCell
  vtkNew<vtkIdList> ids;
  int cellId;
  int ii;

  i = 10;
  j = 15;
  cellId = j * 19 + i;
  vtkCell* cell2D = ug2Dxz->GetCell(j * 19 + i);
  strm << "cell2D: " << *cell2D;
  ug2Dxz->GetCellPoints(cellId, ids);
  strm << "Ids for cell " << cellId << " are ";
  for (ii = 0; ii < ids->GetNumberOfIds(); ii++)
  {
    strm << ids->GetId(ii) << " ";
  }
  strm << endl << endl;

  // Test GetCell(i,j,k)

  i = 10;
  j = 15;
  cellId = j * 19 + i;
  cell2D = ug2Dxz->GetCell(i, 0, j);
  if (cell2D == nullptr)
  {
    std::cerr << "vtkUniformGrid::GetCell returned nullptr instead of a valid cell.\n";
    return EXIT_FAILURE;
  }
  if (cell2D->GetCellType() != VTK_PIXEL)
  {
    std::cerr << "vtkUniformGrid::GetCell returned the wrong cell type.\n"
              << "Expected: " << VTK_PIXEL << " Returned: " << cell2D->GetCellType() << '\n';
    return EXIT_FAILURE;
  }
  double bounds[6];
  cell2D->GetBounds(bounds);
  if (!vtkMathUtilities::FuzzyCompare(bounds[2], bounds[3]))
  {
    std::cerr << "ug2Dxz has finite width along y\n";
    return EXIT_FAILURE;
  }
  strm << "cell2D: " << *cell2D;
  ug2Dxz->GetCellPoints(cellId, ids);
  strm << "Ids for cell " << cellId << " are ";
  for (ii = 0; ii < ids->GetNumberOfIds(); ii++)
  {
    strm << ids->GetId(ii) << " ";
  }
  strm << endl << endl;

  // Test Thread Safe GetCell
  vtkNew<vtkGenericCell> gcell2D;

  i = 10;
  j = 15;
  ug2Dxz->GetCell(j * 19 + i, gcell2D);
  strm << "gcell2D: " << *gcell2D;

  // Test GetCellBounds

  ug2Dxz->GetCellBounds(j * 19 + i, bounds);
  strm << "GetCellBounds(ug2Dxz): " << bounds[0] << ", " << bounds[1] << ", " << bounds[2] << ", "
       << bounds[3] << ", " << bounds[4] << ", " << bounds[5] << endl;

  // Test GetPoint

  double point[6];

  ug2Dxz->GetPoint(j * 20 + i, point);
  strm << "GetPoint(ug2Dxz): " << point[0] << ", " << point[1] << ", " << point[2] << endl;

  // Test FindPoint

  double point3D[3] = { 10, 12, 14 };

  point3D[0] = 10;
  point3D[1] = 0;
  point3D[2] = 14;
  ug2Dxz->GetPoint(ug2Dxz->FindPoint(point3D), point);
  strm << "FindPoint(" << point3D[0] << ", " << point3D[1] << ", " << point3D[2]
       << ") = " << point[0] << ", " << point[1] << ", " << point[2] << endl;

  // Test FindAndGetCell

  double pcoords[3], weights[8];
  int subId;
  vtkCell* dummyCell = nullptr;

  point3D[0] = 10.5;
  point3D[1] = 0.0;
  point3D[2] = 14.7;
  pcoords[0] = pcoords[1] = pcoords[2] = 0.0;
  weights[0] = weights[1] = weights[2] = weights[3] = 0.0;
  dummyCell = nullptr;
  vtkCell* found = ug2Dxz->FindAndGetCell(point3D, dummyCell, 0, 0, subId, pcoords, weights);
  if (found == nullptr)
  {
    strm << "FindAndGetCell(ug2Dxz) not found!" << endl;
    return EXIT_FAILURE;
  }

  strm << "FindAndGetCell(ug2Dxz): " << *found;
  strm << "pcoords: " << pcoords[0] << ", " << pcoords[1] << endl;
  strm << "weights: " << weights[0] << ", " << weights[1] << ", " << weights[2] << ", "
       << weights[3] << endl;

  // Test GetCellType

  strm << "GetCellType(ug2Dxz): " << ug2Dxz->GetCellType(0) << endl;

  // Test GetActualMemorySize

  strm << "GetActualMemorySize(ug2Dxz): " << ug2Dxz->GetActualMemorySize() << endl;

  return EXIT_SUCCESS;
}

int TestOUG_2dyz(ostream& strm)
{
  int i, j;
  vtkNew<vtkUniformGrid> ug2Dyz;

  ug2Dyz->SetDimensions(1, 20, 20);

  // Test GetCell
  vtkNew<vtkIdList> ids;
  int cellId;
  int ii;

  i = 10;
  j = 15;
  cellId = j * 19 + i;
  vtkCell* cell2D = ug2Dyz->GetCell(j * 19 + i);
  strm << "cell2D: " << *cell2D;
  ug2Dyz->GetCellPoints(cellId, ids);
  strm << "Ids for cell " << cellId << " are ";
  for (ii = 0; ii < ids->GetNumberOfIds(); ii++)
  {
    strm << ids->GetId(ii) << " ";
  }
  strm << endl << endl;

  // Test GetCell(i,j,k)

  i = 10;
  j = 15;
  cellId = j * 19 + i;
  cell2D = ug2Dyz->GetCell(0, i, j);
  if (cell2D == nullptr)
  {
    std::cerr << "vtkUniformGrid::GetCell returned nullptr instead of a valid cell.\n";
    return EXIT_FAILURE;
  }
  if (cell2D->GetCellType() != VTK_PIXEL)
  {
    std::cerr << "vtkUniformGrid::GetCell returned the wrong cell type.\n"
              << "Expected: " << VTK_PIXEL << " Returned: " << cell2D->GetCellType() << '\n';
    return EXIT_FAILURE;
  }
  double bounds[6];
  cell2D->GetBounds(bounds);
  if (!vtkMathUtilities::FuzzyCompare(bounds[0], bounds[1]))
  {
    std::cerr << "ug2Dyz has finite width along x\n";
    return EXIT_FAILURE;
  }
  strm << "cell2D: " << *cell2D;
  ug2Dyz->GetCellPoints(cellId, ids);
  strm << "Ids for cell " << cellId << " are ";
  for (ii = 0; ii < ids->GetNumberOfIds(); ii++)
  {
    strm << ids->GetId(ii) << " ";
  }
  strm << endl << endl;

  // Test Thread Safe GetCell
  vtkNew<vtkGenericCell> gcell2D;

  i = 10;
  j = 15;
  ug2Dyz->GetCell(j * 19 + i, gcell2D);
  strm << "gcell2D: " << *gcell2D;

  // Test GetCellBounds

  ug2Dyz->GetCellBounds(j * 19 + i, bounds);
  strm << "GetCellBounds(ug2Dyz): " << bounds[0] << ", " << bounds[1] << ", " << bounds[2] << ", "
       << bounds[3] << ", " << bounds[4] << ", " << bounds[5] << endl;

  // Test GetPoint

  double point[6];

  ug2Dyz->GetPoint(j * 20 + i, point);
  strm << "GetPoint(ug2Dyz): " << point[0] << ", " << point[1] << ", " << point[2] << endl;

  // Test FindPoint

  double point3D[3] = { 10, 12, 14 };

  point3D[0] = 0;
  point3D[1] = 12;
  point3D[2] = 14;
  ug2Dyz->GetPoint(ug2Dyz->FindPoint(point3D), point);
  strm << "FindPoint(" << point3D[0] << ", " << point3D[1] << ", " << point3D[2]
       << ") = " << point[0] << ", " << point[1] << ", " << point[2] << endl;

  // Test FindAndGetCell

  double pcoords[3], weights[8];
  int subId;
  vtkCell* dummyCell = nullptr;

  point3D[0] = 0.0;
  point3D[1] = 12.1;
  point3D[2] = 14.7;
  pcoords[0] = pcoords[1] = pcoords[2] = 0.0;
  weights[0] = weights[1] = weights[2] = weights[3] = 0.0;
  dummyCell = nullptr;
  vtkCell* found = ug2Dyz->FindAndGetCell(point3D, dummyCell, 0, 0, subId, pcoords, weights);
  if (found == nullptr)
  {
    strm << "FindAndGetCell(ug2Dyz) not found!" << endl;
    return EXIT_FAILURE;
  }

  strm << "FindAndGetCell(ug2Dyz): " << *found;
  strm << "pcoords: " << pcoords[0] << ", " << pcoords[1] << endl;
  strm << "weights: " << weights[0] << ", " << weights[1] << ", " << weights[2] << ", "
       << weights[3] << endl;

  // Test GetCellType

  strm << "GetCellType(ug2Dyz): " << ug2Dyz->GetCellType(0) << endl;

  // Test GetActualMemorySize

  strm << "GetActualMemorySize(ug2Dyz): " << ug2Dyz->GetActualMemorySize() << endl;

  return EXIT_SUCCESS;
}

int TestOUG_3d(ostream& strm)
{
  int i, j, k;
  vtkNew<vtkUniformGrid> ug3D;

  ug3D->SetDimensions(20, 20, 20);

  strm << "ug3D:" << *ug3D;

  // Test shallow copy
  vtkNew<vtkUniformGrid> scug3D;
  scug3D->ShallowCopy(ug3D);
  strm << "ShallowCopy(ug3D):" << *scug3D;

  // Test deep copy
  vtkNew<vtkUniformGrid> dcug3D;
  dcug3D->DeepCopy(ug3D);
  strm << "DeepCopy(ug3D):" << *dcug3D;

  // Test GetCell
  vtkNew<vtkIdList> ids;
  int cellId;
  int ii;

  i = 10;
  j = 15;
  k = 7;
  cellId = k * (19 * 19) + j * 19 + i;
  vtkCell* cell3D = ug3D->GetCell(cellId);
  strm << "cell3D: " << *cell3D;
  ug3D->GetCellPoints(cellId, ids);
  strm << "Ids for cell " << cellId << " are ";
  for (ii = 0; ii < ids->GetNumberOfIds(); ii++)
  {
    strm << ids->GetId(ii) << " ";
  }
  strm << endl << endl;

  // Test GetCell(i,j,k)
  i = 10;
  j = 15;
  k = 7;
  cellId = k * (19 * 19) + j * 19 + i;
  cell3D = ug3D->GetCell(i, j, k);
  if (cell3D == nullptr)
  {
    std::cerr << "vtkUniformGrid::GetCell returned nullptr instead of a valid cell.\n";
    return EXIT_FAILURE;
  }
  if (cell3D->GetCellType() != VTK_VOXEL)
  {
    std::cerr << "vtkUniformGrid::GetCell returned the wrong cell type.\n"
              << "Expected: " << VTK_VOXEL << " Returned: " << cell3D->GetCellType() << '\n';
    return EXIT_FAILURE;
  }
  strm << "cell3D: " << *cell3D;
  ug3D->GetCellPoints(cellId, ids);
  strm << "Ids for cell " << cellId << " are ";
  for (ii = 0; ii < ids->GetNumberOfIds(); ii++)
  {
    strm << ids->GetId(ii) << " ";
  }
  strm << endl << endl;

  // Test Thread Safe GetCell
  vtkNew<vtkGenericCell> gcell3D;
  i = 10;
  j = 15;
  k = 7;
  ug3D->GetCell(k * (19 * 19) + j * 19 + i, gcell3D);
  strm << "gcell3D: " << *gcell3D;

  // Test GetCellBounds
  double bounds[6];
  ug3D->GetCellBounds(k * (19 * 19) + j * 19 + i, bounds);
  strm << "GetCellBounds(ug3D): " << bounds[0] << ", " << bounds[1] << ", " << bounds[2] << ", "
       << bounds[3] << ", " << bounds[4] << ", " << bounds[5] << endl;

  // Test GetPoint

  double point[6];
  ug3D->GetPoint(k * (20 * 20) + j * 20 + i, point);
  strm << "GetPoint(ug3D): " << point[0] << ", " << point[1] << ", " << point[2] << endl;

  // Test FindPoint

  double point3D[3] = { 10, 12, 14 };

  ug3D->GetPoint(ug3D->FindPoint(point3D), point);
  strm << "FindPoint(" << point3D[0] << ", " << point3D[1] << ", " << point3D[2]
       << ") = " << point[0] << ", " << point[1] << ", " << point[2] << endl;

  // Test FindAndGetCell

  double pcoords[3], weights[8];
  int subId;
  vtkCell* dummyCell = nullptr;

  point3D[0] = 10.5;
  point3D[1] = 12.1;
  point3D[2] = 14.7;

  strm << "FindAndGetCell(ug3D): "
       << *ug3D->FindAndGetCell(point3D, dummyCell, 0, 0, subId, pcoords, weights);
  strm << "pcoords: " << pcoords[0] << ", " << pcoords[1] << ", " << pcoords[2] << endl;
  strm << "weights: " << weights[0] << ", " << weights[1] << ", " << weights[2] << ", "
       << weights[3] << ", " << weights[4] << ", " << weights[5] << ", " << weights[6] << ", "
       << weights[7] << endl;

  // Test GetCellType

  strm << "GetCellType(ug3D): " << ug3D->GetCellType(0) << endl;

  // Test GetActualMemorySize

  strm << "GetActualMemorySize(ug3D): " << ug3D->GetActualMemorySize() << endl;

  return EXIT_SUCCESS;
}

int TestOUG(ostream& strm)
{
  int ret = EXIT_SUCCESS;
  strm << "Testing vtkUniformGrid" << endl;

  ret = TestOUG_0d(strm);
  if (ret != EXIT_SUCCESS)
  {
    return ret;
  }

  ret = TestOUG_1dx(strm);
  if (ret != EXIT_SUCCESS)
  {
    return ret;
  }

  ret = TestOUG_1dy(strm);
  if (ret != EXIT_SUCCESS)
  {
    return ret;
  }

  ret = TestOUG_1dz(strm);
  if (ret != EXIT_SUCCESS)
  {
    return ret;
  }

  ret = TestOUG_2dxy(strm);
  if (ret != EXIT_SUCCESS)
  {
    return ret;
  }

  ret = TestOUG_2dxz(strm);
  if (ret != EXIT_SUCCESS)
  {
    return ret;
  }

  ret = TestOUG_2dyz(strm);
  if (ret != EXIT_SUCCESS)
  {
    return ret;
  }

  ret = TestOUG_3d(strm);
  if (ret != EXIT_SUCCESS)
  {
    return ret;
  }

  strm << "Testing completed" << endl;
  return ret;
}

int otherUniformGrid(int, char*[])
{
  std::ostringstream vtkmsg_with_warning_C4701;
  return TestOUG(vtkmsg_with_warning_C4701);
}
