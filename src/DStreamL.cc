/*=========================================================================

  Program:   Visualization Toolkit
  Module:    DStreamL.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "DStreamL.hh"

vtkDashedStreamLine::vtkDashedStreamLine()
{
  this->DashFactor = 0.75;
}

void vtkDashedStreamLine::Execute()
{
  vtkStreamPoint *sPrev, *sPtr;
  vtkFloatPoints *newPts;
  vtkFloatVectors *newVectors;
  vtkFloatScalars *newScalars=NULL;
  vtkCellArray *newLines;
  int i, ptId, j, pts[2];
  float tOffset, x[3], v[3], s, r, xPrev[3], vPrev[3], scalarPrev;
  float xEnd[3], vEnd[3], sEnd;

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
        for (j=0; j<3; j++)
          {
          xPrev[j] = sPrev->x[j];
          vPrev[j] = sPrev->v[j];
          }
        scalarPrev = sPrev->s;
        pts[0] = newPts->InsertNextPoint(sPrev->x);
        newVectors->InsertVector(pts[0],sPrev->v);
        if ( newScalars ) newScalars->InsertScalar(pts[0],sPrev->s);
        continue;
        }
//
// Search for end of dash...create end of one dash, beginning of next
//
      if ( (sPtr->t - tOffset) > this->DashTime )
        {
        while ( tOffset < sPtr->t )
          {
          r = (tOffset - sPrev->t) / (sPtr->t - sPrev->t);

          for (j=0; j<3; j++)
            {
            x[j] = sPrev->x[j] + r * (sPtr->x[j] - sPrev->x[j]);
            v[j] = sPrev->v[j] + r * (sPtr->v[j] - sPrev->v[j]);
            xEnd[j] = xPrev[j] + this->DashFactor * (x[j] - xPrev[j]);
            vEnd[j] = vPrev[j] + this->DashFactor * (v[j] - vPrev[j]);
            }

          // terminate this dash
          pts[1] = newPts->InsertNextPoint(xEnd);
          newVectors->InsertVector(pts[1],vEnd);

          if ( newScalars ) 
            {
            s = sPrev->s + r * (sPtr->s - sPrev->s);
            sEnd = scalarPrev + this->DashFactor * (s - scalarPrev);
            newScalars->InsertScalar(pts[1],sEnd);
            }
  
          newLines->InsertNextCell(2,pts);

          //start beginning of next dash
          pts[0] = newPts->InsertNextPoint(x);
          newVectors->InsertVector(pts[0],v);

          if ( newScalars ) newScalars->InsertScalar(pts[0],s);

          for (j=0; j<3; j++)
            {
            xPrev[j] = x[j];
            vPrev[j] = v[j];
            }
          if ( newScalars ) scalarPrev = s;

          tOffset += this->DashTime;
          } // while

        } //if dash should be created
      } //for this streamer
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

void vtkDashedStreamLine::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkStreamLine::PrintSelf(os,indent);

  os << indent << "Dash Factor: " << this->DashFactor << " <<\n";

}


