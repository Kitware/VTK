// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// .NAME
// .SECTION Description
// this program tests vtkStructuredGrid

#include "vtkCell.h"
#include "vtkDebugLeaks.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkGenericCell.h"
#include "vtkIdList.h"
#include "vtkLongArray.h"
#include "vtkMathUtilities.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkShortArray.h"
#include "vtkStructuredGrid.h"

#include <cstdlib>
#include <sstream>

int TestOSG_0d(ostream& strm)
{
  int i, k;
  vtkNew<vtkStructuredGrid> sg0D;

  vtkNew<vtkPoints> onepoints;
  for (k = 0; k < 1; k++)
  {
    onepoints->InsertNextPoint(0.0, 0.0, 0.0);
  }
  sg0D->SetDimensions(1, 1, 1);
  sg0D->SetPoints(onepoints);

  if (sg0D->GetCellSize(0) != 1)
  {
    std::cerr << "vtkStructuredGrid::GetCellSize(cellId) wrong for 0D structured grid.\n";
    return 1;
  }

  vtkNew<vtkShortArray> shortScalars3D;

  vtkNew<vtkShortArray> shortScalars0D;
  shortScalars0D->SetNumberOfComponents(1);
  shortScalars0D->SetNumberOfTuples(1);

  int l = 0;
  shortScalars0D->InsertComponent(l, 0, 0);

  sg0D->GetPointData()->SetScalars(shortScalars0D);

  // Test GetCell
  vtkNew<vtkIdList> ids;
  int cellId;
  int ii;

  cellId = 0;
  vtkCell* cell0D = sg0D->GetCell(0);
  strm << "cell0D: " << *cell0D;
  sg0D->GetCellPoints(cellId, ids);
  strm << "Ids for cell " << cellId << " are ";
  for (ii = 0; ii < ids->GetNumberOfIds(); ii++)
  {
    strm << ids->GetId(ii) << " ";
  }
  strm << endl << endl;

  // Test GetCell(i,j,k)

  cellId = 0;
  cell0D = sg0D->GetCell(0, 0, 0);
  if (cell0D == nullptr)
  {
    std::cerr << "vtkStructuredGrid::GetCell returned nullptr instead of a valid cell.\n";
    return EXIT_FAILURE;
  }
  if (cell0D->GetCellType() != VTK_VERTEX)
  {
    std::cerr << "vtkStructuredGrid::GetCell returned the wrong cell type.\n"
              << "Expected: " << VTK_VERTEX << " Returned: " << cell0D->GetCellType() << '\n';
    return EXIT_FAILURE;
  }
  strm << "cell0D: " << *cell0D;
  sg0D->GetCellPoints(cellId, ids);
  strm << "Ids for cell " << cellId << " are ";
  for (ii = 0; ii < ids->GetNumberOfIds(); ii++)
  {
    strm << ids->GetId(ii) << " ";
  }
  strm << endl << endl;

  // Test Thread Safe GetCell
  vtkNew<vtkGenericCell> gcell0D;

  i = 10;
  sg0D->GetCell(0, gcell0D);
  strm << "gcell0D: " << *gcell0D;

  // Test GetCellBounds

  double bounds[6];
  sg0D->GetCellBounds(i, bounds);
  strm << "GetCellBounds(sg0D): " << bounds[0] << ", " << bounds[1] << ", " << bounds[2] << ", "
       << bounds[3] << ", " << bounds[4] << ", " << bounds[5] << endl;

  // Test GetPoint

  double point[6];

  sg0D->GetPoint(0, point);
  strm << "GetPoint(sg0D): " << point[0] << ", " << point[1] << ", " << point[2] << endl;

  // Test GetCellType

  strm << "GetCellType(sg0D): " << sg0D->GetCellType(0) << endl;

  // Test GetActualMemorySize

  strm << "GetActualMemorySize(sg0D): " << sg0D->GetActualMemorySize() << endl;

  return EXIT_SUCCESS;
}

