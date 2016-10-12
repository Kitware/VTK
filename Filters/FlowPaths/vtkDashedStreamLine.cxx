/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDashedStreamLine.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDashedStreamLine.h"

#ifndef VTK_LEGACY_REMOVE

#include "vtkCellArray.h"
#include "vtkDataSet.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"

vtkStandardNewMacro(vtkDashedStreamLine);

vtkDashedStreamLine::vtkDashedStreamLine()
{
  this->DashFactor = 0.75;

  VTK_LEGACY_BODY(vtkDashedStreamLine::vtkDashedStreamLine, "VTK 6.3");
}

int vtkDashedStreamLine::RequestData(vtkInformation *,
                                     vtkInformationVector **inputVector,
                                     vtkInformationVector *outputVector)
{
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkInformation *sourceInfo = inputVector[1]->GetInformationObject(0);

  vtkStreamer::StreamPoint *sPrev, *sPtr;
  vtkPoints *newPts;
  vtkFloatArray *newVectors;
  vtkFloatArray *newScalars=NULL;
  vtkCellArray *newLines;
  int i, ptId, j;
  vtkIdType pts[2];
  double tOffset, x[3], v[3], r, xPrev[3], vPrev[3], scalarPrev;
  double s = 0;
  double xEnd[3], vEnd[3], sEnd;
  vtkDataSet *input = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet *source = 0;
  if (sourceInfo)
  {
    source = vtkDataSet::SafeDownCast(
      sourceInfo->Get(vtkDataObject::DATA_OBJECT()));
  }

  this->SavePointInterval = this->StepLength;
  this->vtkStreamer::Integrate(input, source);
  if ( this->NumberOfStreamers <= 0 )
  {
    return 1;
  }
  //
  //  Convert streamer into lines. Lines may be dashed.
  //
  newPts = vtkPoints::New();
  newPts->Allocate(1000);
  newVectors = vtkFloatArray::New();
  newVectors->SetNumberOfComponents(3);
  newVectors->Allocate(1000);
  if ( input->GetPointData()->GetScalars() || this->SpeedScalars )
  {
    newScalars = vtkFloatArray::New();
    newScalars->Allocate(1000);
  }
  newLines = vtkCellArray::New();
  newLines->Allocate(newLines->EstimateSize(2*this->NumberOfStreamers,VTK_CELL_SIZE));
  //
  // Loop over all streamers generating points
  //
  for (ptId=0; ptId < this->NumberOfStreamers; ptId++)
  {
    if ( this->Streamers[ptId].GetNumberOfPoints() < 2 )
    {
      continue;
    }
    sPrev = this->Streamers[ptId].GetStreamPoint(0);
    sPtr = this->Streamers[ptId].GetStreamPoint(1);
    for (j=0; j<3; j++)
    {
      xPrev[j] = sPrev->x[j];
      vPrev[j] = sPrev->v[j];
    }
    scalarPrev = sPrev->s;

    if ( this->Streamers[ptId].GetNumberOfPoints() == 2 && sPtr->cellId < 0 )
    {
      continue;
    }

    tOffset = sPrev->t;

    for ( i=1;
    i < this->Streamers[ptId].GetNumberOfPoints() && sPtr->cellId >= 0;
    i++, sPrev=sPtr, sPtr=this->Streamers[ptId].GetStreamPoint(i) )
    {
//
// Search for end of dash...create end of one dash, beginning of next
//
      while ( tOffset >= sPrev->t && tOffset < sPtr->t )
      {
        r = (tOffset - sPrev->t) / (sPtr->t - sPrev->t);

        for (j=0; j<3; j++)
        {
          x[j] = sPrev->x[j] + r * (sPtr->x[j] - sPrev->x[j]);
          v[j] = sPrev->v[j] + r * (sPtr->v[j] - sPrev->v[j]);
          xEnd[j] = xPrev[j] + this->DashFactor * (x[j] - xPrev[j]);
          vEnd[j] = vPrev[j] + this->DashFactor * (v[j] - vPrev[j]);
        }

        // create this dash
        pts[0] = newPts->InsertNextPoint(x);
        newVectors->InsertTuple(pts[0],v);

        pts[1] = newPts->InsertNextPoint(xEnd);
        newVectors->InsertTuple(pts[1],vEnd);

        if ( newScalars )
        {
          s = sPrev->s + r * (sPtr->s - sPrev->s);
          newScalars->InsertTuple(pts[0],&s);
          sEnd = scalarPrev + this->DashFactor * (s - scalarPrev);
          newScalars->InsertTuple(pts[1],&sEnd);
        }

        newLines->InsertNextCell(2,pts);

        for (j=0; j<3; j++)
        {
          xPrev[j] = x[j];
          vPrev[j] = v[j];
        }
        if ( newScalars )
        {
          scalarPrev = s;
        }
        tOffset += this->StepLength;

      } // while
    } //for this streamer
  } //for all streamers
//
// Update ourselves and release memory
//
  vtkDebugMacro(<<"Created " << newPts->GetNumberOfPoints() << " points, "
               << newLines->GetNumberOfCells() << " lines");

  output->SetPoints(newPts);
  newPts->Delete();

  output->GetPointData()->SetVectors(newVectors);
  newVectors->Delete();

  if ( newScalars )
  {
    int idx = output->GetPointData()->AddArray(newScalars);
    output->GetPointData()->SetActiveAttribute(idx,
                                               vtkDataSetAttributes::SCALARS);
    newScalars->Delete();
  }

  output->SetLines(newLines);
  newLines->Delete();

  // Delete the streamers since they are no longer needed
  delete[] this->Streamers;
  this->Streamers = 0;
  this->NumberOfStreamers = 0;

  output->Squeeze();

  return 1;
}

void vtkDashedStreamLine::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Dash Factor: " << this->DashFactor << " <<\n";

}

#endif // VTK_LEGACY_REMOVE
