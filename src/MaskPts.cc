/*=========================================================================

  Program:   Visualization Library
  Module:    MaskPts.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "MaskPts.hh"
#include "vlMath.hh"

void vlMaskPoints::Execute()
{
  vlFloatPoints *newPts;
  vlPointData *pd;
  int numPts=this->Input->GetNumberOfPoints();
  int numNewPts;
  float *x;
  int ptId, id;
//
// Check input
//
  vlDebugMacro(<<"Masking points");
  this->Initialize();

  if ( numPts < 1 )
    {
    vlErrorMacro(<<"No data to mask!");
    return;
    }

  pd = this->Input->GetPointData();
//
// Allocate space
//
  numNewPts = numPts / this->OnRatio;
  newPts = new vlFloatPoints(numNewPts);
  this->PointData.CopyAllocate(pd);
//
// Traverse points and copy
//
  if ( this->RandomMode ) // retro mode
    {
    vlMath math;
    float cap = 1.0 / this->OnRatio;
    for ( ptId = this->Offset; ptId < numPts;  ptId++)
      {
      if (math.Random() <= cap )
        {
        x =  this->Input->GetPoint(ptId);
        id = newPts->InsertNextPoint(x);
        this->PointData.CopyData(pd,ptId,id);
        }
      }
    }
  else // a.r. mode
    {
    for ( ptId = this->Offset; ptId < numPts;  ptId += this->OnRatio )
      {
      x =  this->Input->GetPoint(ptId);
      id = newPts->InsertNextPoint(x);
      this->PointData.CopyData(pd,ptId,id);
      }
    }
//
// Update ourselves
//
  this->SetPoints(newPts);
  this->Squeeze();

  vlDebugMacro(<<"Masked " << numPts << " original points to " << id+1 << " points");
}

void vlMaskPoints::PrintSelf(ostream& os, vlIndent indent)
{
  vlDataSetToPolyFilter::PrintSelf(os,indent);

  os << indent << "On Ratio: " << this->OnRatio << "\n";
  os << indent << "Offset: " << this->Offset << "\n";
  os << indent << "Random Mode: " << (this->RandomMode ? "On\n" : "Off\n");
}