int TestOSG_1dx(ostream& strm)
{
  int i;
  vtkNew<vtkStructuredGrid> sg1Dx;

  vtkNew<vtkPoints> xpoints;
  for (i = 0; i < 20; i++)
  {
    xpoints->InsertNextPoint((double)i, 0.0, 0.0);
  }
  sg1Dx->SetDimensions(20, 1, 1);
  sg1Dx->SetPoints(xpoints);

  if (sg1Dx->GetCellSize(0) != 2)
  {
    std::cerr << "vtkStructuredGrid::GetCellSize(cellId) wrong for 1D structured grid.\n";
    return 1;
  }

  vtkNew<vtkShortArray> shortScalars1D;
  shortScalars1D->SetNumberOfComponents(1);
  shortScalars1D->SetNumberOfTuples(20);

  int l = 0;
  for (i = 0; i < 20; i++)
  {
    shortScalars1D->InsertComponent(l, 0, i);
    l++;
  }

  sg1Dx->GetPointData()->SetScalars(shortScalars1D);

  // Test GetCell
  vtkNew<vtkIdList> ids;
  int cellId;
  int ii;

  i = 10;
  cellId = i;
  vtkCell* cell1D = sg1Dx->GetCell(i);
  strm << "cell1D: " << *cell1D;
  sg1Dx->GetCellPoints(cellId, ids);
  strm << "Ids for cell " << cellId << " are ";
  for (ii = 0; ii < ids->GetNumberOfIds(); ii++)
  {
    strm << ids->GetId(ii) << " ";
  }
  strm << endl << endl;

  // Test GetCell(i,j,k)

  i = 10;
  cellId = i;
  cell1D = sg1Dx->GetCell(i, 0, 0);
  if (cell1D == nullptr)
  {
    std::cerr << "vtkStructuredGrid::GetCell returned nullptr instead of a valid cell.\n";
    return EXIT_FAILURE;
  }
  if (cell1D->GetCellType() != VTK_LINE)
  {
    std::cerr << "vtkStructuredGrid::GetCell returned the wrong cell type.\n"
              << "Expected: " << VTK_LINE << " Returned: " << cell1D->GetCellType() << '\n';
    return EXIT_FAILURE;
  }
  double bounds[6];
  cell1D->GetBounds(bounds);
  if (!vtkMathUtilities::FuzzyCompare(bounds[2], bounds[3]))
  {
    std::cerr << "sg1Dx has finite width along y\n";
    return EXIT_FAILURE;
  }
  else if (!vtkMathUtilities::FuzzyCompare(bounds[4], bounds[5]))
  {
    std::cerr << "sg1Dx has finite width along z\n";
    return EXIT_FAILURE;
  }
  strm << "cell1D: " << *cell1D;
  sg1Dx->GetCellPoints(cellId, ids);
  strm << "Ids for cell " << cellId << " are ";
  for (ii = 0; ii < ids->GetNumberOfIds(); ii++)
  {
    strm << ids->GetId(ii) << " ";
  }
  strm << endl << endl;

  // Test Thread Safe GetCell
  vtkNew<vtkGenericCell> gcell1D;

  i = 10;
  sg1Dx->GetCell(i, gcell1D);
  strm << "gcell1D: " << *gcell1D;

  // Test GetCellBounds

  sg1Dx->GetCellBounds(i, bounds);
  strm << "GetCellBounds(sg1x): " << bounds[0] << ", " << bounds[1] << ", " << bounds[2] << ", "
       << bounds[3] << ", " << bounds[4] << ", " << bounds[5] << endl;

  // Test GetPoint

  double point[6];

  sg1Dx->GetPoint(i, point);
  strm << "GetPoint(sg1x): " << point[0] << ", " << point[1] << ", " << point[2] << endl;

  // Test FindPoint

  double point3D[3] = { 10, 12, 14 };

  point3D[0] = 10;
  point3D[1] = 0;
  point3D[2] = 0;
  sg1Dx->GetPoint(sg1Dx->FindPoint(point3D), point);
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
  vtkCell* found = sg1Dx->FindAndGetCell(point3D, dummyCell, 0, 0, subId, pcoords, weights);
  if (found == nullptr)
  {
    strm << "FindAndGetCell(sg1Dx) not found!" << endl;
    return EXIT_FAILURE;
  }

  strm << "FindAndGetCell(sg1Dx): " << *found;
  strm << "pcoords: " << pcoords[0] << endl;
  strm << "weights: " << weights[0] << ", " << weights[1] << endl;

  // Test GetCellType

  strm << "GetCellType(sg1Dx): " << sg1Dx->GetCellType(0) << endl;

  // Test GetActualMemorySize

  strm << "GetActualMemorySize(sg1Dx): " << sg1Dx->GetActualMemorySize() << endl;

  return EXIT_SUCCESS;
}

