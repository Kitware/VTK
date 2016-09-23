/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTriangularTCoords.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTriangularTCoords.h"

#include "vtkCellArray.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"

#include <cmath>

vtkStandardNewMacro(vtkTriangularTCoords);

int vtkTriangularTCoords::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkPolyData *input = vtkPolyData::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkIdType tmp;
  int j;
  vtkPoints *inPts;
  vtkPointData *pd;
  vtkCellArray *inPolys,*inStrips;
  vtkIdType numNewPts, numNewPolys, polyAllocSize;
  vtkFloatArray *newTCoords;
  vtkIdType newId, numCells, cellId;
  vtkIdType *pts = 0;
  vtkIdType newIds[3];
  vtkIdType npts = 0;;
  int errorLogging = 1;
  vtkPoints *newPoints;
  vtkCellArray *newPolys;
  double p1[3], p2[3], p3[3];
  double tCoords[6];
  vtkPointData *pointData = output->GetPointData();

  // Initialize
  //
  vtkDebugMacro(<<"Generating triangular texture coordinates");

  inPts = input->GetPoints();
  pd = input->GetPointData();

  inPolys = input->GetPolys();
  inStrips = input->GetStrips();

  // Count the number of new points and other primitives that
  // need to be created.
  //
  numNewPts = input->GetNumberOfVerts();

  numNewPolys = 0;
  polyAllocSize = 0;

  for (inPolys->InitTraversal(); inPolys->GetNextCell(npts,pts); )
  {
    numNewPts += npts;
    numNewPolys++;
    polyAllocSize += npts + 1;
  }
  for (inStrips->InitTraversal(); inStrips->GetNextCell(npts,pts); )
  {
    numNewPts += (npts-2) * 3;
    polyAllocSize += (npts - 2) * 4;
  }
  numCells = inPolys->GetNumberOfCells() + inStrips->GetNumberOfCells();

  //  Allocate texture data
  //
  newTCoords = vtkFloatArray::New();
  newTCoords->SetNumberOfComponents(2);
  newTCoords->Allocate(2*numNewPts);

  // Allocate
  //
  newPoints = vtkPoints::New();
  newPoints->Allocate(numNewPts);

  newPolys = vtkCellArray::New();
  newPolys->Allocate(polyAllocSize);

  pointData->CopyTCoordsOff();
  pointData->CopyAllocate(pd);

  // Texture coordinates are the same for each triangle
  //
  tCoords[0]= 0.0;
  tCoords[1]= 0.0;
  tCoords[2]= 1.0;
  tCoords[3]= 0.0;
  tCoords[4]= 0.5;
  tCoords[5]= sqrt(3.0)/2.0;

  int abort=0;
  vtkIdType progressInterval=numCells/20 + 1;
  for (cellId=0, inPolys->InitTraversal();
       inPolys->GetNextCell(npts,pts) && !abort; cellId++)
  {
    if ( !(cellId % progressInterval) )
    {
      this->UpdateProgress((double)cellId/numCells);
      abort = this->GetAbortExecute();
    }

    if (npts != 3)
    {
      if (errorLogging) vtkWarningMacro(
        << "No texture coordinates for this cell, it is not a triangle");
      errorLogging = 0;
      continue;
    }
    newPolys->InsertNextCell(npts);
    for (j=0; j<npts; j++)
    {
      inPts->GetPoint(pts[j], p1);
      newId = newPoints->InsertNextPoint(p1);
      newPolys->InsertCellPoint(newId);
      pointData->CopyData(pd,pts[j],newId);
      newTCoords->InsertNextTuple (&tCoords[2*j]);
    }
  }

  // Triangle strips
  //
  for (inStrips->InitTraversal();
       inStrips->GetNextCell(npts,pts) && !abort; cellId++)
  {
    if ( !(cellId % progressInterval) )
    {
      this->UpdateProgress((double)cellId/numCells);
      abort = this->GetAbortExecute();
    }

    for (j=0; j<(npts-2); j++)
    {
      inPts->GetPoint(pts[j], p1);
      inPts->GetPoint(pts[j+1], p2);
      inPts->GetPoint(pts[j+2], p3);

      newIds[0] = newPoints->InsertNextPoint(p1);
      pointData->CopyData(pd,pts[j],newIds[0]);
      newTCoords->InsertNextTuple (&tCoords[0]);

      newIds[1] = newPoints->InsertNextPoint(p2);
      pointData->CopyData(pd,pts[j+1],newIds[1]);
      newTCoords->InsertNextTuple (&tCoords[2]);

      newIds[2] = newPoints->InsertNextPoint(p3);
      pointData->CopyData(pd,pts[j+2],newIds[2]);
      newTCoords->InsertNextTuple (&tCoords[4]);

      // flip orientation for odd tris
      if (j%2)
      {
        tmp = newIds[0];
        newIds[0] = newIds[2];
        newIds[2] = tmp;
      }
      newPolys->InsertNextCell(3,newIds);
    }
  }

  // Update self and release memory
  //
  output->SetPoints(newPoints);
  newPoints->Delete();

  output->SetPolys(newPolys);
  newPolys->Delete();

  output->GetPointData()->SetTCoords(newTCoords);
  newTCoords->Delete();

  return 1;
}

void vtkTriangularTCoords::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
