/*==========================================================================

  Program: 
  Module:    otherStructuredGrid.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  ==========================================================================*/

// .NAME 
// .SECTION Description
// this program tests vtkCoordinate

#include "vtkStructuredGrid.h"
#include "vtkScalars.h"
#include "vtkFloatArray.h"
#include "vtkDoubleArray.h"
#include "vtkLongArray.h"
#include "vtkShortArray.h"

// All tests need:
//   the following include
//   a Selector proc
//   a Comparator proc
//   a Test proc
//   and a main
#include "rtOtherTestBase.h"

void SelectorCommand(ostream& strm) {
  strm << "sed -e s/0x0/0/ | sed -e s/-0/0/ | grep -v \\(1\\) | grep -v 0x | grep -v Modified | grep -v Array_ | grep -v \"Compute Time:\"";
}

void ComparatorCommand(ostream& strm) {
  strm << "diff";
}

void Test(ostream& strm)
{
  int i, j, k;
  // actual test
  strm << "Testing vtkStructuredGrid" << endl;
  vtkStructuredGrid *sg0D = vtkStructuredGrid::New();
  vtkStructuredGrid *sg1Dx = vtkStructuredGrid::New();
  vtkStructuredGrid *sg1Dy = vtkStructuredGrid::New();
  vtkStructuredGrid *sg1Dz = vtkStructuredGrid::New();
  vtkStructuredGrid *sg2Dxy = vtkStructuredGrid::New();
  vtkStructuredGrid *sg2Dxz = vtkStructuredGrid::New();
  vtkStructuredGrid *sg2Dyz = vtkStructuredGrid::New();
  vtkStructuredGrid *sg3D = vtkStructuredGrid::New();

  vtkPoints *xyzpoints = vtkPoints::New();
  for (k = 0; k < 20; k++)
    {
      for (j = 0; j < 20; j++)
	{
	for (i = 0; i < 20; i++)
	  {
	  xyzpoints->InsertNextPoint((float) i, (float) j, (float) k);
	  }
	}
    }
  sg3D->SetDimensions(20,20,20);
  sg3D->SetPoints(xyzpoints); xyzpoints->Delete();
  
  vtkPoints *xypoints = vtkPoints::New();
  for (j = 0; j < 20; j++)
    {
    for (i = 0; i < 20; i++)
      {
      xypoints->InsertNextPoint((float) i, (float) j, 0.0);
      }
    }
  sg2Dxy->SetDimensions(20,20,1);
  sg2Dxy->SetPoints(xypoints); xypoints->Delete();
  
  vtkPoints *xzpoints = vtkPoints::New();
  for (k = 0; k < 20; k++)
    {
    for (i = 0; i < 20; i++)
      {
      xzpoints->InsertNextPoint((float) i, 0.0, (float) k);
      }
    }
  sg2Dxz->SetDimensions(20,1,20);
  sg2Dxz->SetPoints(xzpoints); xzpoints->Delete();
  
  vtkPoints *yzpoints = vtkPoints::New();
  for (k = 0; k < 20; k++)
    {
    for (j = 0; j < 20; j++)
      {
      yzpoints->InsertNextPoint(0.0, (float) j, (float) k);
      }
    }
  sg2Dyz->SetDimensions(1,20,20);
  sg2Dyz->SetPoints(yzpoints); yzpoints->Delete();
  
  vtkPoints *xpoints = vtkPoints::New();
  for (i = 0; i < 20; i++)
    {
    xpoints->InsertNextPoint((float) i, 0.0, 0.0);
    }
  sg1Dx->SetDimensions(20,1,1);
  sg1Dx->SetPoints(xpoints); xpoints->Delete();
  
  vtkPoints *ypoints = vtkPoints::New();
  for (j = 0; j < 20; j++)
    {
    ypoints->InsertNextPoint(0.0, (float) j, 0.0);
    }
  sg1Dy->SetDimensions(1,20,1);
  sg1Dy->SetPoints(ypoints); ypoints->Delete();
  strm << *sg1Dy;
  
  vtkPoints *zpoints = vtkPoints::New();
  for (k = 0; k < 20; k++)
    {
    zpoints->InsertNextPoint(0.0, 0.0, (float) k);
    }
  sg1Dz->SetDimensions(1,1,20);
  sg1Dz->SetPoints(zpoints); zpoints->Delete();
  
  vtkPoints *onepoints = vtkPoints::New();
  for (k = 0; k < 1; k++)
    {
    onepoints->InsertNextPoint(0.0, 0.0, 0.0);
    }
  sg0D->SetDimensions(1,1,1);
  sg0D->SetPoints(onepoints); onepoints->Delete();
  
  vtkShortArray *shortScalars3D = vtkShortArray::New();
  shortScalars3D->SetNumberOfComponents(3);
  shortScalars3D->SetNumberOfTuples(20*20*20);
  vtkScalars *scalars3D = vtkScalars::New(); 
  scalars3D->SetData(shortScalars3D);
  
  int l = 0;
  for (k = 0; k < 20; k++)
    {
    for (j = 0; j < 20; j++)
      {
      for (i = 0; i < 20; i++)
	{
	shortScalars3D->InsertComponent(l,0,i);
	shortScalars3D->InsertComponent(l,0,j);
	shortScalars3D->InsertComponent(l,0,k);
	l++;
	}
      }
    }
  
  vtkShortArray *shortScalars2D = vtkShortArray::New();
  shortScalars2D->SetNumberOfComponents(2);
  shortScalars2D->SetNumberOfTuples(20*20);
  vtkScalars *scalars2D = vtkScalars::New(); 
  scalars2D->SetData(shortScalars2D);
  
  l = 0;
  for (j = 0; j < 20; j++)
    {
    for (i = 0; i < 20; i++)
      {
      shortScalars2D->InsertComponent(l,0,i);
      shortScalars2D->InsertComponent(l,0,j);
      l++;
      }
    }
  
  vtkShortArray *shortScalars1D = vtkShortArray::New();
  shortScalars1D->SetNumberOfComponents(1);
  shortScalars1D->SetNumberOfTuples(20);
  vtkScalars *scalars1D = vtkScalars::New(); 
  scalars1D->SetData(shortScalars1D);
  
  l = 0;
  for (i = 0; i < 20; i++)
    {
    shortScalars1D->InsertComponent(l,0,i);
    l++;
    }
  
  vtkShortArray *shortScalars0D = vtkShortArray::New();
  shortScalars0D->SetNumberOfComponents(1);
  shortScalars0D->SetNumberOfTuples(1);
  vtkScalars *scalars0D = vtkScalars::New(); 
  scalars0D->SetData(shortScalars0D);

  l = 0;
  shortScalars0D->InsertComponent(l,0,0);
  
  sg3D->GetPointData()->SetScalars(scalars3D);
  sg2Dxy->GetPointData()->SetScalars(scalars2D);
  sg2Dxz->GetPointData()->SetScalars(scalars2D);
  sg2Dyz->GetPointData()->SetScalars(scalars2D);
  sg1Dx->GetPointData()->SetScalars(scalars1D);
  sg1Dy->GetPointData()->SetScalars(scalars1D);
  sg1Dz->GetPointData()->SetScalars(scalars1D);
  sg0D->GetPointData()->SetScalars(scalars0D);
  
  strm << "sg3D:" << *sg3D;
  
  // Test shallow copy
  vtkStructuredGrid *scsg3D = vtkStructuredGrid::New();
  scsg3D->ShallowCopy(sg3D);
  strm << "ShallowCopy(sg3D):" << *scsg3D;
  
  // Test deep copy
  vtkStructuredGrid *dcsg3D = vtkStructuredGrid::New();
  dcsg3D->DeepCopy(sg3D);
  strm << "DeepCopy(sg3D):" << *dcsg3D;
  
  // Test GetCell
  vtkIdList *ids = vtkIdList::New();
  int cellId;
  int ii;
  
  i = 10; j = 15; k = 7;
  cellId = k * (19 * 19) + j * 19 + i;
  vtkCell *cell3D = sg3D->GetCell(cellId);
  strm << "cell3D: " << *cell3D ;
  sg3D->GetCellPoints (cellId, ids);
  strm << "Ids for cell " << cellId << " are ";
  for (ii = 0; ii < ids->GetNumberOfIds(); ii++)
    {
    strm << ids->GetId(ii) << " ";
    }
  strm << endl << endl;

  i = 10; j = 15; k = 7;
  cellId = j * 19 + i;
  vtkCell *cell2D = sg2Dxy->GetCell(cellId);
  strm << "cell2D: " << *cell2D ;
  sg2Dxy->GetCellPoints (cellId, ids);
  strm << "Ids for cell " << cellId << " are ";
  for (ii = 0; ii < ids->GetNumberOfIds(); ii++)
    {
    strm << ids->GetId(ii) << " ";
    }
  strm << endl << endl;


  i = 10; j = 15; k = 7;
  cellId = j * 19 + i;
  cell2D = sg2Dxz->GetCell(j * 19 + i);
  strm << "cell2D: " << *cell2D ;
  sg2Dxz->GetCellPoints (cellId, ids);
  strm << "Ids for cell " << cellId << " are ";
  for (ii = 0; ii < ids->GetNumberOfIds(); ii++)
    {
    strm << ids->GetId(ii) << " ";
    }
  strm << endl << endl;
  
  i = 10; j = 15; k = 7;
  cellId = j * 19 + i;
  cell2D = sg2Dyz->GetCell(j * 19 + i);
  strm << "cell2D: " << *cell2D ;
  sg2Dyz->GetCellPoints (cellId, ids);
  strm << "Ids for cell " << cellId << " are ";
  for (ii = 0; ii < ids->GetNumberOfIds(); ii++)
    {
    strm << ids->GetId(ii) << " ";
    }
  strm << endl << endl;
  
  i = 10;
  cellId = i;
  vtkCell *cell1D = sg1Dx->GetCell(i);
  strm << "cell1D: " << *cell1D;
  sg1Dx->GetCellPoints (cellId, ids);
  strm << "Ids for cell " << cellId << " are ";
  for (ii = 0; ii < ids->GetNumberOfIds(); ii++)
    {
    strm << ids->GetId(ii) << " ";
    }
  strm << endl << endl;
  
  i = 10;
  cellId = i;
  cell1D = sg1Dy->GetCell(i);
  strm << "cell1D: " << *cell1D;
  sg1Dy->GetCellPoints (cellId, ids);
  strm << "Ids for cell " << cellId << " are ";
  for (ii = 0; ii < ids->GetNumberOfIds(); ii++)
    {
    strm << ids->GetId(ii) << " ";
    }
  strm << endl << endl;
  
  i = 10;
  cellId = i;
  cell1D = sg1Dz->GetCell(i);
  strm << "cell1D: " << *cell1D;
  sg1Dz->GetCellPoints (cellId, ids);
  strm << "Ids for cell " << cellId << " are ";
  for (ii = 0; ii < ids->GetNumberOfIds(); ii++)
    {
    strm << ids->GetId(ii) << " ";
    }
  strm << endl << endl;
  
  cellId = 0;
  vtkCell *cell0D = sg0D->GetCell(0);
  strm << "cell0D: " << *cell0D;
  sg0D->GetCellPoints (cellId, ids);
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
  sg3D->GetCell(k * (19 * 19) + j * 19 + i, gcell3D);
  strm << "gcell3D: " << *gcell3D ;

  i = 10; j = 15; k = 7;
  sg2Dxy->GetCell(j * 19 + i,gcell2D);
  strm << "gcell2D: " << *gcell2D ;

  i = 10; j = 15; k = 7;
  sg2Dxz->GetCell(j * 19 + i,gcell2D);
  strm << "gcell2D: " << *gcell2D ;

  i = 10; j = 15; k = 7;
  sg2Dyz->GetCell(j * 19 + i,gcell2D);
  strm << "gcell2D: " << *gcell2D ;

  i = 10;
  sg1Dx->GetCell(i,gcell1D);
  strm << "gcell1D: " << *gcell1D;

  i = 10;
  sg1Dy->GetCell(i,gcell1D);
  strm << "gcell1D: " << *gcell1D;

  i = 10;
  sg1Dz->GetCell(i,gcell1D);
  strm << "gcell1D: " << *gcell1D;

  i = 10;
  sg0D->GetCell(0,gcell0D);
  strm << "gcell0D: " << *gcell0D;

  // Test GetCellBounds
  
  float bounds[6];
  sg3D->GetCellBounds(k * (19 * 19) + j * 19 + i, bounds);
  strm << "GetCellBounds(sg3D): "
       << bounds[0] << ", " << bounds[1] << ", "
       << bounds[2] << ", " << bounds[3] << ", "
       << bounds[4] << ", " << bounds[5] << endl;
    
  sg2Dxy->GetCellBounds(j * 19 + i, bounds);
  strm << "GetCellBounds(sg2Dxy): "
       << bounds[0] << ", " << bounds[1] << ", "
       << bounds[2] << ", " << bounds[3] << ", "
       << bounds[4] << ", " << bounds[5] << endl;
    
  sg2Dxz->GetCellBounds(j * 19 + i, bounds);
  strm << "GetCellBounds(sg2Dxz): "
       << bounds[0] << ", " << bounds[1] << ", "
       << bounds[2] << ", " << bounds[3] << ", "
       << bounds[4] << ", " << bounds[5] << endl;
    
  sg2Dyz->GetCellBounds(j * 19 + i, bounds);
  strm << "GetCellBounds(sg2Dyz): "
       << bounds[0] << ", " << bounds[1] << ", "
       << bounds[2] << ", " << bounds[3] << ", "
       << bounds[4] << ", " << bounds[5] << endl;
    
  sg1Dx->GetCellBounds(i, bounds);
  strm << "GetCellBounds(sg1x): "
       << bounds[0] << ", " << bounds[1] << ", "
       << bounds[2] << ", " << bounds[3] << ", "
       << bounds[4] << ", " << bounds[5] << endl;
    
  sg1Dy->GetCellBounds(i, bounds);
  strm << "GetCellBounds(sg1Dy): "
       << bounds[0] << ", " << bounds[1] << ", "
       << bounds[2] << ", " << bounds[3] << ", "
       << bounds[4] << ", " << bounds[5] << endl;
    
  sg1Dz->GetCellBounds(i, bounds);
  strm << "GetCellBounds(sg1Dz): "
       << bounds[0] << ", " << bounds[1] << ", "
       << bounds[2] << ", " << bounds[3] << ", "
       << bounds[4] << ", " << bounds[5] << endl;
    
  sg0D->GetCellBounds(i, bounds);
  strm << "GetCellBounds(sg0D): "
       << bounds[0] << ", " << bounds[1] << ", "
       << bounds[2] << ", " << bounds[3] << ", "
       << bounds[4] << ", " << bounds[5] << endl;
    
  // Test GetPoint
  
  float point[6];
  sg3D->GetPoint(k * (20 * 20) + j * 20 + i, point);
  strm << "GetPoint(sg3D): "
       << point[0] << ", " << point[1] << ", " << point[2] << endl;
    
  sg2Dxy->GetPoint(j * 20 + i, point);
  strm << "GetPoint(sg2Dxy): "
       << point[0] << ", " << point[1] << ", " << point[2] << endl;
    
  sg2Dxz->GetPoint(j * 20 + i, point);
  strm << "GetPoint(sg2Dxz): "
       << point[0] << ", " << point[1] << ", " << point[2] << endl;
    
  sg2Dyz->GetPoint(j * 20  + i, point);
  strm << "GetPoint(sg2Dyz): "
       << point[0] << ", " << point[1] << ", " << point[2] << endl;
    
  sg1Dx->GetPoint(i, point);
  strm << "GetPoint(sg1x): "
       << point[0] << ", " << point[1] << ", " << point[2] << endl;
    
  sg1Dy->GetPoint(i, point);
  strm << "GetPoint(sg1Dy): "
       << point[0] << ", " << point[1] << ", " << point[2] << endl;
    
  sg1Dz->GetPoint(i, point);
  strm << "GetPoint(sg1Dz): "
       << point[0] << ", " << point[1] << ", " << point[2] << endl;
    
  sg0D->GetPoint(0, point);
  strm << "GetPoint(sg0D): "
       << point[0] << ", " << point[1] << ", " << point[2] << endl;

  // Test FindPoint
  
  float point3D[3] = {10, 12, 14};

  sg3D->GetPoint(sg3D->FindPoint(point3D), point);
  strm << "FindPoint("
       << point3D[0] << ", " << point3D[1] << ", " << point3D[2] << ") = "
       << point[0] << ", " << point[1] << ", " << point[2] << endl;

  point3D[0] = 10; point3D[1] = 12; point3D[2] = 0;
  sg2Dxy->GetPoint(sg2Dxy->FindPoint(point3D), point);
  strm << "FindPoint("
       << point3D[0] << ", " << point3D[1] << ", " << point3D[2] << ") = "
       << point[0] << ", " << point[1] << ", " << point[2] << endl;

  point3D[0] = 10; point3D[1] = 0; point3D[2] = 14;
  sg2Dxz->GetPoint(sg2Dxz->FindPoint(point3D), point);
  strm << "FindPoint("
       << point3D[0] << ", " << point3D[1] << ", " << point3D[2] << ") = "
       << point[0] << ", " << point[1] << ", " << point[2] << endl;

  point3D[0] = 0; point3D[1] = 12; point3D[2] = 14;
  sg2Dyz->GetPoint(sg2Dyz->FindPoint(point3D), point);
  strm << "FindPoint("
       << point3D[0] << ", " << point3D[1] << ", " << point3D[2] << ") = "
       << point[0] << ", " << point[1] << ", " << point[2] << endl;

  point3D[0] = 10; point3D[1] = 0; point3D[2] = 0;
  sg1Dx->GetPoint(sg1Dx->FindPoint(point3D), point);
  strm << "FindPoint("
       << point3D[0] << ", " << point3D[1] << ", " << point3D[2] << ") = "
       << point[0] << ", " << point[1] << ", " << point[2] << endl;

  point3D[0] = 0; point3D[1] = 12; point3D[2] = 0;
  sg1Dy->GetPoint(sg1Dy->FindPoint(point3D), point);
  strm << "FindPoint("
       << point3D[0] << ", " << point3D[1] << ", " << point3D[2] << ") = "
       << point[0] << ", " << point[1] << ", " << point[2] << endl;

  point3D[0] = 0; point3D[1] = 0; point3D[2] = 14;
  sg1Dz->GetPoint(sg1Dz->FindPoint(point3D), point);
  strm << "FindPoint("
       << point3D[0] << ", " << point3D[1] << ", " << point3D[2] << ") = "
       << point[0] << ", " << point[1] << ", " << point[2] << endl;

  // Test FindAndGetCell
  
  float pcoords[3], weights[8];
  int subId;
  vtkCell *dummyCell = 0;
  
  point3D[0] = 10.5;
  point3D[1] = 12.1;
  point3D[2] = 14.7;

  strm << "FindAndGetCell(sg3D): " << *sg3D->FindAndGetCell(point3D, dummyCell, 0.0, 0, subId, pcoords, weights);
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
  vtkCell *found = sg2Dxy->FindAndGetCell(point3D, dummyCell, 0.0, 0, subId, pcoords, weights);
  if (found == NULL)
    {
    strm << "FindAndGetCell(sg2Dxy) not found!" << endl;
    return;
    }
  
  strm << "FindAndGetCell(sg2Dxy): " << *found;
  strm << "pcoords: "
       << pcoords[0] << ", " << pcoords[1] << endl;
  strm << "weights: "
       << weights[0] << ", " << weights[1] << ", " << weights[2] << ", " << weights[3] << endl;

  point3D[0] = 10.5; point3D[1] = 0.0; point3D[2] = 14.7;
  pcoords[0] = pcoords[1] = pcoords[2] = 0.0;
  weights[0] = weights[1] = weights[2] = weights[3] = 0.0;
  dummyCell = 0;
  found = sg2Dxz->FindAndGetCell(point3D, dummyCell, 0.0, 0, subId, pcoords, weights);
  if (found == NULL)
    {
    strm << "FindAndGetCell(sg2Dxz) not found!" << endl;
    return;
    }
  
  strm << "FindAndGetCell(sg2Dxz): " << *found;
  strm << "pcoords: "
       << pcoords[0] << ", " << pcoords[1] << endl;
  strm << "weights: "
       << weights[0] << ", " << weights[1] << ", " << weights[2] << ", " << weights[3] << endl;

  point3D[0] = 0.0; point3D[1] = 12.1; point3D[2] = 14.7;
  pcoords[0] = pcoords[1] = pcoords[2] = 0.0;
  weights[0] = weights[1] = weights[2] = weights[3] = 0.0;
  dummyCell = 0;
  found = sg2Dyz->FindAndGetCell(point3D, dummyCell, 0.0, 0, subId, pcoords, weights);
  if (found == NULL)
    {
    strm << "FindAndGetCell(sg2Dyz) not found!" << endl;
    return;
    }
  
  strm << "FindAndGetCell(sg2Dyz): " << *found;
  strm << "pcoords: "
       << pcoords[0] << ", " << pcoords[1] << endl;
  strm << "weights: "
       << weights[0] << ", " << weights[1] << ", " << weights[2] << ", " << weights[3] << endl;

  point3D[0] = 10.5; point3D[1] = 0.0; point3D[2] = 0.0;
  dummyCell = 0;
  found = sg1Dx->FindAndGetCell(point3D, dummyCell, 0.0, 0, subId, pcoords, weights);
  if (found == NULL)
    {
    strm << "FindAndGetCell(sg1Dx) not found!" << endl;
    return;
    }
  
  strm << "FindAndGetCell(sg1Dx): " << *found;
  strm << "pcoords: "
       << pcoords[0] << endl;
  strm << "weights: "
       << weights[0] << ", " << weights[1] << endl;


  point3D[0] = 0.0; point3D[1] = 12.1; point3D[2] = 0.0;
  dummyCell = 0;
  found = sg1Dy->FindAndGetCell(point3D, dummyCell, 0.0, 0, subId, pcoords, weights);
  if (found == NULL)
    {
    strm << "FindAndGetCell(sg1Dy) not found!" << endl;
    return;
    }
  
  strm << "FindAndGetCell(sg1Dy): " << *found;
  strm << "pcoords: "
       << pcoords[0] << endl;
  strm << "weights: "
       << weights[0] << ", " << weights[1] << endl;


  point3D[0] = 0.0; point3D[1] = 0.0; point3D[2] = 14.7;
  dummyCell = 0;
  found = sg1Dz->FindAndGetCell(point3D, dummyCell, 0.0, 0, subId, pcoords, weights);
  if (found == NULL)
    {
    strm << "FindAndGetCell(sg1Dz) not found!" << endl;
    return;
    }
  
  strm << "FindAndGetCell(sg1Dz): " << *found;
  strm << "pcoords: "
       << pcoords[0] << endl;
  strm << "weights: "
       << weights[0] << ", " << weights[1] << endl;


  // Test GetCellType

  strm << "GetCellType(sg3D): " << sg3D->GetCellType(0) << endl;
  strm << "GetCellType(sg2Dxy): " << sg2Dxy->GetCellType(0) << endl;
  strm << "GetCellType(sg2Dxz): " << sg2Dxz->GetCellType(0) << endl;
  strm << "GetCellType(sg2Dyz): " << sg2Dyz->GetCellType(0) << endl;
  strm << "GetCellType(sg1Dx): " << sg1Dx->GetCellType(0) << endl;
  strm << "GetCellType(sg1Dy): " << sg1Dy->GetCellType(0) << endl;
  strm << "GetCellType(sg1Dz): " << sg1Dz->GetCellType(0) << endl;
  strm << "GetCellType(sg0D): " << sg0D->GetCellType(0) << endl;
  
  // Test GetActualMemorySize

  strm << "GetActualMemorySize(sg3D): " << sg3D->GetActualMemorySize() << endl;
  strm << "GetActualMemorySize(sg2Dxy): " << sg2Dxy->GetActualMemorySize() << endl;
  strm << "GetActualMemorySize(sg2Dxz): " << sg2Dxz->GetActualMemorySize() << endl;
  strm << "GetActualMemorySize(sg2Dyz): " << sg2Dyz->GetActualMemorySize() << endl;
  strm << "GetActualMemorySize(sg1Dx): " << sg1Dx->GetActualMemorySize() << endl;
  strm << "GetActualMemorySize(sg1Dy): " << sg1Dy->GetActualMemorySize() << endl;
  strm << "GetActualMemorySize(sg1Dz): " << sg1Dz->GetActualMemorySize() << endl;
  strm << "GetActualMemorySize(sg0D): " << sg0D->GetActualMemorySize() << endl;
  
  
  strm << "Testing completed" << endl;
  
}

int main(int argc, char* argv[])
{
  rtOtherTestBase::RunTest(argc, argv, SelectorCommand, ComparatorCommand, Test);
  return 0;  
}