int TestOSG_1dy(ostream& strm)
{
  int i, j;
  vtkNew<vtkStructuredGrid> sg1Dy;

  vtkNew<vtkPoints> ypoints;
  for (j = 0; j < 20; j++)
  {
    ypoints->InsertNextPoint(0.0, (double)j, 0.0);
  }
  sg1Dy->SetDimensions(1, 20, 1);
  sg1Dy->SetPoints(ypoints);
  strm << *sg1Dy;

  if (sg1Dy->GetCellSize(0) != 2)
  {
    std::cerr << "vtkStructuredGrid::GetCellSize(cellId) wrong for 1D structured grid.\n";
    return 1;
  }

  vtkNew<vtkShortArray> shortScalars1D;
  shortScalars1D->SetNumberOfComponents(1);
  shortScalars1D->SetNumberOfTuples(20);

  int l = 0;
  for (i = 0; i < 20; i++)
  {
    shortScalars1D->InsertComponent(l, 0, i);
    l++;
  }

  sg1Dy->GetPointData()->SetScalars(shortScalars1D);

  // Test GetCell
  vtkNew<vtkIdList> ids;
  int cellId;
  int ii;

  i = 10;
  cellId = i;
  vtkCell* cell1D = sg1Dy->GetCell(i);
  strm << "cell1D: " << *cell1D;
  sg1Dy->GetCellPoints(cellId, ids);
  strm << "Ids for cell " << cellId << " are ";
  for (ii = 0; ii < ids->GetNumberOfIds(); ii++)
  {
    strm << ids->GetId(ii) << " ";
  }
  strm << endl << endl;

  // Test GetCell(i,j,k)

  i = 10;
  cellId = i;
  cell1D = sg1Dy->GetCell(0, i, 0);
  if (cell1D == nullptr)
  {
    std::cerr << "vtkStructuredGrid::GetCell returned nullptr instead of a valid cell.\n";
    return EXIT_FAILURE;
  }
  if (cell1D->GetCellType() != VTK_LINE)
  {
    std::cerr << "vtkStructuredGrid::GetCell returned the wrong cell type.\n"
              << "Expected: " << VTK_LINE << " Returned: " << cell1D->GetCellType() << '\n';
    return EXIT_FAILURE;
  }
  double bounds[6];
  cell1D->GetBounds(bounds);
  if (!vtkMathUtilities::FuzzyCompare(bounds[0], bounds[1]))
  {
    std::cerr << "sg1Dy has finite width along x\n";
    return EXIT_FAILURE;
  }
  else if (!vtkMathUtilities::FuzzyCompare(bounds[4], bounds[5]))
  {
    std::cerr << "sg1Dy has finite width along z\n";
    return EXIT_FAILURE;
  }
  strm << "cell1D: " << *cell1D;
  sg1Dy->GetCellPoints(cellId, ids);
  strm << "Ids for cell " << cellId << " are ";
  for (ii = 0; ii < ids->GetNumberOfIds(); ii++)
  {
    strm << ids->GetId(ii) << " ";
  }
  strm << endl << endl;

  // Test Thread Safe GetCell
  vtkNew<vtkGenericCell> gcell1D;

  i = 10;
  sg1Dy->GetCell(i, gcell1D);
  strm << "gcell1D: " << *gcell1D;

  // Test GetCellBounds

  sg1Dy->GetCellBounds(i, bounds);
  strm << "GetCellBounds(sg1Dy): " << bounds[0] << ", " << bounds[1] << ", " << bounds[2] << ", "
       << bounds[3] << ", " << bounds[4] << ", " << bounds[5] << endl;

  // Test GetPoint

  double point[6];

  sg1Dy->GetPoint(i, point);
  strm << "GetPoint(sg1Dy): " << point[0] << ", " << point[1] << ", " << point[2] << endl;

  // Test FindPoint

  double point3D[3] = { 10, 12, 14 };

  point3D[0] = 0;
  point3D[1] = 12;
  point3D[2] = 0;
  sg1Dy->GetPoint(sg1Dy->FindPoint(point3D), point);
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
  vtkCell* found = sg1Dy->FindAndGetCell(point3D, dummyCell, 0, 0, subId, pcoords, weights);
  if (found == nullptr)
  {
    strm << "FindAndGetCell(sg1Dy) not found!" << endl;
    return EXIT_FAILURE;
  }

  strm << "FindAndGetCell(sg1Dy): " << *found;
  strm << "pcoords: " << pcoords[0] << endl;
  strm << "weights: " << weights[0] << ", " << weights[1] << endl;

  // Test GetCellType

  strm << "GetCellType(sg1Dy): " << sg1Dy->GetCellType(0) << endl;

  // Test GetActualMemorySize

  strm << "GetActualMemorySize(sg1Dy): " << sg1Dy->GetActualMemorySize() << endl;

  return EXIT_SUCCESS;
}

