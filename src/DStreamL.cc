/*=========================================================================

  Program:   Visualization Library
  Module:    DStreamL.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "DStreamL.hh"

vlDashedStreamLine::vlDashedStreamLine()
{
  this->DashFactor = 0.75;
}

void vlDashedStreamLine::Execute()
{
  vlStreamPoint *sPrev, *sPtr;
  vlFloatPoints *newPts;
  vlFloatVectors *newVectors;
  vlFloatScalars *newScalars=NULL;
  vlCellArray *newLines;
  int i, ptId, j, pts[2];
  float tOffset, dashOffset, x[3], v[3], s, r;

  this->vlStreamer::Integrate();
  if ( this->NumberOfStreamers <= 0 ) return;
//
//  Convert streamer into lines. Lines may be dashed.
//
  newPts  = new vlFloatPoints(1000);
  newVectors  = new vlFloatVectors(1000);
  if ( this->Input->GetPointData()->GetScalars() || this->SpeedScalars )
    newScalars = new vlFloatScalars(1000);
  newLines = new vlCellArray();
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
        pts[0] = newPts->InsertNextPoint(sPtr->x);
        newVectors->InsertVector(pts[0],sPtr->v);
        if ( newScalars ) newScalars->InsertScalar(pts[0],sPtr->s);
        continue;
        }
//
// Start point of dash created here
//
      if ( (sPtr->t - tOffset) > this->DashTime )
        {
        r = (this->DashTime - (sPrev->t-tOffset)) / (sPtr->t - sPrev->t);
        for (j=0; j<3; j++)
          {
          x[j] = sPrev->x[j] + r * (sPtr->x[j] - sPrev->x[j]);
          v[j] = sPrev->v[j] + r * (sPtr->v[j] - sPrev->v[j]);
          }

        pts[0] = newPts->InsertNextPoint(x);
        newVectors->InsertVector(pts[0],v);

        if ( newScalars ) 
          {
          s = sPrev->s + r * (sPtr->s - sPrev->s);
          newScalars->InsertScalar(pts[0],s);
          }

        tOffset += this->DashTime;
        }
//
// End point of dash created here
//
      dashOffset = this->DashTime * this->DashFactor;
      if ( (sPtr->t - dashOffset) > this->DashTime )
        {
        r = (this->DashTime - (sPrev->t-dashOffset)) / (sPtr->t - sPrev->t);
        for (j=0; j<3; j++)
          {
          x[j] = sPrev->x[j] + r * (sPtr->x[j] - sPrev->x[j]);
          v[j] = sPrev->v[j] + r * (sPtr->v[j] - sPrev->v[j]);
          }

        pts[1] = newPts->InsertNextPoint(x);
        newVectors->InsertVector(pts[1],v);

        if ( newScalars ) 
          {
          s = sPrev->s + r * (sPtr->s - sPrev->s);
          newScalars->InsertScalar(pts[1],s);
          }

        newLines->InsertNextCell(2,pts);

        dashOffset += this->DashTime;
        }

      } //for this streamer
    } //for all streamers
//
// Update ourselves
//
  vlDebugMacro(<<"Created " << newPts->GetNumberOfPoints() << " points, "
               << newLines->GetNumberOfCells() << " lines");

  this->SetPoints(newPts);
  this->PointData.SetVectors(newVectors);
  if ( newScalars ) this->PointData.SetScalars(newScalars);
  this->SetLines(newLines);

  this->Squeeze();

}

void vlDashedStreamLine::PrintSelf(ostream& os, vlIndent indent)
{
  vlStreamLine::PrintSelf(os,indent);

  os << indent << "Dash Factor: " << this->DashFactor << " <<\n";

}


