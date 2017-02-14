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
// this program tests vtkImageData

#include "vtkCell.h"
#include "vtkDebugLeaks.h"
#include "vtkGenericCell.h"
#include "vtkMathUtilities.h"
#include "vtkImageData.h"

#include <sstream>

int TestOID(ostream& strm)
{
  int i, j, k;
  // actual test
  strm << "Testing vtkImageData" << endl;
  vtkImageData *id0D = vtkImageData::New();
  vtkImageData *id1Dx = vtkImageData::New();
  vtkImageData *id1Dy = vtkImageData::New();
  vtkImageData *id1Dz = vtkImageData::New();
  vtkImageData *id2Dxy = vtkImageData::New();
  vtkImageData *id2Dxz = vtkImageData::New();
  vtkImageData *id2Dyz = vtkImageData::New();
  vtkImageData *id3D = vtkImageData::New();

  id3D->SetDimensions(20,20,20);

  id2Dxy->SetDimensions(20,20,1);

  id2Dxz->SetDimensions(20,1,20);

  id2Dyz->SetDimensions(1,20,20);

  id1Dx->SetDimensions(20,1,1);

  id1Dy->SetDimensions(1,20,1);

  strm << *id1Dy;

  id1Dz->SetDimensions(1,1,20);

  id0D->SetDimensions(1,1,1);

  strm << "id3D:" << *id3D;

  // Test shallow copy
  vtkImageData *scid3D = vtkImageData::New();
  scid3D->ShallowCopy(id3D);
  strm << "ShallowCopy(id3D):" << *scid3D;

  // Test deep copy
  vtkImageData *dcid3D = vtkImageData::New();
  dcid3D->DeepCopy(id3D);
  strm << "DeepCopy(id3D):" << *dcid3D;

  // Test GetCell
  vtkIdList *ids = vtkIdList::New();
  int cellId;
  int ii;

  i = 10; j = 15; k = 7;
  cellId = k * (19 * 19) + j * 19 + i;
  vtkCell *cell3D = id3D->GetCell(cellId);
  strm << "cell3D: " << *cell3D ;
  id3D->GetCellPoints (cellId, ids);
  strm << "Ids for cell " << cellId << " are ";
  for (ii = 0; ii < ids->GetNumberOfIds(); ii++)
  {
    strm << ids->GetId(ii) << " ";
  }
  strm << endl << endl;

  i = 10; j = 15;
  cellId = j * 19 + i;
  vtkCell *cell2D = id2Dxy->GetCell(cellId);
  strm << "cell2D: " << *cell2D ;
  id2Dxy->GetCellPoints (cellId, ids);
  strm << "Ids for cell " << cellId << " are ";
  for (ii = 0; ii < ids->GetNumberOfIds(); ii++)
  {
    strm << ids->GetId(ii) << " ";
  }
  strm << endl << endl;


  i = 10; j = 15;
  cellId = j * 19 + i;
  cell2D = id2Dxz->GetCell(j * 19 + i);
  strm << "cell2D: " << *cell2D ;
  id2Dxz->GetCellPoints (cellId, ids);
  strm << "Ids for cell " << cellId << " are ";
  for (ii = 0; ii < ids->GetNumberOfIds(); ii++)
  {
    strm << ids->GetId(ii) << " ";
  }
  strm << endl << endl;

  i = 10; j = 15;
  cellId = j * 19 + i;
  cell2D = id2Dyz->GetCell(j * 19 + i);
  strm << "cell2D: " << *cell2D ;
  id2Dyz->GetCellPoints (cellId, ids);
  strm << "Ids for cell " << cellId << " are ";
  for (ii = 0; ii < ids->GetNumberOfIds(); ii++)
  {
    strm << ids->GetId(ii) << " ";
  }
  strm << endl << endl;

  i = 10;
  cellId = i;
  vtkCell *cell1D = id1Dx->GetCell(i);
  strm << "cell1D: " << *cell1D;
  id1Dx->GetCellPoints (cellId, ids);
  strm << "Ids for cell " << cellId << " are ";
  for (ii = 0; ii < ids->GetNumberOfIds(); ii++)
  {
    strm << ids->GetId(ii) << " ";
  }
  strm << endl << endl;

  i = 10;
  cellId = i;
  cell1D = id1Dy->GetCell(i);
  strm << "cell1D: " << *cell1D;
  id1Dy->GetCellPoints (cellId, ids);
  strm << "Ids for cell " << cellId << " are ";
  for (ii = 0; ii < ids->GetNumberOfIds(); ii++)
  {
    strm << ids->GetId(ii) << " ";
  }
  strm << endl << endl;

  i = 10;
  cellId = i;
  cell1D = id1Dz->GetCell(i);
  strm << "cell1D: " << *cell1D;
  id1Dz->GetCellPoints (cellId, ids);
  strm << "Ids for cell " << cellId << " are ";
  for (ii = 0; ii < ids->GetNumberOfIds(); ii++)
  {
    strm << ids->GetId(ii) << " ";
  }
  strm << endl << endl;

  cellId = 0;
  vtkCell *cell0D = id0D->GetCell(0);
  strm << "cell0D: " << *cell0D;
  id0D->GetCellPoints (cellId, ids);
  strm << "Ids for cell " << cellId << " are ";
  for (ii = 0; ii < ids->GetNumberOfIds(); ii++)
  {
    strm << ids->GetId(ii) << " ";
  }
  strm << endl << endl;

  // Test GetCell(i,j,k)
  i = 10; j = 15; k = 7;
  cellId = k * (19 * 19) + j * 19 + i;
  cell3D = id3D->GetCell(i,j,k);
  if (cell3D == NULL)
  {
    std::cerr << "vtkImageData::GetCell returned NULL instead of a valid cell.\n";
    return 1;
  }
  if (cell3D->GetCellType() != VTK_VOXEL)
  {
    std::cerr << "vtkImageData::GetCell returned the wrong cell type.\n"
              << "Expected: " << VTK_VOXEL << " Returned: " << cell3D->GetCellType() << '\n';
    return 1;
  }
  strm << "cell3D: " << *cell3D ;
  id3D->GetCellPoints (cellId, ids);
  strm << "Ids for cell " << cellId << " are ";
  for (ii = 0; ii < ids->GetNumberOfIds(); ii++)
  {
    strm << ids->GetId(ii) << " ";
  }
  strm << endl << endl;

  i = 10; j = 15;
  cellId = j * 19 + i;
  cell2D = id2Dxy->GetCell(i,j,0);
  if (cell2D == NULL)
  {
    std::cerr << "vtkImageData::GetCell returned NULL instead of a valid cell.\n";
    return 1;
  }
  if (cell2D->GetCellType() != VTK_PIXEL)
  {
    std::cerr << "vtkImageData::GetCell returned the wrong cell type.\n"
              << "Expected: " << VTK_PIXEL << " Returned: " << cell2D->GetCellType() << '\n';
    return 1;
  }
  double bounds[6];
  cell2D->GetBounds(bounds);
  if (!vtkMathUtilities::FuzzyCompare(bounds[4],bounds[5]))
  {
    std::cerr << "id2Dxy has finite width along z\n";
    return 1;
  }
  strm << "cell2D: " << *cell2D ;
  id2Dxy->GetCellPoints (cellId, ids);
  strm << "Ids for cell " << cellId << " are ";
  for (ii = 0; ii < ids->GetNumberOfIds(); ii++)
  {
    strm << ids->GetId(ii) << " ";
  }
  strm << endl << endl;

  i = 10; j = 15;
  cellId = j * 19 + i;
  cell2D = id2Dxz->GetCell(i,0,j);
  if (cell2D == NULL)
  {
    std::cerr << "vtkImageData::GetCell returned NULL instead of a valid cell.\n";
    return 1;
  }
  if (cell2D->GetCellType() != VTK_PIXEL)
  {
    std::cerr << "vtkImageData::GetCell returned the wrong cell type.\n"
              << "Expected: " << VTK_PIXEL << " Returned: " << cell2D->GetCellType() << '\n';
    return 1;
  }
  cell2D->GetBounds(bounds);
  if (!vtkMathUtilities::FuzzyCompare(bounds[2],bounds[3]))
  {
    std::cerr << "id2Dxz has finite width along y\n";
    return 1;
  }
  strm << "cell2D: " << *cell2D ;
  id2Dxz->GetCellPoints (cellId, ids);
  strm << "Ids for cell " << cellId << " are ";
  for (ii = 0; ii < ids->GetNumberOfIds(); ii++)
  {
    strm << ids->GetId(ii) << " ";
  }
  strm << endl << endl;

  i = 10; j = 15;
  cellId = j * 19 + i;
  cell2D = id2Dyz->GetCell(0,i,j);
  if (cell2D == NULL)
  {
    std::cerr << "vtkImageData::GetCell returned NULL instead of a valid cell.\n";
    return 1;
  }
  if (cell2D->GetCellType() != VTK_PIXEL)
  {
    std::cerr << "vtkImageData::GetCell returned the wrong cell type.\n"
              << "Expected: " << VTK_PIXEL << " Returned: " << cell2D->GetCellType() << '\n';
    return 1;
  }
  cell2D->GetBounds(bounds);
  if (!vtkMathUtilities::FuzzyCompare(bounds[0],bounds[1]))
  {
    std::cerr << "id2Dyz has finite width along x\n";
    return 1;
  }
  strm << "cell2D: " << *cell2D ;
  id2Dyz->GetCellPoints (cellId, ids);
  strm << "Ids for cell " << cellId << " are ";
  for (ii = 0; ii < ids->GetNumberOfIds(); ii++)
  {
    strm << ids->GetId(ii) << " ";
  }
  strm << endl << endl;

  i = 10;
  cellId = i;
  cell1D = id1Dx->GetCell(i,0,0);
  if (cell1D == NULL)
  {
    std::cerr << "vtkImageData::GetCell returned NULL instead of a valid cell.\n";
    return 1;
  }
  if (cell1D->GetCellType() != VTK_LINE)
  {
    std::cerr << "vtkImageData::GetCell returned the wrong cell type.\n"
              << "Expected: " << VTK_LINE << " Returned: " << cell1D->GetCellType() << '\n';
    return 1;
  }
  cell1D->GetBounds(bounds);
  if (!vtkMathUtilities::FuzzyCompare(bounds[2],bounds[3]))
  {
    std::cerr << "id1Dx has finite width along y\n";
    return 1;
  }
  else if(!vtkMathUtilities::FuzzyCompare(bounds[4],bounds[5]))
  {
    std::cerr << "id1Dx has finite width along z\n";
    return 1;
  }
  strm << "cell1D: " << *cell1D;
  id1Dx->GetCellPoints (cellId, ids);
  strm << "Ids for cell " << cellId << " are ";
  for (ii = 0; ii < ids->GetNumberOfIds(); ii++)
  {
    strm << ids->GetId(ii) << " ";
  }
  strm << endl << endl;

  i = 10;
  cellId = i;
  cell1D = id1Dy->GetCell(0,i,0);
  if (cell1D == NULL)
  {
    std::cerr << "vtkImageData::GetCell returned NULL instead of a valid cell.\n";
    return 1;
  }
  if (cell1D->GetCellType() != VTK_LINE)
  {
    std::cerr << "vtkImageData::GetCell returned the wrong cell type.\n"
              << "Expected: " << VTK_LINE << " Returned: " << cell1D->GetCellType() << '\n';
    return 1;
  }
  cell1D->GetBounds(bounds);
  if (!vtkMathUtilities::FuzzyCompare(bounds[0],bounds[1]))
  {
    std::cerr << "id1Dy has finite width along x\n";
    return 1;
  }
  else if(!vtkMathUtilities::FuzzyCompare(bounds[4],bounds[5]))
  {
    std::cerr << "id1Dy has finite width along z\n";
    return 1;
  }
  strm << "cell1D: " << *cell1D;
  id1Dy->GetCellPoints (cellId, ids);
  strm << "Ids for cell " << cellId << " are ";
  for (ii = 0; ii < ids->GetNumberOfIds(); ii++)
  {
    strm << ids->GetId(ii) << " ";
  }
  strm << endl << endl;

  i = 10;
  cellId = i;
  cell1D = id1Dz->GetCell(0,0,i);
  if (cell1D == NULL)
  {
    std::cerr << "vtkImageData::GetCell returned NULL instead of a valid cell.\n";
    return 1;
  }
  if (cell1D->GetCellType() != VTK_LINE)
  {
    std::cerr << "vtkImageData::GetCell returned the wrong cell type.\n"
              << "Expected: " << VTK_LINE << " Returned: " << cell1D->GetCellType() << '\n';
    return 1;
  }
  cell1D->GetBounds(bounds);
  if (!vtkMathUtilities::FuzzyCompare(bounds[0],bounds[1]))
  {
    std::cerr << "id1Dz has finite width along x\n";
    return 1;
  }
  else if(!vtkMathUtilities::FuzzyCompare(bounds[2],bounds[3]))
  {
    std::cerr << "id1Dz has finite width along y\n";
    return 1;
  }
  strm << "cell1D: " << *cell1D;
  id1Dz->GetCellPoints (cellId, ids);
  strm << "Ids for cell " << cellId << " are ";
  for (ii = 0; ii < ids->GetNumberOfIds(); ii++)
  {
    strm << ids->GetId(ii) << " ";
  }
  strm << endl << endl;

  cellId = 0;
  cell0D = id0D->GetCell(0,0,0);
  if (cell0D == NULL)
  {
    std::cerr << "vtkImageData::GetCell returned NULL instead of a valid cell.\n";
    return 1;
  }
  if (cell0D->GetCellType() != VTK_VERTEX)
  {
    std::cerr << "vtkImageData::GetCell returned the wrong cell type.\n"
              << "Expected: " << VTK_VERTEX << " Returned: " << cell0D->GetCellType() << '\n';
    return 1;
  }
  strm << "cell0D: " << *cell0D;
  id0D->GetCellPoints (cellId, ids);
  strm << "Ids for cell " << cellId << " are ";
  for (ii = 0; ii < ids->GetNumberOfIds(); ii++)
  {
    strm << ids->GetId(ii) << " ";
  }
  strm << endl << endl;

  // Test Thread Safe GetCell
  vtkGenericCell *gcell3D = vtkGenericCell::New();
  vtkGenericCell *gcell2D = vtkGenericCell::New();
  vtkGenericCell *gcell1D = vtkGenericCell::New();
  vtkGenericCell *gcell0D = vtkGenericCell::New();
  i = 10; j = 15; k = 7;
  id3D->GetCell(k * (19 * 19) + j * 19 + i, gcell3D);
  strm << "gcell3D: " << *gcell3D ;

  i = 10; j = 15;
  id2Dxy->GetCell(j * 19 + i,gcell2D);
  strm << "gcell2D: " << *gcell2D ;

  i = 10; j = 15;
  id2Dxz->GetCell(j * 19 + i,gcell2D);
  strm << "gcell2D: " << *gcell2D ;

  i = 10; j = 15;
  id2Dxz->GetCell(j * 19 + i,gcell2D);
  strm << "gcell2D: " << *gcell2D ;

  i = 10; j = 15;
  id2Dyz->GetCell(j * 19 + i,gcell2D);
  strm << "gcell2D: " << *gcell2D ;

  i = 10;
  id1Dx->GetCell(i,gcell1D);
  strm << "gcell1D: " << *gcell1D;

  i = 10;
  id1Dy->GetCell(i,gcell1D);
  strm << "gcell1D: " << *gcell1D;

  i = 10;
  id1Dz->GetCell(i,gcell1D);
  strm << "gcell1D: " << *gcell1D;

  i = 10;
  id0D->GetCell(0,gcell0D);
  strm << "gcell0D: " << *gcell0D;

  // Test GetCellBounds
  id3D->GetCellBounds(k * (19 * 19) + j * 19 + i, bounds);
  strm << "GetCellBounds(id3D): "
       << bounds[0] << ", " << bounds[1] << ", "
       << bounds[2] << ", " << bounds[3] << ", "
       << bounds[4] << ", " << bounds[5] << endl;

  id2Dxy->GetCellBounds(j * 19 + i, bounds);
  strm << "GetCellBounds(id2Dxy): "
       << bounds[0] << ", " << bounds[1] << ", "
       << bounds[2] << ", " << bounds[3] << ", "
       << bounds[4] << ", " << bounds[5] << endl;

  id2Dxz->GetCellBounds(j * 19 + i, bounds);
  strm << "GetCellBounds(id2Dxz): "
       << bounds[0] << ", " << bounds[1] << ", "
       << bounds[2] << ", " << bounds[3] << ", "
       << bounds[4] << ", " << bounds[5] << endl;

  id2Dyz->GetCellBounds(j * 19 + i, bounds);
  strm << "GetCellBounds(id2Dyz): "
       << bounds[0] << ", " << bounds[1] << ", "
       << bounds[2] << ", " << bounds[3] << ", "
       << bounds[4] << ", " << bounds[5] << endl;

  id1Dx->GetCellBounds(i, bounds);
  strm << "GetCellBounds(rg1x): "
       << bounds[0] << ", " << bounds[1] << ", "
       << bounds[2] << ", " << bounds[3] << ", "
       << bounds[4] << ", " << bounds[5] << endl;

  id1Dy->GetCellBounds(i, bounds);
  strm << "GetCellBounds(id1Dy): "
       << bounds[0] << ", " << bounds[1] << ", "
       << bounds[2] << ", " << bounds[3] << ", "
       << bounds[4] << ", " << bounds[5] << endl;

  id1Dz->GetCellBounds(i, bounds);
  strm << "GetCellBounds(id1Dz): "
       << bounds[0] << ", " << bounds[1] << ", "
       << bounds[2] << ", " << bounds[3] << ", "
       << bounds[4] << ", " << bounds[5] << endl;

  id0D->GetCellBounds(i, bounds);
  strm << "GetCellBounds(id0D): "
       << bounds[0] << ", " << bounds[1] << ", "
       << bounds[2] << ", " << bounds[3] << ", "
       << bounds[4] << ", " << bounds[5] << endl;

  // Test GetPoint

  double point[6];
  id3D->GetPoint(k * (20 * 20) + j * 20 + i, point);
  strm << "GetPoint(id3D): "
       << point[0] << ", " << point[1] << ", " << point[2] << endl;

  id2Dxy->GetPoint(j * 20 + i, point);
  strm << "GetPoint(id2Dxy): "
       << point[0] << ", " << point[1] << ", " << point[2] << endl;

  id2Dxz->GetPoint(j * 20 + i, point);
  strm << "GetPoint(id2Dxz): "
       << point[0] << ", " << point[1] << ", " << point[2] << endl;

  id2Dyz->GetPoint(j * 20  + i, point);
  strm << "GetPoint(id2Dyz): "
       << point[0] << ", " << point[1] << ", " << point[2] << endl;

  id1Dx->GetPoint(i, point);
  strm << "GetPoint(rg1x): "
       << point[0] << ", " << point[1] << ", " << point[2] << endl;

  id1Dy->GetPoint(i, point);
  strm << "GetPoint(id1Dy): "
       << point[0] << ", " << point[1] << ", " << point[2] << endl;

  id1Dz->GetPoint(i, point);
  strm << "GetPoint(id1Dz): "
       << point[0] << ", " << point[1] << ", " << point[2] << endl;

  id0D->GetPoint(0, point);
  strm << "GetPoint(id0D): "
       << point[0] << ", " << point[1] << ", " << point[2] << endl;

  // Test FindPoint

  double point3D[3] = {10, 12, 14};

  id3D->GetPoint(id3D->FindPoint(point3D), point);
  strm << "FindPoint("
       << point3D[0] << ", " << point3D[1] << ", " << point3D[2] << ") = "
       << point[0] << ", " << point[1] << ", " << point[2] << endl;

  point3D[0] = 10; point3D[1] = 12; point3D[2] = 0;
  id2Dxy->GetPoint(id2Dxy->FindPoint(point3D), point);
  strm << "FindPoint("
       << point3D[0] << ", " << point3D[1] << ", " << point3D[2] << ") = "
       << point[0] << ", " << point[1] << ", " << point[2] << endl;

  point3D[0] = 10; point3D[1] = 0; point3D[2] = 14;
  id2Dxz->GetPoint(id2Dxz->FindPoint(point3D), point);
  strm << "FindPoint("
       << point3D[0] << ", " << point3D[1] << ", " << point3D[2] << ") = "
       << point[0] << ", " << point[1] << ", " << point[2] << endl;

  point3D[0] = 0; point3D[1] = 12; point3D[2] = 14;
  id2Dyz->GetPoint(id2Dyz->FindPoint(point3D), point);
  strm << "FindPoint("
       << point3D[0] << ", " << point3D[1] << ", " << point3D[2] << ") = "
       << point[0] << ", " << point[1] << ", " << point[2] << endl;

  point3D[0] = 10; point3D[1] = 0; point3D[2] = 0;
  id1Dx->GetPoint(id1Dx->FindPoint(point3D), point);
  strm << "FindPoint("
       << point3D[0] << ", " << point3D[1] << ", " << point3D[2] << ") = "
       << point[0] << ", " << point[1] << ", " << point[2] << endl;

  point3D[0] = 0; point3D[1] = 12; point3D[2] = 0;
  id1Dy->GetPoint(id1Dy->FindPoint(point3D), point);
  strm << "FindPoint("
       << point3D[0] << ", " << point3D[1] << ", " << point3D[2] << ") = "
       << point[0] << ", " << point[1] << ", " << point[2] << endl;

  point3D[0] = 0; point3D[1] = 0; point3D[2] = 14;
  id1Dz->GetPoint(id1Dz->FindPoint(point3D), point);
  strm << "FindPoint("
       << point3D[0] << ", " << point3D[1] << ", " << point3D[2] << ") = "
       << point[0] << ", " << point[1] << ", " << point[2] << endl;

  // Test FindAndGetCell

  double pcoords[3], weights[8];
  int subId;
  vtkCell *dummyCell = NULL;

  point3D[0] = 10.5;
  point3D[1] = 12.1;
  point3D[2] = 14.7;

  strm << "FindAndGetCell(id3D): " << *id3D->FindAndGetCell(point3D, dummyCell, 0, 0, subId, pcoords, weights);
  strm << "pcoords: "
       << pcoords[0] << ", " << pcoords[1] << ", " << pcoords[2] << endl;
  strm << "weights: "
       << weights[0] << ", " << weights[1] << ", "
       << weights[2] << ", " << weights[3] << ", "
       << weights[4] << ", " << weights[5] << ", "
       << weights[6] << ", " << weights[7] << endl;

  point3D[0] = 10.5; point3D[1] = 12.1; point3D[2] = 0;
  pcoords[0] = pcoords[1] = pcoords[2] = 0.0;
  dummyCell = 0;
  vtkCell *found = id2Dxy->FindAndGetCell(point3D, dummyCell, 0, 0, subId, pcoords, weights);
  if (found == NULL)
  {
    strm << "FindAndGetCell(id2Dxy) not found!" << endl;
    return 1;
  }

  strm << "FindAndGetCell(id2Dxy): " << *found;
  strm << "pcoords: "
       << pcoords[0] << ", " << pcoords[1] << endl;
  strm << "weights: "
       << weights[0] << ", " << weights[1] << ", " << weights[2] << ", " << weights[3] << endl;

  point3D[0] = 10.5; point3D[1] = 0.0; point3D[2] = 14.7;
  pcoords[0] = pcoords[1] = pcoords[2] = 0.0;
  weights[0] = weights[1] = weights[2] = weights[3] = 0.0;
  dummyCell = 0;
  found = id2Dxz->FindAndGetCell(point3D, dummyCell, 0, 0, subId, pcoords, weights);
  if (found == NULL)
  {
    strm << "FindAndGetCell(id2Dxz) not found!" << endl;
    return 1;
  }

  strm << "FindAndGetCell(id2Dxz): " << *found;
  strm << "pcoords: "
       << pcoords[0] << ", " << pcoords[1] << endl;
  strm << "weights: "
       << weights[0] << ", " << weights[1] << ", " << weights[2] << ", " << weights[3] << endl;

  point3D[0] = 0.0; point3D[1] = 12.1; point3D[2] = 14.7;
  pcoords[0] = pcoords[1] = pcoords[2] = 0.0;
  weights[0] = weights[1] = weights[2] = weights[3] = 0.0;
  dummyCell = 0;
  found = id2Dyz->FindAndGetCell(point3D, dummyCell, 0, 0, subId, pcoords, weights);
  if (found == NULL)
  {
    strm << "FindAndGetCell(id2Dyz) not found!" << endl;
    return 1;
  }

  strm << "FindAndGetCell(id2Dyz): " << *found;
  strm << "pcoords: "
       << pcoords[0] << ", " << pcoords[1] << endl;
  strm << "weights: "
       << weights[0] << ", " << weights[1] << ", " << weights[2] << ", " << weights[3] << endl;

  point3D[0] = 10.5; point3D[1] = 0.0; point3D[2] = 0.0;
  dummyCell = 0;
  found = id1Dx->FindAndGetCell(point3D, dummyCell, 0, 0, subId, pcoords, weights);
  if (found == NULL)
  {
    strm << "FindAndGetCell(id1Dx) not found!" << endl;
    return 1;
  }

  strm << "FindAndGetCell(id1Dx): " << *found;
  strm << "pcoords: "
       << pcoords[0] << endl;
  strm << "weights: "
       << weights[0] << ", " << weights[1] << endl;


  point3D[0] = 0.0; point3D[1] = 12.1; point3D[2] = 0.0;
  dummyCell = 0;
  found = id1Dy->FindAndGetCell(point3D, dummyCell, 0, 0, subId, pcoords, weights);
  if (found == NULL)
  {
    strm << "FindAndGetCell(id1Dy) not found!" << endl;
    return 1;
  }

  strm << "FindAndGetCell(id1Dy): " << *found;
  strm << "pcoords: "
       << pcoords[0] << endl;
  strm << "weights: "
       << weights[0] << ", " << weights[1] << endl;


  point3D[0] = 0.0; point3D[1] = 0.0; point3D[2] = 14.7;
  dummyCell = 0;
  found = id1Dz->FindAndGetCell(point3D, dummyCell, 0, 0, subId, pcoords, weights);
  if (found == NULL)
  {
    strm << "FindAndGetCell(id1Dz) not found!" << endl;
    return 1;
  }

  strm << "FindAndGetCell(id1Dz): " << *found;
  strm << "pcoords: "
       << pcoords[0] << endl;
  strm << "weights: "
       << weights[0] << ", " << weights[1] << endl;


  // Test GetCellType

  strm << "GetCellType(id3D): " << id3D->GetCellType(0) << endl;
  strm << "GetCellType(id2Dxy): " << id2Dxy->GetCellType(0) << endl;
  strm << "GetCellType(id2Dxz): " << id2Dxz->GetCellType(0) << endl;
  strm << "GetCellType(id2Dyz): " << id2Dyz->GetCellType(0) << endl;
  strm << "GetCellType(id1Dx): " << id1Dx->GetCellType(0) << endl;
  strm << "GetCellType(id1Dy): " << id1Dy->GetCellType(0) << endl;
  strm << "GetCellType(id1Dz): " << id1Dz->GetCellType(0) << endl;
  strm << "GetCellType(id0D): " << id0D->GetCellType(0) << endl;

  // Test GetActualMemorySize

  strm << "GetActualMemorySize(id3D): " << id3D->GetActualMemorySize() << endl;
  strm << "GetActualMemorySize(id2Dxy): " << id2Dxy->GetActualMemorySize() << endl;
  strm << "GetActualMemorySize(id2Dxz): " << id2Dxz->GetActualMemorySize() << endl;
  strm << "GetActualMemorySize(id2Dyz): " << id2Dyz->GetActualMemorySize() << endl;
  strm << "GetActualMemorySize(id1Dx): " << id1Dx->GetActualMemorySize() << endl;
  strm << "GetActualMemorySize(id1Dy): " << id1Dy->GetActualMemorySize() << endl;
  strm << "GetActualMemorySize(id1Dz): " << id1Dz->GetActualMemorySize() << endl;
  strm << "GetActualMemorySize(id0D): " << id0D->GetActualMemorySize() << endl;

  // Cleanup
  id0D->Delete();
  id1Dx->Delete();
  id1Dy->Delete();
  id1Dz->Delete();
  id2Dxy->Delete();
  id2Dxz->Delete();
  id2Dyz->Delete();
  id3D->Delete();
  scid3D->Delete();
  dcid3D->Delete();
  ids->Delete();
  gcell3D->Delete();
  gcell2D->Delete();
  gcell1D->Delete();
  gcell0D->Delete();

  strm << "Testing completed" << endl;
  return 0;
}

int otherImageData(int,char *[])
{
  std::ostringstream vtkmsg_with_warning_C4701;
  return TestOID(vtkmsg_with_warning_C4701);
}