int TestOSG_1dz(ostream& strm)
{
  int i, k;
  vtkNew<vtkStructuredGrid> sg1Dz;

  vtkNew<vtkPoints> zpoints;
  for (k = 0; k < 20; k++)
  {
    zpoints->InsertNextPoint(0.0, 0.0, (double)k);
  }
  sg1Dz->SetDimensions(1, 1, 20);
  sg1Dz->SetPoints(zpoints);

  if (sg1Dz->GetCellSize(0) != 2)
  {
    std::cerr << "vtkStructuredGrid::GetCellSize(cellId) wrong for 1D structured grid.\n";
    return 1;
  }

  vtkNew<vtkShortArray> shortScalars1D;
  shortScalars1D->SetNumberOfComponents(1);
  shortScalars1D->SetNumberOfTuples(20);

  int l = 0;
  for (i = 0; i < 20; i++)
  {
    shortScalars1D->InsertComponent(l, 0, i);
    l++;
  }

  sg1Dz->GetPointData()->SetScalars(shortScalars1D);

  // Test GetCell
  vtkNew<vtkIdList> ids;
  int cellId;
  int ii;

  i = 10;
  cellId = i;
  vtkCell* cell1D = sg1Dz->GetCell(i);
  strm << "cell1D: " << *cell1D;
  sg1Dz->GetCellPoints(cellId, ids);
  strm << "Ids for cell " << cellId << " are ";
  for (ii = 0; ii < ids->GetNumberOfIds(); ii++)
  {
    strm << ids->GetId(ii) << " ";
  }
  strm << endl << endl;

  // Test GetCell(i,j,k)

  i = 10;
  cellId = i;
  cell1D = sg1Dz->GetCell(0, 0, i);
  if (cell1D == nullptr)
  {
    std::cerr << "vtkStructuredGrid::GetCell returned nullptr instead of a valid cell.\n";
    return EXIT_FAILURE;
  }
  if (cell1D->GetCellType() != VTK_LINE)
  {
    std::cerr << "vtkStructuredGrid::GetCell returned the wrong cell type.\n"
              << "Expected: " << VTK_LINE << " Returned: " << cell1D->GetCellType() << '\n';
    return EXIT_FAILURE;
  }
  double bounds[6];
  cell1D->GetBounds(bounds);
  if (!vtkMathUtilities::FuzzyCompare(bounds[0], bounds[1]))
  {
    std::cerr << "sg1Dz has finite width along x\n";
    return EXIT_FAILURE;
  }
  else if (!vtkMathUtilities::FuzzyCompare(bounds[2], bounds[3]))
  {
    std::cerr << "sg1Dz has finite width along y\n";
    return EXIT_FAILURE;
  }
  strm << "cell1D: " << *cell1D;
  sg1Dz->GetCellPoints(cellId, ids);
  strm << "Ids for cell " << cellId << " are ";
  for (ii = 0; ii < ids->GetNumberOfIds(); ii++)
  {
    strm << ids->GetId(ii) << " ";
  }
  strm << endl << endl;

  // Test Thread Safe GetCell
  vtkNew<vtkGenericCell> gcell1D;

  i = 10;
  sg1Dz->GetCell(i, gcell1D);
  strm << "gcell1D: " << *gcell1D;

  // Test GetCellBounds

  sg1Dz->GetCellBounds(i, bounds);
  strm << "GetCellBounds(sg1Dz): " << bounds[0] << ", " << bounds[1] << ", " << bounds[2] << ", "
       << bounds[3] << ", " << bounds[4] << ", " << bounds[5] << endl;

  // Test GetPoint

  double point[6];

  sg1Dz->GetPoint(i, point);
  strm << "GetPoint(sg1Dz): " << point[0] << ", " << point[1] << ", " << point[2] << endl;

  // Test FindPoint

  double point3D[3] = { 10, 12, 14 };

  point3D[0] = 0;
  point3D[1] = 0;
  point3D[2] = 14;
  sg1Dz->GetPoint(sg1Dz->FindPoint(point3D), point);
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
  vtkCell* found = sg1Dz->FindAndGetCell(point3D, dummyCell, 0, 0, subId, pcoords, weights);
  if (found == nullptr)
  {
    strm << "FindAndGetCell(sg1Dz) not found!" << endl;
    return EXIT_FAILURE;
  }

  strm << "FindAndGetCell(sg1Dz): " << *found;
  strm << "pcoords: " << pcoords[0] << endl;
  strm << "weights: " << weights[0] << ", " << weights[1] << endl;

  // Test GetCellType

  strm << "GetCellType(sg1Dz): " << sg1Dz->GetCellType(0) << endl;

  // Test GetActualMemorySize

  strm << "GetActualMemorySize(sg1Dz): " << sg1Dz->GetActualMemorySize() << endl;

  return EXIT_SUCCESS;
}

