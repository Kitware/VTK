/*=========================================================================

  Program:   Visualization Library
  Module:    WarpScal.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "WarpScal.hh"

void vlWarpScalar::Execute()
{
  vlPoints *inPts;
  vlNormals *inNormals;
  vlScalars *inScalars;
  vlFloatPoints *newPts;
  vlPointData *pd;
  int i, ptId;
  float *x, *n, s, newX[3];
  
  vlDebugMacro(<<"Warping data with scalars");
  this->Initialize();

  inPts = this->Input->GetPoints();
  pd = this->Input->GetPointData();
  inNormals = pd->GetNormals();
  inScalars = pd->GetScalars();

  if ( !inNormals || !inPts || !inScalars )
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
    n = inNormals->GetNormal(ptId);
    s = inScalars->GetScalar(ptId);
    for (i=0; i<3; i++)
      {
      newX[i] = x[i] + this->ScaleFactor * s * n[i];
      }
    newPts->SetPoint(ptId, newX);
    }
//
// Update ourselves
//
  this->SetPoints(newPts);
}

void vlWarpScalar::PrintSelf(ostream& os, vlIndent indent)
{
  if (this->ShouldIPrint(vlWarpScalar::GetClassName()))
    {
    vlPointSetToPointSetFilter::PrintSelf(os,indent);

    os << indent << "Scale Factor: " << this->ScaleFactor << "\n";
    }
}
