/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkShrinkPolyData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkShrinkPolyData.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"

vtkStandardNewMacro(vtkShrinkPolyData);

vtkShrinkPolyData::vtkShrinkPolyData(double sf)
{
  sf = ( sf < 0.0 ? 0.0 : (sf > 1.0 ? 1.0 : sf));
  this->ShrinkFactor = sf;
}

template <class T>
void vtkShrinkPolyDataExecute(vtkShrinkPolyData *self, T *inPts,
                              double shrinkFactor,
                              vtkInformation *inInfo, vtkInformation *outInfo)
{
  int j, k;
  T center[3];
  int   abortExecute=0;
  vtkCellArray *newVerts, *newLines, *newPolys;
  vtkPointData *pd;
  vtkCellArray *inVerts,*inLines,*inPolys,*inStrips;
  vtkIdType numNewPts, numNewLines, numNewPolys, polyAllocSize;
  vtkIdType npts = 0;
  vtkIdType *pts = 0;
  vtkIdType newIds[3] = {0, 0, 0};
  vtkPoints *newPoints;
  T *p1, *p2, *p3;
  vtkPolyData *input = vtkPolyData::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *output= vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPointData *pointData = output->GetPointData(); 

  pd = input->GetPointData();

  inVerts = input->GetVerts();
  inLines = input->GetLines();
  inPolys = input->GetPolys();
  inStrips = input->GetStrips();

  // Count the number of new points and other primitives that 
  // need to be created.
  //
  numNewPts = input->GetNumberOfVerts();
  numNewLines = 0;
  numNewPolys = 0;
  polyAllocSize = 0;

  for (inLines->InitTraversal(); inLines->GetNextCell(npts,pts); )
    {
    numNewPts += (npts-1) * 2;
    numNewLines += npts - 1;
    }
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

  // Allocate
  //
  newVerts = vtkCellArray::New();
  newVerts->Allocate(input->GetNumberOfVerts());

  newLines = vtkCellArray::New();
  newLines->Allocate(numNewLines*3);
 
  newPolys = vtkCellArray::New();
  newPolys->Allocate(polyAllocSize);

  pointData->CopyAllocate(pd);

  newPoints = input->GetPoints()->NewInstance();
  newPoints->SetDataType(input->GetPoints()->GetDataType());
  newPoints->Allocate(numNewPts);
  newPoints->SetNumberOfPoints(numNewPts);
  T *outPts = (T *)newPoints->GetVoidPointer(0);
  vtkIdType outCount = 0;
  
  // Copy vertices (no shrinking necessary)
  //
  for (inVerts->InitTraversal(); 
       inVerts->GetNextCell(npts,pts) && !abortExecute; )
    {
    newVerts->InsertNextCell(npts);
    for (j=0; j<npts; j++)
      {
      outPts[0] = inPts[pts[j]*3];
      outPts[1] = inPts[pts[j]*3+1];
      outPts[2] = inPts[pts[j]*3+2];
      outPts += 3;
      newVerts->InsertCellPoint(outCount);
      pointData->CopyData(pd,pts[j],outCount);
      outCount++;
      }    
    abortExecute = self->GetAbortExecute();
    }
  self->UpdateProgress (0.10);
  
  // Lines need to be shrunk, and if polyline, split into separate pieces
  //
  for (inLines->InitTraversal(); 
       inLines->GetNextCell(npts,pts) && !abortExecute; )
    {
    for (j=0; j<(npts-1); j++)
      {
      p1 = inPts + pts[j]*3;
      p2 = inPts + pts[j+1]*3;
      for (k=0; k<3; k++)
        {
        center[k] = (p1[k] + p2[k]) / 2;
        }

      for (k=0; k<3; k++)
        {
        outPts[k] = (T)(center[k] + shrinkFactor*(p1[k] - center[k]));
        }
      outPts += 3;
      pointData->CopyData(pd,pts[j],outCount);
      outCount++;

      for (k=0; k<3; k++)
        {
        outPts[k] = (T)(center[k] + shrinkFactor*(p2[k] - center[k]));
        }
      outPts += 3;
      pointData->CopyData(pd,pts[j+1],outCount);
      newIds[0] = outCount - 1;
      newIds[1] = outCount;
      newLines->InsertNextCell(2,newIds);
      outCount++;
      }
    abortExecute = self->GetAbortExecute();
    }
  self->UpdateProgress (0.25);

  // Polygons need to be shrunk
  //
  for (inPolys->InitTraversal(); 
       inPolys->GetNextCell(npts,pts) && !abortExecute; )
    {
    for (center[0]=center[1]=center[2]=0, j=0; j<npts; j++)
      {
      p1 = inPts + pts[j]*3;
      for (k=0; k<3; k++)
        {
        center[k] += p1[k];
        }
      }

    for (k=0; k<3; k++)
      {
      center[k] /= npts;
      }

    newPolys->InsertNextCell(npts);
    for (j=0; j<npts; j++)
      {
      p1 = inPts + pts[j]*3;
      for (k=0; k<3; k++)
        {
        outPts[k] = (T)(center[k] + shrinkFactor*(p1[k] - center[k]));
        }
      outPts += 3;
      newPolys->InsertCellPoint(outCount);
      pointData->CopyData(pd,pts[j],outCount);
      outCount++;
      }
    abortExecute = self->GetAbortExecute();
    }
  self->UpdateProgress (0.75);

  // Triangle strips need to be shrunk and split into separate pieces.
  //
  vtkIdType tmp;
  for (inStrips->InitTraversal(); 
       inStrips->GetNextCell(npts,pts) && !abortExecute; )
    {
    for (j=0; j<(npts-2); j++)
      {
      p1 = inPts + pts[j]*3;
      p2 = inPts + pts[j+1]*3;
      p3 = inPts + pts[j+2]*3;
      for (k=0; k<3; k++)
        {
        center[k] = (p1[k] + p2[k] + p3[k]) / 3;
        }

      for (k=0; k<3; k++)
        {
        outPts[k] = (T)(center[k] + shrinkFactor*(p1[k] - center[k]));
        }
      outPts += 3;
      pointData->CopyData(pd,pts[j],outCount);
      newIds[0] = outCount;
      outCount++;

      for (k=0; k<3; k++)
        {
        outPts[k] = (T)(center[k] + shrinkFactor*(p2[k] - center[k]));
        }
      outPts += 3;
      pointData->CopyData(pd,pts[j+1],outCount);
      newIds[1] = outCount;
      outCount++;

      for (k=0; k<3; k++)
        {
        outPts[k] = (T)(center[k] + shrinkFactor*(p3[k] - center[k]));
        }
      outPts += 3;
      pointData->CopyData(pd,pts[j+2],outCount);
      newIds[1] = outCount;
      outCount++;

      // must reverse order for every other triangle
      if (j%2)
        {
        tmp = newIds[0];
        newIds[0] = newIds[2];
        newIds[2] = tmp;
        }
      newPolys->InsertNextCell(3,newIds);
      }
    abortExecute = self->GetAbortExecute();
    }

  // Update self and release memory
  //
  output->SetPoints(newPoints);
  newPoints->Delete();

  output->SetVerts(newVerts);
  newVerts->Delete();

  output->SetLines(newLines);
  newLines->Delete();
 
  output->SetPolys(newPolys);
  newPolys->Delete();

  output->GetCellData()->PassData(input->GetCellData());
}

int vtkShrinkPolyData::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input
  vtkPolyData *input = vtkPolyData::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Initialize
  vtkDebugMacro(<<"Shrinking polygonal data");

  if (input == NULL || input->GetPoints() == NULL)
    {
    return 1;
    }
  
  // get the input pointer for templating
  void *inPtr = input->GetPoints()->GetVoidPointer(0);

  // call templated function
  switch (input->GetPoints()->GetDataType())
    {
    vtkTemplateMacro(
      vtkShrinkPolyDataExecute(this, 
                               (VTK_TT *)(inPtr), this->ShrinkFactor,
                               inInfo, outInfo));
    default:
      break;
    }

  return 1;
}

void vtkShrinkPolyData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Shrink Factor: " << this->ShrinkFactor << "\n";
}