int TestOSG_2dxy(ostream& strm)
{
  int i, j;
  vtkNew<vtkStructuredGrid> sg2Dxy;

  vtkNew<vtkPoints> xypoints;
  for (j = 0; j < 20; j++)
  {
    for (i = 0; i < 20; i++)
    {
      xypoints->InsertNextPoint((double)i, (double)j, 0.0);
    }
  }
  sg2Dxy->SetDimensions(20, 20, 1);
  sg2Dxy->SetPoints(xypoints);

  if (sg2Dxy->GetCellSize(0) != 4)
  {
    std::cerr << "vtkStructuredGrid::GetCellSize(cellId) wrong for 2D structured grid.\n";
    return 1;
  }

  vtkNew<vtkShortArray> shortScalars2D;
  shortScalars2D->SetNumberOfComponents(2);
  shortScalars2D->SetNumberOfTuples(20 * 20);

  int l = 0;
  for (j = 0; j < 20; j++)
  {
    for (i = 0; i < 20; i++)
    {
      shortScalars2D->InsertComponent(l, 0, i);
      shortScalars2D->InsertComponent(l, 0, j);
      l++;
    }
  }

  sg2Dxy->GetPointData()->SetScalars(shortScalars2D);

  // Test GetCell
  vtkNew<vtkIdList> ids;
  int cellId;
  int ii;

  i = 10;
  j = 15;
  cellId = j * 19 + i;
  vtkCell* cell2D = sg2Dxy->GetCell(cellId);
  strm << "cell2D: " << *cell2D;
  sg2Dxy->GetCellPoints(cellId, ids);
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
  cell2D = sg2Dxy->GetCell(i, j, 0);
  if (cell2D == nullptr)
  {
    std::cerr << "vtkStructuredGrid::GetCell returned nullptr instead of a valid cell.\n";
    return EXIT_FAILURE;
  }
  if (cell2D->GetCellType() != VTK_QUAD)
  {
    std::cerr << "vtkStructuredGrid::GetCell returned the wrong cell type.\n"
              << "Expected: " << VTK_QUAD << " Returned: " << cell2D->GetCellType() << '\n';
    return EXIT_FAILURE;
  }
  double bounds[6];
  cell2D->GetBounds(bounds);
  if (!vtkMathUtilities::FuzzyCompare(bounds[4], bounds[5]))
  {
    std::cerr << "sg2Dxy has finite width along z\n";
    return EXIT_FAILURE;
  }
  strm << "cell2D: " << *cell2D;
  sg2Dxy->GetCellPoints(cellId, ids);
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
  sg2Dxy->GetCell(j * 19 + i, gcell2D);
  strm << "gcell2D: " << *gcell2D;

  // Test GetCellBounds

  sg2Dxy->GetCellBounds(j * 19 + i, bounds);
  strm << "GetCellBounds(sg2Dxy): " << bounds[0] << ", " << bounds[1] << ", " << bounds[2] << ", "
       << bounds[3] << ", " << bounds[4] << ", " << bounds[5] << endl;

  // Test GetPoint

  double point[6];

  sg2Dxy->GetPoint(j * 20 + i, point);
  strm << "GetPoint(sg2Dxy): " << point[0] << ", " << point[1] << ", " << point[2] << endl;

  // Test FindPoint

  double point3D[3] = { 10, 12, 14 };

  point3D[0] = 10;
  point3D[1] = 12;
  point3D[2] = 0;
  sg2Dxy->GetPoint(sg2Dxy->FindPoint(point3D), point);
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
  vtkCell* found = sg2Dxy->FindAndGetCell(point3D, dummyCell, 0, 0, subId, pcoords, weights);
  if (found == nullptr)
  {
    strm << "FindAndGetCell(sg2Dxy) not found!" << endl;
    return EXIT_FAILURE;
  }

  strm << "FindAndGetCell(sg2Dxy): " << *found;
  strm << "pcoords: " << pcoords[0] << ", " << pcoords[1] << endl;
  strm << "weights: " << weights[0] << ", " << weights[1] << ", " << weights[2] << ", "
       << weights[3] << endl;

  // Test GetCellType

  strm << "GetCellType(sg2Dxy): " << sg2Dxy->GetCellType(0) << endl;

  // Test GetActualMemorySize

  strm << "GetActualMemorySize(sg2Dxy): " << sg2Dxy->GetActualMemorySize() << endl;

  return EXIT_SUCCESS;
}

