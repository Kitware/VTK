/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStreamPoints.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkStreamPoints.h"
#include "vtkObjectFactory.h"
#include "vtkFloatArray.h"

vtkCxxRevisionMacro(vtkStreamPoints, "1.35");
vtkStandardNewMacro(vtkStreamPoints);

// Construct object with time increment set to 1.0.
vtkStreamPoints::vtkStreamPoints()
{
  this->TimeIncrement = 1.0;
  this->NumberOfStreamers = 0;
}

void vtkStreamPoints::Execute()
{
  vtkStreamPoint *sPrev, *sPtr;
  vtkPoints *newPts;
  vtkFloatArray *newVectors;
  vtkFloatArray *newScalars=NULL;
  vtkCellArray *newVerts;
  vtkIdType i, ptId, id;
  int j;
  float tOffset, x[3], v[3], s, r;
  vtkPolyData *output = this->GetOutput();
  vtkDataSet *input = this->GetInput();
  vtkIdList *pts;

  this->SavePointInterval = this->TimeIncrement;  
  this->vtkStreamer::Integrate();
  if ( this->NumberOfStreamers <= 0 )
    {
    return;
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
    output->GetPointData()->SetScalars(newScalars);
    newScalars->Delete();
    }

  // Delete the streamers since they are no longer needed
  delete[] this->Streamers;
  this->Streamers = 0;
  this->NumberOfStreamers = 0;

  output->Squeeze();
  pts->Delete();
}

void vtkStreamPoints::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Time Increment: " << this->TimeIncrement << " <<\n";
}


