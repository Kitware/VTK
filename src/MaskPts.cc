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
  for ( ptId = this->Offset; ptId < numPts;  ptId += this->OnRatio )
    {
    x =  this->Input->GetPoint(ptId);
    id = newPts->InsertNextPoint(x);
    this->PointData.CopyData(pd,ptId,id);
    }
//
// Update ourselves
//
  this->SetPoints(newPts);
}

void vlMaskPoints::PrintSelf(ostream& os, vlIndent indent)
{
  vlDataSetToPolyFilter::PrintSelf(os,indent);

  os << indent << "On Ratio: " << this->OnRatio << "\n";
  os << indent << "Offset: " << this->Offset << "\n";
}
