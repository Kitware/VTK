/*=========================================================================

  Program:   Visualization Toolkit
  Module:    WarpVect.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "WarpVect.hh"

void vtkWarpVector::Execute()
{
  vtkPoints *inPts;
  vtkFloatPoints *newPts;
  vtkPointData *pd;
  vtkVectors *inVectors;
  int i, ptId;
  float *x, *v, newX[3];
  vtkPointSet *input=(vtkPointSet *)this->Input;

  vtkDebugMacro(<<"Warping data with vectors");
  this->Initialize();

  inPts = input->GetPoints();
  pd = input->GetPointData();
  inVectors = pd->GetVectors();

  if ( !inVectors || !inPts )
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
    v = inVectors->GetVector(ptId);
    for (i=0; i<3; i++)
      {
      newX[i] = x[i] + this->ScaleFactor * v[i];
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

void vtkWarpVector::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPointSetToPointSetFilter::PrintSelf(os,indent);

  os << indent << "Scale Factor: " << this->ScaleFactor << "\n";
}