int TestOSG_2dxz(ostream& strm)
{
  int i, j, k;
  vtkNew<vtkStructuredGrid> sg2Dxz;

  vtkNew<vtkPoints> xzpoints;
  for (k = 0; k < 20; k++)
  {
    for (i = 0; i < 20; i++)
    {
      xzpoints->InsertNextPoint((double)i, 0.0, (double)k);
    }
  }
  sg2Dxz->SetDimensions(20, 1, 20);
  sg2Dxz->SetPoints(xzpoints);

  if (sg2Dxz->GetCellSize(0) != 4)
  {
    std::cerr << "vtkStructuredGrid::GetCellSize(cellId) wrong for 2D structured grid.\n";
    return 1;
  }

  vtkNew<vtkShortArray> shortScalars2D;
  shortScalars2D->SetNumberOfComponents(2);
  shortScalars2D->SetNumberOfTuples(20 * 20);

  int l = 0;
  for (j = 0; j < 20; j++)
  {
    for (i = 0; i < 20; i++)
    {
      shortScalars2D->InsertComponent(l, 0, i);
      shortScalars2D->InsertComponent(l, 0, j);
      l++;
    }
  }

  sg2Dxz->GetPointData()->SetScalars(shortScalars2D);

  // Test GetCell
  vtkNew<vtkIdList> ids;
  int cellId;
  int ii;

  i = 10;
  j = 15;
  cellId = j * 19 + i;
  vtkCell* cell2D = sg2Dxz->GetCell(j * 19 + i);
  strm << "cell2D: " << *cell2D;
  sg2Dxz->GetCellPoints(cellId, ids);
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
  cell2D = sg2Dxz->GetCell(i, 0, j);
  if (cell2D == nullptr)
  {
    std::cerr << "vtkStructuredGrid::GetCell returned nullptr instead of a valid cell.\n";
    return EXIT_FAILURE;
  }
  if (cell2D->GetCellType() != VTK_QUAD)
  {
    std::cerr << "vtkStructuredGrid::GetCell returned the wrong cell type.\n"
              << "Expected: " << VTK_QUAD << " Returned: " << cell2D->GetCellType() << '\n';
    return EXIT_FAILURE;
  }
  double bounds[6];
  cell2D->GetBounds(bounds);
  if (!vtkMathUtilities::FuzzyCompare(bounds[2], bounds[3]))
  {
    std::cerr << "sg2Dxz has finite width along y\n";
    return EXIT_FAILURE;
  }
  strm << "cell2D: " << *cell2D;
  sg2Dxz->GetCellPoints(cellId, ids);
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
  sg2Dxz->GetCell(j * 19 + i, gcell2D);
  strm << "gcell2D: " << *gcell2D;

  // Test GetCellBounds

  sg2Dxz->GetCellBounds(j * 19 + i, bounds);
  strm << "GetCellBounds(sg2Dxz): " << bounds[0] << ", " << bounds[1] << ", " << bounds[2] << ", "
       << bounds[3] << ", " << bounds[4] << ", " << bounds[5] << endl;

  // Test GetPoint

  double point[6];

  sg2Dxz->GetPoint(j * 20 + i, point);
  strm << "GetPoint(sg2Dxz): " << point[0] << ", " << point[1] << ", " << point[2] << endl;

  // Test FindPoint

  double point3D[3] = { 10, 12, 14 };

  point3D[0] = 10;
  point3D[1] = 0;
  point3D[2] = 14;
  sg2Dxz->GetPoint(sg2Dxz->FindPoint(point3D), point);
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
  vtkCell* found = sg2Dxz->FindAndGetCell(point3D, dummyCell, 0, 0, subId, pcoords, weights);
  if (found == nullptr)
  {
    strm << "FindAndGetCell(sg2Dxz) not found!" << endl;
    return EXIT_FAILURE;
  }

  strm << "FindAndGetCell(sg2Dxz): " << *found;
  strm << "pcoords: " << pcoords[0] << ", " << pcoords[1] << endl;
  strm << "weights: " << weights[0] << ", " << weights[1] << ", " << weights[2] << ", "
       << weights[3] << endl;

  // Test GetCellType

  strm << "GetCellType(sg2Dxz): " << sg2Dxz->GetCellType(0) << endl;

  // Test GetActualMemorySize

  strm << "GetActualMemorySize(sg2Dxz): " << sg2Dxz->GetActualMemorySize() << endl;

  return EXIT_SUCCESS;
}

