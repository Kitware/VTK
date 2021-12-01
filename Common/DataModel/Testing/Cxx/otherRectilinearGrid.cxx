/*=========================================================================

  Program:   Visualization Toolkit
  Module:    otherRectilinearGrid.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME
// .SECTION Description
// this program tests vtkRectilinearGrid

#include "vtkCell.h"
#include "vtkDebugLeaks.h"
#include "vtkDoubleArray.h"
#include "vtkGenericCell.h"
#include "vtkIdList.h"
#include "vtkLongArray.h"
#include "vtkMathUtilities.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkRectilinearGrid.h"
#include "vtkShortArray.h"

#include <sstream>

int test_rg3d(ostream& strm)
{
  int i, j, k;
  // actual test
  strm << "Testing vtkRectilinearGrid 3D" << endl;
  vtkNew<vtkRectilinearGrid> rg3D;

  vtkNew<vtkDoubleArray> xdata;
  vtkNew<vtkDoubleArray> ydata;
  vtkNew<vtkDoubleArray> zdata;

  for (i = 0; i < 20; i++)
  {
    xdata->InsertNextValue((double)i);
    ydata->InsertNextValue((double)i);
    zdata->InsertNextValue((double)i);
  }

  rg3D->SetDimensions(20, 20, 20);
  rg3D->SetXCoordinates(xdata);
  rg3D->SetYCoordinates(ydata);
  rg3D->SetZCoordinates(zdata);

  if (rg3D->GetCellSize(0) != 8)
  {
    std::cerr << "vtkRectilinearGrid::GetCellSize(cellId) wrong for 3D rectilinear grids.\n";
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

  rg3D->GetPointData()->SetScalars(shortScalars3D);

  strm << "rg3D:" << *rg3D;

  // Test shallow copy
  vtkNew<vtkRectilinearGrid> scrg3D;
  scrg3D->ShallowCopy(rg3D);
  strm << "ShallowCopy(rg3D):" << *scrg3D;

  // Test deep copy
  vtkNew<vtkRectilinearGrid> dcrg3D;
  dcrg3D->DeepCopy(rg3D);
  strm << "DeepCopy(rg3D):" << *dcrg3D;

  // Test GetCell
  vtkNew<vtkIdList> ids;
  int cellId;
  int ii;

  i = 10;
  j = 15;
  k = 7;
  cellId = k * (19 * 19) + j * 19 + i;
  vtkSmartPointer<vtkCell> cell3D = rg3D->GetCell(cellId);
  strm << "cell3D: " << *cell3D;
  rg3D->GetCellPoints(cellId, ids);
  strm << "Ids for cell " << cellId << " are ";
  for (ii = 0; ii < ids->GetNumberOfIds(); ii++)
  {
    strm << ids->GetId(ii) << " ";
  }
  strm << endl << endl;

  // Test GetCell(i,j,k)
  double bounds[6];

  i = 10;
  j = 15;
  k = 7;
  cellId = k * (19 * 19) + j * 19 + i;
  cell3D = rg3D->GetCell(i, j, k);
  if (cell3D == nullptr)
  {
    std::cerr << "vtkRectilinearGrid::GetCell returned nullptr instead of a valid cell.\n";
    return 1;
  }
  if (cell3D->GetCellType() != VTK_VOXEL)
  {
    std::cerr << "vtkRectilinearGrid::GetCell returned the wrong cell type.\n"
              << "Expected: " << VTK_VOXEL << " Returned: " << cell3D->GetCellType() << '\n';
    return 1;
  }
  strm << "cell3D: " << *cell3D;
  rg3D->GetCellPoints(cellId, ids);
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
  rg3D->GetCell(k * (19 * 19) + j * 19 + i, gcell3D);
  strm << "gcell3D: " << *gcell3D;

  // Test GetCellBounds
  rg3D->GetCellBounds(k * (19 * 19) + j * 19 + i, bounds);
  strm << "GetCellBounds(rg3D): " << bounds[0] << ", " << bounds[1] << ", " << bounds[2] << ", "
       << bounds[3] << ", " << bounds[4] << ", " << bounds[5] << endl;

  // Test GetPoint

  double point[6];
  rg3D->GetPoint(k * (20 * 20) + j * 20 + i, point);
  strm << "GetPoint(rg3D): " << point[0] << ", " << point[1] << ", " << point[2] << endl;

  // Test FindPoint

  double point3D[3] = { 10, 12, 14 };

  rg3D->GetPoint(rg3D->FindPoint(point3D), point);
  strm << "FindPoint(" << point3D[0] << ", " << point3D[1] << ", " << point3D[2]
       << ") = " << point[0] << ", " << point[1] << ", " << point[2] << endl;

  // Test FindAndGetCell

  double pcoords[3], weights[8];
  int subId;
  vtkCell* dummyCell = nullptr;

  point3D[0] = 10.5;
  point3D[1] = 12.1;
  point3D[2] = 14.7;

  strm << "FindAndGetCell(rg3D): "
       << *rg3D->FindAndGetCell(point3D, dummyCell, 0, 0, subId, pcoords, weights);
  strm << "pcoords: " << pcoords[0] << ", " << pcoords[1] << ", " << pcoords[2] << endl;
  strm << "weights: " << weights[0] << ", " << weights[1] << ", " << weights[2] << ", "
       << weights[3] << ", " << weights[4] << ", " << weights[5] << ", " << weights[6] << ", "
       << weights[7] << endl;

  // Test GetCellType

  strm << "GetCellType(rg3D): " << rg3D->GetCellType(0) << endl;

  // Test GetActualMemorySize

  strm << "GetActualMemorySize(rg3D): " << rg3D->GetActualMemorySize() << endl;

  return EXIT_SUCCESS;
}

int test_rg2d_xy(ostream& strm)
{
  int i, j;
  // actual test
  strm << "Testing vtkRectilinearGrid 2D (xy)" << endl;
  vtkNew<vtkRectilinearGrid> rg2Dxy;

  vtkNew<vtkDoubleArray> xdata;
  vtkNew<vtkDoubleArray> ydata;

  for (i = 0; i < 20; i++)
  {
    xdata->InsertNextValue((double)i);
    ydata->InsertNextValue((double)i);
  }

  rg2Dxy->SetDimensions(20, 20, 1);
  rg2Dxy->SetXCoordinates(xdata);
  rg2Dxy->SetYCoordinates(ydata);

  if (rg2Dxy->GetCellSize(0) != 4)
  {
    std::cerr << "vtkRectilinearGrid::GetCellSize(cellId) wrong for 2D rectilinear grids.\n";
    return 1;
  }

  vtkNew<vtkShortArray> shortScalars3D;

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

  rg2Dxy->GetPointData()->SetScalars(shortScalars2D);

  // Test GetCell
  vtkNew<vtkIdList> ids;
  int cellId;
  int ii;

  i = 10;
  j = 15;
  cellId = j * 19 + i;
  vtkSmartPointer<vtkCell> cell2D = rg2Dxy->GetCell(cellId);
  strm << "cell2D: " << *cell2D;
  rg2Dxy->GetCellPoints(cellId, ids);
  strm << "Ids for cell " << cellId << " are ";
  for (ii = 0; ii < ids->GetNumberOfIds(); ii++)
  {
    strm << ids->GetId(ii) << " ";
  }
  strm << endl << endl;

  // Test GetCell(i,j,k)
  double bounds[6];

  i = 10;
  j = 15;
  cellId = j * 19 + i;
  cell2D = rg2Dxy->GetCell(i, j, 0);
  if (cell2D == nullptr)
  {
    std::cerr << "vtkRectilinearGrid::GetCell returned nullptr instead of a valid cell.\n";
    return 1;
  }
  if (cell2D->GetCellType() != VTK_PIXEL)
  {
    std::cerr << "vtkRectilinearGrid::GetCell returned the wrong cell type.\n"
              << "Expected: " << VTK_PIXEL << " Returned: " << cell2D->GetCellType() << '\n';
    return 1;
  }
  cell2D->GetBounds(bounds);
  if (!vtkMathUtilities::FuzzyCompare(bounds[4], bounds[5]))
  {
    std::cerr << "rg2Dxy has finite width along z\n";
    return 1;
  }
  strm << "cell2D: " << *cell2D;
  rg2Dxy->GetCellPoints(cellId, ids);
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
  rg2Dxy->GetCell(j * 19 + i, gcell2D);
  strm << "gcell2D: " << *gcell2D;

  // Test GetCellBounds

  rg2Dxy->GetCellBounds(j * 19 + i, bounds);
  strm << "GetCellBounds(rg2Dxy): " << bounds[0] << ", " << bounds[1] << ", " << bounds[2] << ", "
       << bounds[3] << ", " << bounds[4] << ", " << bounds[5] << endl;

  // Test GetPoint

  double point[6];

  rg2Dxy->GetPoint(j * 20 + i, point);
  strm << "GetPoint(rg2Dxy): " << point[0] << ", " << point[1] << ", " << point[2] << endl;

  // Test FindPoint

  double point3D[3] = { 10, 12, 14 };

  point3D[0] = 10;
  point3D[1] = 12;
  point3D[2] = 0;
  rg2Dxy->GetPoint(rg2Dxy->FindPoint(point3D), point);
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
  vtkCell* found = rg2Dxy->FindAndGetCell(point3D, dummyCell, 0, 0, subId, pcoords, weights);
  if (found == nullptr)
  {
    strm << "FindAndGetCell(rg2Dxy) not found!" << endl;
    return 1;
  }

  strm << "FindAndGetCell(rg2Dxy): " << *found;
  strm << "pcoords: " << pcoords[0] << ", " << pcoords[1] << endl;
  strm << "weights: " << weights[0] << ", " << weights[1] << ", " << weights[2] << ", "
       << weights[3] << endl;

  // Test GetCellType

  strm << "GetCellType(rg2Dxy): " << rg2Dxy->GetCellType(0) << endl;

  // Test GetActualMemorySize

  strm << "GetActualMemorySize(rg2Dxy): " << rg2Dxy->GetActualMemorySize() << endl;

  strm << "Testing completed" << endl;
  return EXIT_SUCCESS;
}

int test_rg2d_xz(ostream& strm)
{
  int i, j;
  // actual test
  strm << "Testing vtkRectilinearGrid" << endl;
  vtkNew<vtkRectilinearGrid> rg2Dxz;

  vtkNew<vtkDoubleArray> xdata;
  vtkNew<vtkDoubleArray> zdata;

  for (i = 0; i < 20; i++)
  {
    xdata->InsertNextValue((double)i);
    zdata->InsertNextValue((double)i);
  }

  rg2Dxz->SetDimensions(20, 1, 20);
  rg2Dxz->SetXCoordinates(xdata);
  rg2Dxz->SetZCoordinates(zdata);

  if (rg2Dxz->GetCellSize(0) != 4)
  {
    std::cerr << "vtkRectilinearGrid::GetCellSize(cellId) wrong for 2D rectilinear grids.\n";
    return 1;
  }

  vtkNew<vtkShortArray> shortScalars3D;

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

  rg2Dxz->GetPointData()->SetScalars(shortScalars2D);

  // Test GetCell
  vtkNew<vtkIdList> ids;
  int cellId;
  int ii;

  i = 10;
  j = 15;
  cellId = j * 19 + i;
  vtkSmartPointer<vtkCell> cell2D = rg2Dxz->GetCell(j * 19 + i);
  strm << "cell2D: " << *cell2D;
  rg2Dxz->GetCellPoints(cellId, ids);
  strm << "Ids for cell " << cellId << " are ";
  for (ii = 0; ii < ids->GetNumberOfIds(); ii++)
  {
    strm << ids->GetId(ii) << " ";
  }
  strm << endl << endl;

  // Test GetCell(i,j,k)
  double bounds[6];

  i = 10;
  j = 15;
  cellId = j * 19 + i;
  cell2D = rg2Dxz->GetCell(i, 0, j);
  if (cell2D == nullptr)
  {
    std::cerr << "vtkRectilinearGrid::GetCell returned nullptr instead of a valid cell.\n";
    return 1;
  }
  if (cell2D->GetCellType() != VTK_PIXEL)
  {
    std::cerr << "vtkRectilinearGrid::GetCell returned the wrong cell type.\n"
              << "Expected: " << VTK_PIXEL << " Returned: " << cell2D->GetCellType() << '\n';
    return 1;
  }
  cell2D->GetBounds(bounds);
  if (!vtkMathUtilities::FuzzyCompare(bounds[2], bounds[3]))
  {
    std::cerr << "rg2Dxz has finite width along y\n";
    return 1;
  }
  strm << "cell2D: " << *cell2D;
  rg2Dxz->GetCellPoints(cellId, ids);
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
  rg2Dxz->GetCell(j * 19 + i, gcell2D);
  strm << "gcell2D: " << *gcell2D;

  // Test GetCellBounds

  rg2Dxz->GetCellBounds(j * 19 + i, bounds);
  strm << "GetCellBounds(rg2Dxz): " << bounds[0] << ", " << bounds[1] << ", " << bounds[2] << ", "
       << bounds[3] << ", " << bounds[4] << ", " << bounds[5] << endl;

  // Test GetPoint

  double point[6];

  rg2Dxz->GetPoint(j * 20 + i, point);
  strm << "GetPoint(rg2Dxz): " << point[0] << ", " << point[1] << ", " << point[2] << endl;

  // Test FindPoint

  double point3D[3] = { 10, 12, 14 };

  point3D[0] = 10;
  point3D[1] = 0;
  point3D[2] = 14;
  rg2Dxz->GetPoint(rg2Dxz->FindPoint(point3D), point);
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
  vtkCell* found = rg2Dxz->FindAndGetCell(point3D, dummyCell, 0, 0, subId, pcoords, weights);
  if (found == nullptr)
  {
    strm << "FindAndGetCell(rg2Dxz) not found!" << endl;
    return 1;
  }

  strm << "FindAndGetCell(rg2Dxz): " << *found;
  strm << "pcoords: " << pcoords[0] << ", " << pcoords[1] << endl;
  strm << "weights: " << weights[0] << ", " << weights[1] << ", " << weights[2] << ", "
       << weights[3] << endl;

  // Test GetCellType

  strm << "GetCellType(rg2Dxz): " << rg2Dxz->GetCellType(0) << endl;

  // Test GetActualMemorySize

  strm << "GetActualMemorySize(rg2Dxz): " << rg2Dxz->GetActualMemorySize() << endl;

  strm << "Testing completed" << endl;
  return EXIT_SUCCESS;
}

int test_rg2d_yz(ostream& strm)
{
  int i, j;
  // actual test
  strm << "Testing vtkRectilinearGrid" << endl;
  vtkNew<vtkRectilinearGrid> rg2Dyz;

  vtkNew<vtkDoubleArray> ydata;
  vtkNew<vtkDoubleArray> zdata;

  for (i = 0; i < 20; i++)
  {
    ydata->InsertNextValue((double)i);
    zdata->InsertNextValue((double)i);
  }

  rg2Dyz->SetDimensions(1, 20, 20);
  rg2Dyz->SetYCoordinates(ydata);
  rg2Dyz->SetZCoordinates(zdata);

  if (rg2Dyz->GetCellSize(0) != 4)
  {
    std::cerr << "vtkRectilinearGrid::GetCellSize(cellId) wrong for 2D rectilinear grids.\n";
    return 1;
  }

  vtkNew<vtkShortArray> shortScalars3D;

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

  rg2Dyz->GetPointData()->SetScalars(shortScalars2D);

  // Test GetCell
  vtkNew<vtkIdList> ids;
  int cellId;
  int ii;

  i = 10;
  j = 15;
  cellId = j * 19 + i;
  vtkSmartPointer<vtkCell> cell2D = rg2Dyz->GetCell(j * 19 + i);
  strm << "cell2D: " << *cell2D;
  rg2Dyz->GetCellPoints(cellId, ids);
  strm << "Ids for cell " << cellId << " are ";
  for (ii = 0; ii < ids->GetNumberOfIds(); ii++)
  {
    strm << ids->GetId(ii) << " ";
  }
  strm << endl << endl;

  // Test GetCell(i,j,k)
  double bounds[6];

  i = 10;
  j = 15;
  cellId = j * 19 + i;
  cell2D = rg2Dyz->GetCell(0, i, j);
  if (cell2D == nullptr)
  {
    std::cerr << "vtkRectilinearGrid::GetCell returned nullptr instead of a valid cell.\n";
    return 1;
  }
  if (cell2D->GetCellType() != VTK_PIXEL)
  {
    std::cerr << "vtkRectilinearGrid::GetCell returned the wrong cell type.\n"
              << "Expected: " << VTK_PIXEL << " Returned: " << cell2D->GetCellType() << '\n';
    return 1;
  }
  cell2D->GetBounds(bounds);
  if (!vtkMathUtilities::FuzzyCompare(bounds[0], bounds[1]))
  {
    std::cerr << "rg2Dyz has finite width along x\n";
    return 1;
  }
  strm << "cell2D: " << *cell2D;
  rg2Dyz->GetCellPoints(cellId, ids);
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
  rg2Dyz->GetCell(j * 19 + i, gcell2D);
  strm << "gcell2D: " << *gcell2D;

  // Test GetCellBounds

  rg2Dyz->GetCellBounds(j * 19 + i, bounds);
  strm << "GetCellBounds(rg2Dyz): " << bounds[0] << ", " << bounds[1] << ", " << bounds[2] << ", "
       << bounds[3] << ", " << bounds[4] << ", " << bounds[5] << endl;

  // Test GetPoint

  double point[6];

  rg2Dyz->GetPoint(j * 20 + i, point);
  strm << "GetPoint(rg2Dyz): " << point[0] << ", " << point[1] << ", " << point[2] << endl;

  // Test FindPoint

  double point3D[3] = { 10, 12, 14 };

  point3D[0] = 0;
  point3D[1] = 12;
  point3D[2] = 14;
  rg2Dyz->GetPoint(rg2Dyz->FindPoint(point3D), point);
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
  vtkCell* found = rg2Dyz->FindAndGetCell(point3D, dummyCell, 0, 0, subId, pcoords, weights);
  if (found == nullptr)
  {
    strm << "FindAndGetCell(rg2Dyz) not found!" << endl;
    return 1;
  }

  strm << "FindAndGetCell(rg2Dyz): " << *found;
  strm << "pcoords: " << pcoords[0] << ", " << pcoords[1] << endl;
  strm << "weights: " << weights[0] << ", " << weights[1] << ", " << weights[2] << ", "
       << weights[3] << endl;

  // Test GetCellType

  strm << "GetCellType(rg2Dyz): " << rg2Dyz->GetCellType(0) << endl;

  // Test GetActualMemorySize

  strm << "GetActualMemorySize(rg2Dyz): " << rg2Dyz->GetActualMemorySize() << endl;

  strm << "Testing completed" << endl;
  return EXIT_SUCCESS;
}

int test_rg1d_x(ostream& strm)
{
  int i;
  // actual test
  strm << "Testing vtkRectilinearGrid" << endl;
  vtkNew<vtkRectilinearGrid> rg1Dx;

  vtkNew<vtkDoubleArray> xdata;

  for (i = 0; i < 20; i++)
  {
    xdata->InsertNextValue((double)i);
  }

  rg1Dx->SetDimensions(20, 1, 1);
  rg1Dx->SetXCoordinates(xdata);

  if (rg1Dx->GetCellSize(0) != 2)
  {
    std::cerr << "vtkRectilinearGrid::GetCellSize(cellId) wrong for 1D rectilinear grids.\n";
    return 1;
  }

  vtkNew<vtkShortArray> shortScalars3D;

  vtkNew<vtkShortArray> shortScalars1D;
  shortScalars1D->SetNumberOfComponents(1);
  shortScalars1D->SetNumberOfTuples(20);

  int l = 0;
  for (i = 0; i < 20; i++)
  {
    shortScalars1D->InsertComponent(l, 0, i);
    l++;
  }

  rg1Dx->GetPointData()->SetScalars(shortScalars1D);

  // Test GetCell
  vtkNew<vtkIdList> ids;
  int cellId;
  int ii;

  i = 10;
  cellId = i;
  vtkSmartPointer<vtkCell> cell1D = rg1Dx->GetCell(i);
  strm << "cell1D: " << *cell1D;
  rg1Dx->GetCellPoints(cellId, ids);
  strm << "Ids for cell " << cellId << " are ";
  for (ii = 0; ii < ids->GetNumberOfIds(); ii++)
  {
    strm << ids->GetId(ii) << " ";
  }
  strm << endl << endl;

  // Test GetCell(i,j,k)
  double bounds[6];

  i = 10;
  cellId = i;
  cell1D = rg1Dx->GetCell(i, 0, 0);
  if (cell1D == nullptr)
  {
    std::cerr << "vtkRectilinearGrid::GetCell returned nullptr instead of a valid cell.\n";
    return 1;
  }
  if (cell1D->GetCellType() != VTK_LINE)
  {
    std::cerr << "vtkRectilinearGrid::GetCell returned the wrong cell type.\n"
              << "Expected: " << VTK_LINE << " Returned: " << cell1D->GetCellType() << '\n';
    return 1;
  }
  cell1D->GetBounds(bounds);
  if (!vtkMathUtilities::FuzzyCompare(bounds[2], bounds[3]))
  {
    std::cerr << "rg1Dx has finite width along y\n";
    return 1;
  }
  else if (!vtkMathUtilities::FuzzyCompare(bounds[4], bounds[5]))
  {
    std::cerr << "rg1Dx has finite width along z\n";
    return 1;
  }
  strm << "cell1D: " << *cell1D;
  rg1Dx->GetCellPoints(cellId, ids);
  strm << "Ids for cell " << cellId << " are ";
  for (ii = 0; ii < ids->GetNumberOfIds(); ii++)
  {
    strm << ids->GetId(ii) << " ";
  }
  strm << endl << endl;

  // Test Thread Safe GetCell
  vtkNew<vtkGenericCell> gcell1D;

  i = 10;
  rg1Dx->GetCell(i, gcell1D);
  strm << "gcell1D: " << *gcell1D;

  // Test GetCellBounds

  rg1Dx->GetCellBounds(i, bounds);
  strm << "GetCellBounds(rg1x): " << bounds[0] << ", " << bounds[1] << ", " << bounds[2] << ", "
       << bounds[3] << ", " << bounds[4] << ", " << bounds[5] << endl;

  // Test GetPoint

  double point[6];

  rg1Dx->GetPoint(i, point);
  strm << "GetPoint(rg1x): " << point[0] << ", " << point[1] << ", " << point[2] << endl;

  // Test FindPoint

  double point3D[3] = { 10, 12, 14 };

  point3D[0] = 10;
  point3D[1] = 0;
  point3D[2] = 0;
  rg1Dx->GetPoint(rg1Dx->FindPoint(point3D), point);
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
  vtkCell* found = rg1Dx->FindAndGetCell(point3D, dummyCell, 0, 0, subId, pcoords, weights);
  if (found == nullptr)
  {
    strm << "FindAndGetCell(rg1Dx) not found!" << endl;
    return 1;
  }

  strm << "FindAndGetCell(rg1Dx): " << *found;
  strm << "pcoords: " << pcoords[0] << endl;
  strm << "weights: " << weights[0] << ", " << weights[1] << endl;

  // Test GetCellType

  strm << "GetCellType(rg1Dx): " << rg1Dx->GetCellType(0) << endl;

  // Test GetActualMemorySize

  strm << "GetActualMemorySize(rg1Dx): " << rg1Dx->GetActualMemorySize() << endl;

  strm << "Testing completed" << endl;
  return EXIT_SUCCESS;
}

int test_rg1d_y(ostream& strm)
{
  int i;
  // actual test
  strm << "Testing vtkRectilinearGrid" << endl;
  vtkNew<vtkRectilinearGrid> rg1Dy;

  vtkNew<vtkDoubleArray> ydata;

  for (i = 0; i < 20; i++)
  {
    ydata->InsertNextValue((double)i);
  }

  rg1Dy->SetDimensions(1, 20, 1);
  rg1Dy->SetYCoordinates(ydata);
  strm << *rg1Dy;

  if (rg1Dy->GetCellSize(0) != 2)
  {
    std::cerr << "vtkRectilinearGrid::GetCellSize(cellId) wrong for 1D rectilinear grids.\n";
    return 1;
  }

  vtkNew<vtkShortArray> shortScalars3D;

  vtkNew<vtkShortArray> shortScalars1D;
  shortScalars1D->SetNumberOfComponents(1);
  shortScalars1D->SetNumberOfTuples(20);

  int l = 0;
  for (i = 0; i < 20; i++)
  {
    shortScalars1D->InsertComponent(l, 0, i);
    l++;
  }

  rg1Dy->GetPointData()->SetScalars(shortScalars1D);

  // Test GetCell
  vtkNew<vtkIdList> ids;
  int cellId;
  int ii;

  i = 10;
  cellId = i;
  vtkSmartPointer<vtkCell> cell1D = rg1Dy->GetCell(i);
  strm << "cell1D: " << *cell1D;
  rg1Dy->GetCellPoints(cellId, ids);
  strm << "Ids for cell " << cellId << " are ";
  for (ii = 0; ii < ids->GetNumberOfIds(); ii++)
  {
    strm << ids->GetId(ii) << " ";
  }
  strm << endl << endl;

  // Test GetCell(i,j,k)
  double bounds[6];

  i = 10;
  cellId = i;
  cell1D = rg1Dy->GetCell(0, i, 0);
  if (cell1D == nullptr)
  {
    std::cerr << "vtkRectilinearGrid::GetCell returned nullptr instead of a valid cell.\n";
    return 1;
  }
  if (cell1D->GetCellType() != VTK_LINE)
  {
    std::cerr << "vtkRectilinearGrid::GetCell returned the wrong cell type.\n"
              << "Expected: " << VTK_LINE << " Returned: " << cell1D->GetCellType() << '\n';
    return 1;
  }
  cell1D->GetBounds(bounds);
  if (!vtkMathUtilities::FuzzyCompare(bounds[0], bounds[1]))
  {
    std::cerr << "rg1Dy has finite width along x\n";
    return 1;
  }
  else if (!vtkMathUtilities::FuzzyCompare(bounds[4], bounds[5]))
  {
    std::cerr << "rg1Dy has finite width along z\n";
    return 1;
  }
  strm << "cell1D: " << *cell1D;
  rg1Dy->GetCellPoints(cellId, ids);
  strm << "Ids for cell " << cellId << " are ";
  for (ii = 0; ii < ids->GetNumberOfIds(); ii++)
  {
    strm << ids->GetId(ii) << " ";
  }
  strm << endl << endl;

  // Test Thread Safe GetCell
  vtkNew<vtkGenericCell> gcell1D;

  i = 10;
  rg1Dy->GetCell(i, gcell1D);
  strm << "gcell1D: " << *gcell1D;

  // Test GetCellBounds

  rg1Dy->GetCellBounds(i, bounds);
  strm << "GetCellBounds(rg1Dy): " << bounds[0] << ", " << bounds[1] << ", " << bounds[2] << ", "
       << bounds[3] << ", " << bounds[4] << ", " << bounds[5] << endl;

  // Test GetPoint

  double point[6];

  rg1Dy->GetPoint(i, point);
  strm << "GetPoint(rg1Dy): " << point[0] << ", " << point[1] << ", " << point[2] << endl;

  // Test FindPoint

  double point3D[3] = { 10, 12, 14 };

  point3D[0] = 0;
  point3D[1] = 12;
  point3D[2] = 0;
  rg1Dy->GetPoint(rg1Dy->FindPoint(point3D), point);
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
  vtkCell* found = rg1Dy->FindAndGetCell(point3D, dummyCell, 0, 0, subId, pcoords, weights);
  if (found == nullptr)
  {
    strm << "FindAndGetCell(rg1Dy) not found!" << endl;
    return 1;
  }

  strm << "FindAndGetCell(rg1Dy): " << *found;
  strm << "pcoords: " << pcoords[0] << endl;
  strm << "weights: " << weights[0] << ", " << weights[1] << endl;

  // Test GetCellType

  strm << "GetCellType(rg1Dy): " << rg1Dy->GetCellType(0) << endl;

  // Test GetActualMemorySize

  strm << "GetActualMemorySize(rg1Dy): " << rg1Dy->GetActualMemorySize() << endl;

  strm << "Testing completed" << endl;
  return EXIT_SUCCESS;
}

int test_rg1d_z(ostream& strm)
{
  int i;
  // actual test
  strm << "Testing vtkRectilinearGrid" << endl;
  vtkNew<vtkRectilinearGrid> rg1Dz;

  vtkNew<vtkDoubleArray> zdata;

  for (i = 0; i < 20; i++)
  {
    zdata->InsertNextValue((double)i);
  }

  rg1Dz->SetDimensions(1, 1, 20);
  rg1Dz->SetZCoordinates(zdata);

  if (rg1Dz->GetCellSize(0) != 2)
  {
    std::cerr << "vtkRectilinearGrid::GetCellSize(cellId) wrong for 1D rectilinear grids.\n";
    return 1;
  }

  vtkNew<vtkShortArray> shortScalars3D;

  vtkNew<vtkShortArray> shortScalars1D;
  shortScalars1D->SetNumberOfComponents(1);
  shortScalars1D->SetNumberOfTuples(20);

  int l = 0;
  for (i = 0; i < 20; i++)
  {
    shortScalars1D->InsertComponent(l, 0, i);
    l++;
  }

  rg1Dz->GetPointData()->SetScalars(shortScalars1D);

  // Test GetCell
  vtkNew<vtkIdList> ids;
  int cellId;
  int ii;

  i = 10;
  cellId = i;
  vtkSmartPointer<vtkCell> cell1D = rg1Dz->GetCell(i);
  strm << "cell1D: " << *cell1D;
  rg1Dz->GetCellPoints(cellId, ids);
  strm << "Ids for cell " << cellId << " are ";
  for (ii = 0; ii < ids->GetNumberOfIds(); ii++)
  {
    strm << ids->GetId(ii) << " ";
  }
  strm << endl << endl;

  // Test GetCell(i,j,k)
  double bounds[6];

  i = 10;
  cellId = i;
  cell1D = rg1Dz->GetCell(0, 0, i);
  if (cell1D == nullptr)
  {
    std::cerr << "vtkRectilinearGrid::GetCell returned nullptr instead of a valid cell.\n";
    return 1;
  }
  if (cell1D->GetCellType() != VTK_LINE)
  {
    std::cerr << "vtkRectilinearGrid::GetCell returned the wrong cell type.\n"
              << "Expected: " << VTK_LINE << " Returned: " << cell1D->GetCellType() << '\n';
    return 1;
  }
  cell1D->GetBounds(bounds);
  if (!vtkMathUtilities::FuzzyCompare(bounds[0], bounds[1]))
  {
    std::cerr << "rg1Dz has finite width along x\n";
    return 1;
  }
  else if (!vtkMathUtilities::FuzzyCompare(bounds[2], bounds[3]))
  {
    std::cerr << "rg1Dz has finite width along y\n";
    return 1;
  }
  strm << "cell1D: " << *cell1D;
  rg1Dz->GetCellPoints(cellId, ids);
  strm << "Ids for cell " << cellId << " are ";
  for (ii = 0; ii < ids->GetNumberOfIds(); ii++)
  {
    strm << ids->GetId(ii) << " ";
  }
  strm << endl << endl;

  // Test Thread Safe GetCell
  vtkNew<vtkGenericCell> gcell1D;

  i = 10;
  rg1Dz->GetCell(i, gcell1D);
  strm << "gcell1D: " << *gcell1D;

  // Test GetCellBounds

  rg1Dz->GetCellBounds(i, bounds);
  strm << "GetCellBounds(rg1Dz): " << bounds[0] << ", " << bounds[1] << ", " << bounds[2] << ", "
       << bounds[3] << ", " << bounds[4] << ", " << bounds[5] << endl;

  // Test GetPoint

  double point[6];

  rg1Dz->GetPoint(i, point);
  strm << "GetPoint(rg1Dz): " << point[0] << ", " << point[1] << ", " << point[2] << endl;

  // Test FindPoint

  double point3D[3] = { 10, 12, 14 };

  point3D[0] = 0;
  point3D[1] = 0;
  point3D[2] = 14;
  rg1Dz->GetPoint(rg1Dz->FindPoint(point3D), point);
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
  vtkCell* found = rg1Dz->FindAndGetCell(point3D, dummyCell, 0, 0, subId, pcoords, weights);
  if (found == nullptr)
  {
    strm << "FindAndGetCell(rg1Dz) not found!" << endl;
    return 1;
  }

  strm << "FindAndGetCell(rg1Dz): " << *found;
  strm << "pcoords: " << pcoords[0] << endl;
  strm << "weights: " << weights[0] << ", " << weights[1] << endl;

  // Test GetCellType

  strm << "GetCellType(rg1Dz): " << rg1Dz->GetCellType(0) << endl;

  // Test GetActualMemorySize

  strm << "GetActualMemorySize(rg1Dz): " << rg1Dz->GetActualMemorySize() << endl;

  strm << "Testing completed" << endl;
  return EXIT_SUCCESS;
}

int test_rg0d(ostream& strm)
{
  int i;
  // actual test
  strm << "Testing vtkRectilinearGrid" << endl;
  vtkNew<vtkRectilinearGrid> rg0D;

  rg0D->SetDimensions(1, 1, 1);

  if (rg0D->GetCellSize(0) != 1)
  {
    std::cerr << "vtkRectilinearGrid::GetCellSize(cellId) wrong for 0D rectilinear grids.\n";
    return 1;
  }

  vtkNew<vtkShortArray> shortScalars3D;

  vtkNew<vtkShortArray> shortScalars0D;
  shortScalars0D->SetNumberOfComponents(1);
  shortScalars0D->SetNumberOfTuples(1);

  int l = 0;
  shortScalars0D->InsertComponent(l, 0, 0);

  rg0D->GetPointData()->SetScalars(shortScalars0D);

  // Test GetCell
  vtkNew<vtkIdList> ids;
  int cellId;
  int ii;

  cellId = 0;
  vtkSmartPointer<vtkCell> cell0D = rg0D->GetCell(0);
  strm << "cell0D: " << *cell0D;
  rg0D->GetCellPoints(cellId, ids);
  strm << "Ids for cell " << cellId << " are ";
  for (ii = 0; ii < ids->GetNumberOfIds(); ii++)
  {
    strm << ids->GetId(ii) << " ";
  }
  strm << endl << endl;

  // Test GetCell(i,j,k)
  double bounds[6];

  cellId = 0;
  cell0D = rg0D->GetCell(0, 0, 0);
  if (cell0D == nullptr)
  {
    std::cerr << "vtkRectilinearGrid::GetCell returned nullptr instead of a valid cell.\n";
    return 1;
  }
  if (cell0D->GetCellType() != VTK_VERTEX)
  {
    std::cerr << "vtkRectilinearGrid::GetCell returned the wrong cell type.\n"
              << "Expected: " << VTK_VERTEX << " Returned: " << cell0D->GetCellType() << '\n';
    return 1;
  }
  strm << "cell0D: " << *cell0D;
  rg0D->GetCellPoints(cellId, ids);
  strm << "Ids for cell " << cellId << " are ";
  for (ii = 0; ii < ids->GetNumberOfIds(); ii++)
  {
    strm << ids->GetId(ii) << " ";
  }
  strm << endl << endl;

  // Test Thread Safe GetCell
  vtkNew<vtkGenericCell> gcell0D;

  i = 10;
  rg0D->GetCell(0, gcell0D);
  strm << "gcell0D: " << *gcell0D;

  // Test GetCellBounds

  rg0D->GetCellBounds(i, bounds);
  strm << "GetCellBounds(rg0D): " << bounds[0] << ", " << bounds[1] << ", " << bounds[2] << ", "
       << bounds[3] << ", " << bounds[4] << ", " << bounds[5] << endl;

  // Test GetPoint

  double point[6];

  rg0D->GetPoint(0, point);
  strm << "GetPoint(rg0D): " << point[0] << ", " << point[1] << ", " << point[2] << endl;

  // Test GetCellType

  strm << "GetCellType(rg0D): " << rg0D->GetCellType(0) << endl;

  // Test GetActualMemorySize

  strm << "GetActualMemorySize(rg0D): " << rg0D->GetActualMemorySize() << endl;

  strm << "Testing completed" << endl;
  return EXIT_SUCCESS;
}

int TestORG(ostream& strm)
{
  int ret = EXIT_SUCCESS;
  // actual test
  strm << "Testing vtkRectilinearGrid" << endl;

  ret = test_rg3d(strm);
  if (ret != EXIT_SUCCESS)
  {
    return ret;
  }

  ret = test_rg2d_xy(strm);
  if (ret != EXIT_SUCCESS)
  {
    return ret;
  }

  ret = test_rg2d_xz(strm);
  if (ret != EXIT_SUCCESS)
  {
    return ret;
  }

  ret = test_rg2d_yz(strm);
  if (ret != EXIT_SUCCESS)
  {
    return ret;
  }

  ret = test_rg1d_x(strm);
  if (ret != EXIT_SUCCESS)
  {
    return ret;
  }

  ret = test_rg1d_y(strm);
  if (ret != EXIT_SUCCESS)
  {
    return ret;
  }

  ret = test_rg1d_z(strm);
  if (ret != EXIT_SUCCESS)
  {
    return ret;
  }

  ret = test_rg0d(strm);
  if (ret != EXIT_SUCCESS)
  {
    return ret;
  }

  strm << "Testing completed" << endl;
  return ret;
}

int otherRectilinearGrid(int, char*[])
{
  std::ostringstream vtkmsg_with_warning_C4701;
  return TestORG(vtkmsg_with_warning_C4701);
}
