/*=========================================================================

  Program:   Visualization Library
  Module:    WarpTo.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
#include "WarpTo.hh"

void vlWarpTo::Execute()
{
  vlPoints *inPts;
  vlFloatPoints *newPts;
  vlPointData *pd;
  int i, ptId;
  float *x, newX[3];
  vlPointSet *input=(vlPointSet *)this->Input;
  float dist;

  vlDebugMacro(<<"Warping data to a point");
  this->Initialize();

  inPts = input->GetPoints();
  pd = input->GetPointData();

  if (!inPts )
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
    for (i=0; i<3; i++)
      {
      newX[i] = (1.0 - this->ScaleFactor)*x[i] + 
	this->ScaleFactor*this->Position[i];
      }
    newPts->SetPoint(ptId, newX);
    }
  //
  // Update ourselves
  //
  this->PointData.CopyNormalsOff(); // distorted geometry - normals are bad
  this->PointData.PassData(input->GetPointData());

  this->SetPoints(newPts);
}

void vlWarpTo::PrintSelf(ostream& os, vlIndent indent)
{
  vlPointSetToPointSetFilter::PrintSelf(os,indent);

  os << indent << "Position: (" << this->Position[0] << ", " 
    << this->Position[1] << ", " << this->Position[2] << ")\n";
  os << indent << "Scale Factor: " << this->ScaleFactor << "\n";
}