int TestOSG_2dyz(ostream& strm)
{
  int i, j, k;
  vtkNew<vtkStructuredGrid> sg2Dyz;

  vtkNew<vtkPoints> yzpoints;
  for (k = 0; k < 20; k++)
  {
    for (j = 0; j < 20; j++)
    {
      yzpoints->InsertNextPoint(0.0, (double)j, (double)k);
    }
  }
  sg2Dyz->SetDimensions(1, 20, 20);
  sg2Dyz->SetPoints(yzpoints);

  if (sg2Dyz->GetCellSize(0) != 4)
  {
    std::cerr << "vtkStructuredGrid::GetCellSize(cellId) wrong for 2D structured grid.\n";
    return 1;
  }

  vtkNew<vtkShortArray> shortScalars2D;
  shortScalars2D->SetNumberOfComponents(2);
  shortScalars2D->SetNumberOfTuples(20 * 20);

  int l = 0;
  for (j = 0; j < 20; j++)
  {
    for (i = 0; i < 20; i++)
    {
      shortScalars2D->InsertComponent(l, 0, i);
      shortScalars2D->InsertComponent(l, 0, j);
      l++;
    }
  }

  sg2Dyz->GetPointData()->SetScalars(shortScalars2D);

  // Test GetCell
  vtkNew<vtkIdList> ids;
  int cellId;
  int ii;

  i = 10;
  j = 15;
  cellId = j * 19 + i;
  vtkCell* cell2D = sg2Dyz->GetCell(j * 19 + i);
  strm << "cell2D: " << *cell2D;
  sg2Dyz->GetCellPoints(cellId, ids);
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
  cell2D = sg2Dyz->GetCell(0, i, j);
  if (cell2D == nullptr)
  {
    std::cerr << "vtkStructuredGrid::GetCell returned nullptr instead of a valid cell.\n";
    return EXIT_FAILURE;
  }
  if (cell2D->GetCellType() != VTK_QUAD)
  {
    std::cerr << "vtkStructuredGrid::GetCell returned the wrong cell type.\n"
              << "Expected: " << VTK_QUAD << " Returned: " << cell2D->GetCellType() << '\n';
    return EXIT_FAILURE;
  }
  double bounds[6];
  cell2D->GetBounds(bounds);
  if (!vtkMathUtilities::FuzzyCompare(bounds[0], bounds[1]))
  {
    std::cerr << "sg2Dyz has finite width along x\n";
    return EXIT_FAILURE;
  }
  strm << "cell2D: " << *cell2D;
  sg2Dyz->GetCellPoints(cellId, ids);
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
  sg2Dyz->GetCell(j * 19 + i, gcell2D);
  strm << "gcell2D: " << *gcell2D;

  // Test GetCellBounds

  sg2Dyz->GetCellBounds(j * 19 + i, bounds);
  strm << "GetCellBounds(sg2Dyz): " << bounds[0] << ", " << bounds[1] << ", " << bounds[2] << ", "
       << bounds[3] << ", " << bounds[4] << ", " << bounds[5] << endl;

  // Test GetPoint

  double point[6];

  sg2Dyz->GetPoint(j * 20 + i, point);
  strm << "GetPoint(sg2Dyz): " << point[0] << ", " << point[1] << ", " << point[2] << endl;

  // Test FindPoint

  double point3D[3] = { 10, 12, 14 };

  point3D[0] = 0;
  point3D[1] = 12;
  point3D[2] = 14;
  sg2Dyz->GetPoint(sg2Dyz->FindPoint(point3D), point);
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
  vtkCell* found = sg2Dyz->FindAndGetCell(point3D, dummyCell, 0, 0, subId, pcoords, weights);
  if (found == nullptr)
  {
    strm << "FindAndGetCell(sg2Dyz) not found!" << endl;
    return EXIT_FAILURE;
  }

  strm << "FindAndGetCell(sg2Dyz): " << *found;
  strm << "pcoords: " << pcoords[0] << ", " << pcoords[1] << endl;
  strm << "weights: " << weights[0] << ", " << weights[1] << ", " << weights[2] << ", "
       << weights[3] << endl;

  // Test GetCellType

  strm << "GetCellType(sg2Dyz): " << sg2Dyz->GetCellType(0) << endl;

  // Test GetActualMemorySize

  strm << "GetActualMemorySize(sg2Dyz): " << sg2Dyz->GetActualMemorySize() << endl;

  return EXIT_SUCCESS;
}

