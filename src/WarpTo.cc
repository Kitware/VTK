/*=========================================================================

  Program:   Visualization Toolkit
  Module:    WarpTo.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
#include "WarpTo.hh"

void vtkWarpTo::Execute()
{
  vtkPoints *inPts;
  vtkFloatPoints *newPts;
  vtkPointData *pd;
  int i, ptId;
  float *x, newX[3];
  vtkPointSet *input=(vtkPointSet *)this->Input;
  float dist;

  vtkDebugMacro(<<"Warping data to a point");
  this->Initialize();

  inPts = input->GetPoints();
  pd = input->GetPointData();

  if (!inPts )
    {
    vtkErrorMacro(<<"No input data");
    return;
    }

  newPts = new vtkFloatPoints(inPts->GetNumberOfPoints());
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
  // Update ourselves and release memory
  //
  this->PointData.CopyNormalsOff(); // distorted geometry - normals are bad
  this->PointData.PassData(input->GetPointData());

  this->SetPoints(newPts);
  newPts->Delete();
}

void vtkWarpTo::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPointSetToPointSetFilter::PrintSelf(os,indent);

  os << indent << "Position: (" << this->Position[0] << ", " 
    << this->Position[1] << ", " << this->Position[2] << ")\n";
  os << indent << "Scale Factor: " << this->ScaleFactor << "\n";
}
