/*=========================================================================

  Program:   Visualization Toolkit
  Module:    StreamL.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "StreamL.hh"

vtkStreamLine::vtkStreamLine()
{
  this->DashTime = 0.01;
}

void vtkStreamLine::Execute()
{
  vtkStreamPoint *sPrev, *sPtr;
  vtkFloatPoints *newPts;
  vtkFloatVectors *newVectors;
  vtkFloatScalars *newScalars=NULL;
  vtkCellArray *newLines;
  int i, ptId, j, npts, pts[MAX_CELL_SIZE];
  float tOffset, x[3], v[3], s, r;

  this->vtkStreamer::Integrate();
  if ( this->NumberOfStreamers <= 0 ) return;
//
//  Convert streamer into lines. Lines may be dashed.
//
  newPts  = new vtkFloatPoints(1000);
  newVectors  = new vtkFloatVectors(1000);
  if ( this->Input->GetPointData()->GetScalars() || this->SpeedScalars )
    newScalars = new vtkFloatScalars(1000);
  newLines = new vtkCellArray();
  newLines->Allocate(newLines->EstimateSize(2*this->NumberOfStreamers,MAX_CELL_SIZE));
//
// Loop over all streamers generating points
//
  for (ptId=0; ptId < this->NumberOfStreamers; ptId++)
    {
    tOffset = 0.0;

    if ( this->Streamers[ptId].GetNumberOfPoints() < 2 ) continue;
    if ( this->Streamers[ptId].GetNumberOfPoints() == 2 )
      {
      sPtr=this->Streamers[ptId].GetStreamPoint(1);
      if ( sPtr->cellId < 0 ) continue;
      }

    for ( sPrev=sPtr=this->Streamers[ptId].GetStreamPoint(0), i=0; 
    i < this->Streamers[ptId].GetNumberOfPoints() && sPtr->cellId >= 0;
    i++, sPrev=sPtr, sPtr=this->Streamers[ptId].GetStreamPoint(i) )
      {

      if ( i == 0 ) //create first point
        {
        npts = 0;
        pts[npts++] = newPts->InsertNextPoint(sPtr->x);
        newVectors->InsertVector(pts[npts-1],sPtr->v);
        if ( newScalars ) newScalars->InsertScalar(pts[npts-1],sPtr->s);
        continue;
        }
//
// For each streamer, create points approximately "time increment" apart
//
      if ( (sPtr->t - tOffset) > this->DashTime )
        {
        r = (this->DashTime - (sPrev->t-tOffset)) / (sPtr->t - sPrev->t);
        if ( r >= 0.0 )
          {
          for (j=0; j<3; j++)
            {
            x[j] = sPrev->x[j] + r * (sPtr->x[j] - sPrev->x[j]);
            v[j] = sPrev->v[j] + r * (sPtr->v[j] - sPrev->v[j]);
            }
            tOffset += this->DashTime;
          }
        else //more than one "dash time" apart
          {
          for (j=0; j<3; j++)
            {
            x[j] = sPtr->x[j];
            v[j] = sPtr->v[j];
            }
            tOffset = sPtr->t;
          }

        pts[npts++] = newPts->InsertNextPoint(x);
        newVectors->InsertVector(pts[npts-1],v);

        if ( newScalars ) 
          {
          s = sPrev->s + r * (sPtr->s - sPrev->s);
          newScalars->InsertScalar(pts[npts-1],s);
          }

        if ( npts == MAX_CELL_SIZE )
          {
          newLines->InsertNextCell(npts,pts);
          pts[0] = pts[npts-1];
          npts = 1; //prepare for next line
          }
        }
      } //for this streamer
      if ( npts > 1 ) newLines->InsertNextCell(npts,pts);
    } //for all streamers
//
// Update ourselves
//
  vtkDebugMacro(<<"Created " << newPts->GetNumberOfPoints() << " points, "
               << newLines->GetNumberOfCells() << " lines");

  this->SetPoints(newPts);
  this->PointData.SetVectors(newVectors);
  if ( newScalars ) this->PointData.SetScalars(newScalars);
  this->SetLines(newLines);

  this->Squeeze();

}

void vtkStreamLine::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkStreamer::PrintSelf(os,indent);

  os << indent << "Dash Time: " << this->DashTime << " <<\n";

}


