/*=========================================================================

  Program:   Visualization Library
  Module:    ElevatF.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
//
// Methods for elevation filter
//
#include "ElevatF.hh"
#include "vlMath.hh"
#include "FScalars.hh"

// Description:
// Construct object with LowPoint=(0,0,0) and HighPoint=(0,0,1). Scalar
// range is (0,1).
vlElevationFilter::vlElevationFilter()
{
  this->LowPoint[0] = 0.0;
  this->LowPoint[1] = 0.0;
  this->LowPoint[2] = 0.0;
 
  this->HighPoint[0] = 0.0;
  this->HighPoint[1] = 0.0;
  this->HighPoint[2] = 1.0;

  this->ScalarRange[0] = 0.0;
  this->ScalarRange[1] = 1.0;
}

//
// Convert position along ray into scalar value.  Example use includes 
// coloring terrain by elevation.
//
void vlElevationFilter::Execute()
{
  int i, j, numPts;
  vlFloatScalars *newScalars;
  float l, *bounds, *x, s, v[3];
  float diffVector[3], diffScalar;
  vlMath math;
//
// Initialize
//
  vlDebugMacro(<<"Generating elevation scalars!");
  this->Initialize();

  if ( ((numPts=this->Input->GetNumberOfPoints()) < 1) )
    {
    vlErrorMacro(<< "No input!");
    return;
    }
//
// Allocate
//
  newScalars = new vlFloatScalars(numPts);
//
// Set up 1D parametric system
//
  bounds = this->Input->GetBounds();

  for (i=0; i<3; i++) diffVector[i] = this->HighPoint[i] - this->LowPoint[i];
  if ( (l = math.Dot(diffVector,diffVector)) == 0.0)
    {
    vlErrorMacro(<< this << ": Bad vector, using (0,0,1)\n");
    diffVector[0] = diffVector[1] = 0.0; diffVector[2] = 1.0;
    l = 1.0;
    }
//
// Compute parametric coordinate and map into scalar range
//
  diffScalar = this->ScalarRange[1] - this->ScalarRange[0];
  for (i=0; i<numPts; i++)
    {
    x = this->Input->GetPoint(i);
    for (j=0; j<3; j++) v[j] = x[j] - this->LowPoint[j];
    s = math.Dot(v,diffVector) / l;
    s = (s < 0.0 ? 0.0 : s > 1.0 ? 1.0 : s);
    newScalars->InsertScalar(i,this->ScalarRange[0]+s*diffScalar);
    }
//
// Update self
//
  this->PointData.CopyScalarsOff();
  this->PointData.PassData(this->Input->GetPointData());

  this->PointData.SetScalars(newScalars);
}

void vlElevationFilter::PrintSelf(ostream& os, vlIndent indent)
{
  vlDataSetToDataSetFilter::PrintSelf(os,indent);

  os << indent << "Low Point: (" << this->LowPoint[0] << ", "
                                << this->LowPoint[1] << ", "
                                << this->LowPoint[2] << ")\n";
  os << indent << "High Point: (" << this->HighPoint[0] << ", "
                                << this->HighPoint[1] << ", "
                                << this->HighPoint[2] << ")\n";
  os << indent << "Scalar Range: (" << this->ScalarRange[0] << ", "
                                << this->ScalarRange[1] << ")\n";
}
