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
  this->StepLength = 0.01;
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
    if ( this->Streamers[ptId].GetNumberOfPoints() < 2 ) continue;

    sPrev = this->Streamers[ptId].GetStreamPoint(0);

    if ( this->Streamers[ptId].GetNumberOfPoints() == 2 )
      {
      sPtr = this->Streamers[ptId].GetStreamPoint(1);
      if ( sPtr->cellId < 0 ) continue;
      }

    tOffset = sPrev->t;

    for ( i=0, sPtr=sPrev; 
    i < this->Streamers[ptId].GetNumberOfPoints() && sPtr->cellId >= 0;
    i++, sPrev=sPtr, sPtr=this->Streamers[ptId].GetStreamPoint(i) )
      {

      if ( i == 0 ) //create first point
        {
        npts = 1;
        pts[0] = newPts->InsertNextPoint(sPrev->x);
        newVectors->InsertVector(pts[0],sPrev->v);
        if ( newScalars ) newScalars->InsertScalar(pts[0],sPrev->s);
        continue;
        }
//
// Search for end of segment and create line segments
//
      if ( (sPtr->t - tOffset) > this->StepLength )
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
          pts[npts] = newPts->InsertNextPoint(x);
          newVectors->InsertVector(pts[npts],v);

          if ( newScalars ) 
            {
            s = sPrev->s + r * (sPtr->s - sPrev->s);
            newScalars->InsertScalar(pts[npts],s);
            }
  
          if ( ++npts == MAX_CELL_SIZE )
            {
            newLines->InsertNextCell(npts,pts);
            pts[0] = pts[npts-1];
            npts = 1; //prepare for next line
            }

          tOffset += this->StepLength;
          } // while

        } //if points should be created
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

  os << indent << "Step Length: " << this->StepLength << " <<\n";

}


