/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPolyDataRemoveCell.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCellData.h"
#include "vtkIdTypeArray.h"
#include "vtkIntArray.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkSmartPointer.h"


int TestPolyDataRemoveCell(int , char *[])
{
  int rval = 0;

  vtkIdType numPoints = 20;
  vtkIdType numVerts = 5;
  vtkIdType numLines = 8;
  vtkIdType numTriangles = 3;
  vtkIdType numStrips = 2;
  vtkIdType numCells = numVerts+numLines+numTriangles+numStrips;
  vtkIdType i;

  vtkPoints* points = vtkPoints::New();
  points->SetNumberOfPoints(numPoints);
  for(i=0;i<numPoints;i++)
    {
    double loc[3] = {i, i*i, 0};
    points->InsertPoint(i, loc);
    }
  vtkSmartPointer<vtkPolyData> poly = vtkSmartPointer<vtkPolyData>::New();
  poly->Allocate(numCells, numCells);
  poly->SetPoints(points);
  points->Delete();

  for(i=0;i<numVerts;i++)
    {
    poly->InsertNextCell(VTK_VERTEX, 1, &i);
    }

  for(i=0;i<numLines;i++)
    {
    vtkIdType pts[2] = {i, i+1};
    poly->InsertNextCell(VTK_LINE, 2, pts);
    }

  for(i=0;i<numTriangles;i++)
    {
    vtkIdType pts[3] = {0, i+1, i+2};
    poly->InsertNextCell(VTK_TRIANGLE, 3, pts);
    }

  for(i=0;i<numStrips;i++)
    {
    vtkIdType pts[3] = {0, i+1, i+2};
    poly->InsertNextCell(VTK_TRIANGLE_STRIP, 3, pts);
    }

  vtkIntArray* cellTypes = vtkIntArray::New();
  const char ctName[] = "cell types";
  cellTypes->SetName(ctName);
  cellTypes->SetNumberOfComponents(1);
  cellTypes->SetNumberOfTuples(numCells);
  for(i=0;i<numCells;i++)
    {
    cellTypes->SetValue(i, poly->GetCellType(i));
    }
  poly->GetCellData()->AddArray(cellTypes);
  cellTypes->Delete();

  vtkIdTypeArray* cellPoints = vtkIdTypeArray::New();
  const char cpName[] = "cell points";
  cellPoints->SetName(cpName);
  cellPoints->SetNumberOfComponents(4); // num points + point ids
  cellPoints->SetNumberOfTuples(numCells);
  for(i=0;i<numCells;i++)
    {
    vtkIdType npts, *pts;
    poly->GetCellPoints(i, npts, pts);
    vtkIdType data[4] = {npts, pts[0], 0, 0};
    for(vtkIdType j=1;j<npts;j++)
      {
      data[j+1] = pts[j];
      }
    cellPoints->SetTupleValue(i, data);
    }
  poly->GetCellData()->AddArray(cellPoints);
  cellPoints->Delete();

  poly->BuildCells();
  // now that we're all set up, try deleting one of each object
  poly->DeleteCell(numVerts-1); // vertex
  poly->DeleteCell(numVerts+numLines-1); // line
  poly->DeleteCell(numVerts+numLines+numTriangles-1); // triangle
  poly->DeleteCell(numCells-1); // strip

  poly->RemoveDeletedCells();
  
  if(poly->GetNumberOfCells() != numCells-4)
    {
    cout << "Wrong number of cells after removal.\n";
    return 1;
    }

  // the arrays should have been changed so get them again...
  cellTypes = vtkIntArray::SafeDownCast(poly->GetCellData()->GetArray(ctName));
  cellPoints = vtkIdTypeArray::SafeDownCast(poly->GetCellData()->GetArray(cpName));

  // check the cell types and arrays
  for(i=0;i<poly->GetNumberOfCells();i++)
    {
    if(cellTypes->GetValue(i) != poly->GetCellType(i))
      {
      cout << "Problem with cell type for cell " << i << endl;
      return 1;
      }
    }

  // check the cell's points
  for(i=0;i<poly->GetNumberOfCells();i++)
    {
    vtkIdType npts, *pts;
    poly->GetCellPoints(i, npts, pts);
    vtkIdType data[4];
    cellPoints->GetTupleValue(i, data);
    if(data[0] != npts)
      {
      cout << "Problem with the number of points for cell " << i << endl;
      return 1;
      }
    for(vtkIdType j=0;j<npts;j++)
      {
      if(pts[j] != data[j+1])
        {
        cout << "Problem with  point " << j << " for cell " << i << endl;
        return 1;        
        }
      }
    }

  return rval;
}


