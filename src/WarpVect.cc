/*=========================================================================

  Program:   Visualization Library
  Module:    WarpVect.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Description:
---------------------------------------------------------------------------
This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "WarpVect.hh"

void vlWarpVector::Execute()
{
  vlPoints *inPts;
  vlFloatPoints *newPts;
  vlPointData *pd;
  vlVectors *inVectors;
  int i, ptId;
  float *x, *v, newX[3];
  
//
// Initialize
//
  this->Initialize();

  inPts = this->Input->GetPoints();
  pd = this->Input->GetPointData();
  inVectors = pd->GetVectors();

  if ( !inVectors || !inPts )
    {
    vlErrorMacro(<<"No input data");
    return;
    }

  newPts = new vlFloatPoints(inPts->GetNumberOfPoints());
//
// Loop over all points, adjusting locations
//
  for (ptId=0; ptId < inPts->GetNumberOfPoints(); ptId++)
    {
    x = inPts->GetPoint(ptId);
    v = inVectors->GetVector(ptId);
    for (i=0; i<3; i++)
      {
      newX[i] = x[i] + this->ScaleFactor * v[i];
      }
    newPts->SetPoint(ptId, newX);
    }
//
// Update ourselves
//
  this->SetPoints(newPts);
}

void vlWarpVector::PrintSelf(ostream& os, vlIndent indent)
{
  if (this->ShouldIPrint(vlWarpVector::GetClassName()))
    {
    vlPointSetToPointSetFilter::PrintSelf(os,indent);

    os << indent << "Scale Factor: " << this->ScaleFactor << "\n";
    }
}