int TestOSG_3d(ostream& strm)
{
  int i, j, k;
  vtkNew<vtkStructuredGrid> sg3D;

  vtkNew<vtkPoints> xyzpoints;
  for (k = 0; k < 20; k++)
  {
    for (j = 0; j < 20; j++)
    {
      for (i = 0; i < 20; i++)
      {
        xyzpoints->InsertNextPoint((double)i, (double)j, (double)k);
      }
    }
  }
  sg3D->SetDimensions(20, 20, 20);
  sg3D->SetPoints(xyzpoints);

  if (sg3D->GetCellSize(0) != 8)
  {
    std::cerr << "vtkStructuredGrid::GetCellSize(cellId) wrong for 3D structured grid.\n";
    return 1;
  }

  vtkNew<vtkShortArray> shortScalars3D;
  shortScalars3D->SetNumberOfComponents(3);
  shortScalars3D->SetNumberOfTuples(20 * 20 * 20);

  int l = 0;
  for (k = 0; k < 20; k++)
  {
    for (j = 0; j < 20; j++)
    {
      for (i = 0; i < 20; i++)
      {
        shortScalars3D->InsertComponent(l, 0, i);
        shortScalars3D->InsertComponent(l, 0, j);
        shortScalars3D->InsertComponent(l, 0, k);
        l++;
      }
    }
  }

  sg3D->GetPointData()->SetScalars(shortScalars3D);

  strm << "sg3D:" << *sg3D;

  // Test shallow copy
  vtkNew<vtkStructuredGrid> scsg3D;
  scsg3D->ShallowCopy(sg3D);
  strm << "ShallowCopy(sg3D):" << *scsg3D;

  // Test deep copy
  vtkNew<vtkStructuredGrid> dcsg3D;
  dcsg3D->DeepCopy(sg3D);
  strm << "DeepCopy(sg3D):" << *dcsg3D;

  // Test GetCell
  vtkNew<vtkIdList> ids;
  int cellId;
  int ii;

  i = 10;
  j = 15;
  k = 7;
  cellId = k * (19 * 19) + j * 19 + i;
  vtkCell* cell3D = sg3D->GetCell(cellId);
  strm << "cell3D: " << *cell3D;
  sg3D->GetCellPoints(cellId, ids);
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
  cell3D = sg3D->GetCell(i, j, k);
  if (cell3D == nullptr)
  {
    std::cerr << "vtkStructuredGrid::GetCell returned nullptr instead of a valid cell.\n";
    return EXIT_FAILURE;
  }
  if (cell3D->GetCellType() != VTK_HEXAHEDRON)
  {
    std::cerr << "vtkStructuredGrid::GetCell returned the wrong cell type.\n"
              << "Expected: " << VTK_HEXAHEDRON << " Returned: " << cell3D->GetCellType() << '\n';
    return EXIT_FAILURE;
  }
  strm << "cell3D: " << *cell3D;
  sg3D->GetCellPoints(cellId, ids);
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
  sg3D->GetCell(k * (19 * 19) + j * 19 + i, gcell3D);
  strm << "gcell3D: " << *gcell3D;

  // Test GetCellBounds

  double bounds[6];
  sg3D->GetCellBounds(k * (19 * 19) + j * 19 + i, bounds);
  strm << "GetCellBounds(sg3D): " << bounds[0] << ", " << bounds[1] << ", " << bounds[2] << ", "
       << bounds[3] << ", " << bounds[4] << ", " << bounds[5] << endl;

  // Test GetPoint

  double point[6];
  sg3D->GetPoint(k * (20 * 20) + j * 20 + i, point);
  strm << "GetPoint(sg3D): " << point[0] << ", " << point[1] << ", " << point[2] << endl;

  // Test FindPoint

  double point3D[3] = { 10, 12, 14 };

  sg3D->GetPoint(sg3D->FindPoint(point3D), point);
  strm << "FindPoint(" << point3D[0] << ", " << point3D[1] << ", " << point3D[2]
       << ") = " << point[0] << ", " << point[1] << ", " << point[2] << endl;

  // Test FindAndGetCell

  double pcoords[3], weights[8];
  int subId;
  vtkCell* dummyCell = nullptr;

  point3D[0] = 10.5;
  point3D[1] = 12.1;
  point3D[2] = 14.7;

  strm << "FindAndGetCell(sg3D): "
       << *sg3D->FindAndGetCell(point3D, dummyCell, 0, 0, subId, pcoords, weights);
  strm << "pcoords: " << pcoords[0] << ", " << pcoords[1] << ", " << pcoords[2] << endl;
  strm << "weights: " << weights[0] << ", " << weights[1] << ", " << weights[2] << ", "
       << weights[3] << ", " << weights[4] << ", " << weights[5] << ", " << weights[6] << ", "
       << weights[7] << endl;

  // Test GetCellType

  strm << "GetCellType(sg3D): " << sg3D->GetCellType(0) << endl;

  // Test GetActualMemorySize

  strm << "GetActualMemorySize(sg3D): " << sg3D->GetActualMemorySize() << endl;

  return EXIT_SUCCESS;
}

int TestOSG(ostream& strm)
{
  int ret = EXIT_SUCCESS;

  strm << "Testing vtkStructuredGrid" << endl;

  ret = TestOSG_0d(strm);
  if (ret != EXIT_SUCCESS)
  {
    return ret;
  }

  ret = TestOSG_1dx(strm);
  if (ret != EXIT_SUCCESS)
  {
    return ret;
  }

  ret = TestOSG_1dy(strm);
  if (ret != EXIT_SUCCESS)
  {
    return ret;
  }

  ret = TestOSG_1dz(strm);
  if (ret != EXIT_SUCCESS)
  {
    return ret;
  }

  ret = TestOSG_2dxy(strm);
  if (ret != EXIT_SUCCESS)
  {
    return ret;
  }

  ret = TestOSG_2dxz(strm);
  if (ret != EXIT_SUCCESS)
  {
    return ret;
  }

  ret = TestOSG_2dyz(strm);
  if (ret != EXIT_SUCCESS)
  {
    return ret;
  }

  ret = TestOSG_3d(strm);
  if (ret != EXIT_SUCCESS)
  {
    return ret;
  }

  strm << "Testing completed" << endl;
  return ret;
}

int otherStructuredGrid(int, char*[])
{
  std::ostringstream vtkmsg_with_warning_C4701;
  return TestOSG(vtkmsg_with_warning_C4701);
}
