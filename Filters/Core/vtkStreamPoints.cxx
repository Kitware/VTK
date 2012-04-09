/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStreamPoints.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkStreamPoints.h"

#include "vtkCellArray.h"
#include "vtkDataSet.h"
#include "vtkFloatArray.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"

vtkStandardNewMacro(vtkStreamPoints);

// Construct object with time increment set to 1.0.
vtkStreamPoints::vtkStreamPoints()
{
  this->TimeIncrement = 1.0;
  this->NumberOfStreamers = 0;
}

int vtkStreamPoints::RequestData(
  vtkInformation *,
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
  vtkCellArray *newVerts;
  vtkIdType i, ptId, id;
  int j;
  double tOffset, x[3], v[3], s, r;
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet *input = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet *source = 0;
  if (sourceInfo)
    {
    source = vtkDataSet::SafeDownCast(
      sourceInfo->Get(vtkDataObject::DATA_OBJECT()));
    }
  vtkIdList *pts;

  this->SavePointInterval = this->TimeIncrement;  
  this->vtkStreamer::Integrate(input, source);
  if ( this->NumberOfStreamers <= 0 )
    {
    return 1;
    }

  pts = vtkIdList::New();
  pts->Allocate(2500);
  newPts  = vtkPoints::New();
  newPts ->Allocate(1000);
  newVectors  = vtkFloatArray::New();
  newVectors->SetNumberOfComponents(3);
  newVectors ->Allocate(3000);
  if ( input->GetPointData()->GetScalars() || this->SpeedScalars 
    || this->OrientationScalars)
    {
    newScalars = vtkFloatArray::New();
    newScalars->Allocate(1000);
    }
  newVerts = vtkCellArray::New();
  newVerts->Allocate(newVerts->EstimateSize(2*this->NumberOfStreamers,VTK_CELL_SIZE));
  
  //
  // Loop over all streamers generating points
  //
  for (ptId=0; ptId < this->NumberOfStreamers; ptId++)
    {
    // tOffset is the time that the next point will have.
    tOffset = 0.0;

    for ( sPrev=sPtr=this->Streamers[ptId].GetStreamPoint(0), i=0; 
    i < this->Streamers[ptId].GetNumberOfPoints() && sPtr->cellId >= 0;
    i++, sPrev=sPtr, sPtr=this->Streamers[ptId].GetStreamPoint(i) )
      {
      //
      // For each streamer, create points "time increment" apart
      //
      if ( tOffset < sPtr->t )
        {
        while ( tOffset < sPtr->t )
          {
          r = (tOffset - sPrev->t) / (sPtr->t - sPrev->t);

          for (j=0; j<3; j++)
            {
            x[j] = sPrev->x[j] + r * (sPtr->x[j] - sPrev->x[j]);
            v[j] = sPrev->v[j] + r * (sPtr->v[j] - sPrev->v[j]);
            }

          // add point to line
          id = newPts->InsertNextPoint(x);
          pts->InsertNextId(id);
          newVectors->InsertTuple(id,v);

          if ( newScalars ) 
            {
            s = sPrev->s + r * (sPtr->s - sPrev->s);
            newScalars->InsertTuple(id,&s);
            }
  
          tOffset += this->TimeIncrement;
          } // while

        } //if points should be created

      } //for this streamer
    if ( pts->GetNumberOfIds() > 1 )
      {
      newVerts->InsertNextCell(pts);
      pts->Reset();
      }
    } //for all streamers
  //
  // Update ourselves
  //
  vtkDebugMacro(<<"Created " << newPts->GetNumberOfPoints() << " points");

  output->SetPoints(newPts);
  newPts->Delete();
  output->SetVerts(newVerts);
  newVerts->Delete();

  output->GetPointData()->SetVectors(newVectors);
  newVectors->Delete();

  if ( newScalars ) 
    {
    int idx = output->GetPointData()->AddArray(newScalars);
    output->GetPointData()->SetActiveAttribute(idx, vtkDataSetAttributes::SCALARS);
    newScalars->Delete();
    }

  // Delete the streamers since they are no longer needed
  delete[] this->Streamers;
  this->Streamers = 0;
  this->NumberOfStreamers = 0;

  output->Squeeze();
  pts->Delete();

  return 1;
}

void vtkStreamPoints::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Time Increment: " << this->TimeIncrement << " <<\n";
}
