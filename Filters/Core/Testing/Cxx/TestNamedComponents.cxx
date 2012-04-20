/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestNamedComponents.cxx

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
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkSmartPointer.h"
#include "vtkThreshold.h"
#include "vtkUnstructuredGrid.h"
#include "vtkArrayCalculator.h"

int TestNamedComponents(int , char *[])
{
  int rval = 0;

  vtkIdType numPoints = 20;
  vtkIdType numVerts = 5;
  vtkIdType numLines = 8;
  vtkIdType numTriangles = 3;
  vtkIdType numStrips = 2;
  vtkIdType numCells = numVerts+numLines+numTriangles+numStrips;
  vtkIdType i;


  vtkIdTypeArray* pointCoords = vtkIdTypeArray::New();
  const char pcName[] = "point coords";
  pointCoords->SetName(pcName);
  pointCoords->SetNumberOfComponents(3); // num points + point ids
  pointCoords->SetNumberOfTuples(numPoints);
  pointCoords->SetComponentName(0,"XLOC");
  pointCoords->SetComponentName(1,"YLOC");
  pointCoords->SetComponentName(2,"ZLOC");

  vtkPoints* points = vtkPoints::New();
  points->SetNumberOfPoints(numPoints);
  for(i=0;i<numPoints;i++)
    {
    double loc[3] = {static_cast<double>(i), static_cast<double>(i*i), 0.0};
    points->InsertPoint(i, loc);
    pointCoords->InsertTuple(i,loc);
    }
  vtkSmartPointer<vtkPolyData> poly = vtkSmartPointer<vtkPolyData>::New();
  poly->Allocate(numCells, numCells);
  poly->SetPoints(points);
  poly->GetPointData()->AddArray( pointCoords );
  pointCoords->Delete();
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

  vtkIntArray* cellIndex = vtkIntArray::New();
  const char ctName[] = "scalars";
  cellIndex->SetName(ctName);
  cellIndex->SetNumberOfComponents(1);
  cellIndex->SetNumberOfTuples(numCells);
  cellIndex->SetComponentName(0,"index");
  for(i=0;i<numCells;i++)
    {
    cellIndex->SetValue(i, i);
    }
  poly->GetCellData()->SetScalars( cellIndex );
  cellIndex->Delete();

  vtkIdTypeArray* cellPoints = vtkIdTypeArray::New();
  const char cpName[] = "cell points";
  cellPoints->SetName(cpName);
  cellPoints->SetNumberOfComponents(4); // num points + point ids
  cellPoints->SetNumberOfTuples(numCells);

  cellPoints->SetComponentName(0,"NumberOfPoints");
  cellPoints->SetComponentName(1,"X_ID");
  cellPoints->SetComponentName(2,"Y_ID");
  cellPoints->SetComponentName(3,"Z_ID");

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

  vtkSmartPointer<vtkThreshold> thresh = vtkSmartPointer<vtkThreshold>::New();
  thresh->SetInputData(poly);
  thresh->SetInputArrayToProcess(0, 0, 0,
                                   vtkDataObject::FIELD_ASSOCIATION_CELLS,
                                   vtkDataSetAttributes::SCALARS);

  thresh->ThresholdBetween(0, 10);
  thresh->Update();

  vtkSmartPointer<vtkUnstructuredGrid> out = thresh->GetOutput();

  if ( out == NULL )
    {
    vtkGenericWarningMacro("threshold failed.");
    return 1;
    }

  // the arrays should have been changed so get them again...
  cellIndex = vtkIntArray::SafeDownCast(out->GetCellData()->GetArray(ctName));
  cellPoints = vtkIdTypeArray::SafeDownCast(out->GetCellData()->GetArray(cpName));

  //confirm component names are intact
  if ( strcmp(cellIndex->GetComponentName(0),"index") != 0 )
    {
    vtkGenericWarningMacro("threshold failed to mantain component name on cell scalars.");
    return 1;
    }

  if ( strcmp(cellPoints->GetComponentName(0),"NumberOfPoints")  != 0 ||
       strcmp(cellPoints->GetComponentName(1),"X_ID")  != 0 ||
       strcmp(cellPoints->GetComponentName(2),"Y_ID")  != 0 ||
       strcmp(cellPoints->GetComponentName(3),"Z_ID")  != 0)
    {
    vtkGenericWarningMacro("threshold failed to mantain component names on point property.");
    return 1;
    }

  //Test component names with the calculator
  vtkSmartPointer<vtkArrayCalculator> calc = vtkSmartPointer<vtkArrayCalculator>::New();
  calc->SetInputData( poly );
  calc->SetAttributeModeToUsePointData();
  // Add coordinate scalar and vector variables
  calc->AddCoordinateScalarVariable( "coordsX", 0 );
  calc->AddScalarVariable("point coords_YLOC","point coords",1 );
  calc->SetFunction("coordsX + point coords_YLOC");
  calc->SetResultArrayName("Result");
  calc->Update();


  return rval;
}


