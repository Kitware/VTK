/*=========================================================================

  Program:   Visualization Toolkit
  Module:    StreamPt.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "StreamPt.hh"

vtkStreamPoints::vtkStreamPoints()
{
  this->TimeIncrement = 1.0;
}

void vtkStreamPoints::Execute()
{
  vtkStreamPoint *sPrev, *sPtr;
  vtkFloatPoints *newPts;
  vtkFloatVectors *newVectors;
  vtkFloatScalars *newScalars=NULL;
  int i, ptId, j, id;
  float tOffset, x[3], v[3], s, r;

  this->vtkStreamer::Integrate();
  if ( this->NumberOfStreamers <= 0 ) return;

  newPts  = new vtkFloatPoints(1000);
  newVectors  = new vtkFloatVectors(1000);
  if ( this->Input->GetPointData()->GetScalars() || this->SpeedScalars )
    newScalars = new vtkFloatScalars(1000);
//
// Loop over all streamers generating points
//
  for (ptId=0; ptId < this->NumberOfStreamers; ptId++)
    {
    tOffset = 0.0;

    for ( sPrev=sPtr=this->Streamers[ptId].GetStreamPoint(0), i=0; 
    i < this->Streamers[ptId].GetNumberOfPoints() && sPtr->cellId >= 0;
    i++, sPrev=sPtr, sPtr=this->Streamers[ptId].GetStreamPoint(i) )
      {

      if ( i == 0 ) //create first point
        {
        id = newPts->InsertNextPoint(sPtr->x);
        newVectors->InsertVector(id,sPtr->v);
        if ( newScalars ) newScalars->InsertScalar(id,sPtr->s);
        continue;
        }
//
// For each streamer, create points "time increment" apart
//
      if ( (sPtr->t - tOffset) > this->TimeIncrement )
        {
        r = (this->TimeIncrement - (sPrev->t-tOffset)) / (sPtr->t - sPrev->t);
        for (j=0; j<3; j++)
          {
          x[j] = sPrev->x[j] + r * (sPtr->x[j] - sPrev->x[j]);
          v[j] = sPrev->v[j] + r * (sPtr->v[j] - sPrev->v[j]);
          }

        id = newPts->InsertNextPoint(x);
        newVectors->InsertVector(id,v);

        if ( newScalars ) 
          {
          s = sPrev->s + r * (sPtr->s - sPrev->s);
          newScalars->InsertScalar(id,s);
          }

        tOffset += this->TimeIncrement;

        }
      } //for this streamer

    } //for all streamers
//
// Update ourselves
//
  vtkDebugMacro(<<"Created " << newPts->GetNumberOfPoints() << " points");

  this->SetPoints(newPts);
  newPts->Delete();

  this->PointData.SetVectors(newVectors);
  newVectors->Delete();

  if ( newScalars ) 
    {
    this->PointData.SetScalars(newScalars);
    newScalars->Delete();
    }

  this->Squeeze();
}

void vtkStreamPoints::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkStreamer::PrintSelf(os,indent);

  os << indent << "Time Increment: " << this->TimeIncrement << " <<\n";
}
