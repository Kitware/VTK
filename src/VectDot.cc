/*=========================================================================

  Program:   Visualization Library
  Module:    VectDot.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "VectDot.hh"
#include "vlMath.hh"

// Description:
// Construct object with scalar range is (-1,1).
vlVectorDot::vlVectorDot()
{
  this->ScalarRange[0] = -1.0;
  this->ScalarRange[1] = 1.0;
}

//
// Compute dot product.
//
void vlVectorDot::Execute()
{
  int ptId, numPts;
  vlFloatScalars *newScalars;
  vlMath math;
  vlDataSet *input=this->Input;
  vlPointData *pd=input->GetPointData();
  vlNormals *inNormals;
  vlVectors *inVectors;
  float s, *n, *v, min, max, dR, dS;
//
// Initialize
//
  vlDebugMacro(<<"Generating vector/normal dot product!");
  this->Initialize();

  if ( ((numPts=input->GetNumberOfPoints()) < 1) ||
  ((inVectors=pd->GetVectors()) == NULL) ||
  ((inNormals=pd->GetNormals()) == NULL ) )
    {
    vlErrorMacro(<< "No input!");
    return;
    }
//
// Allocate
//
  newScalars = new vlFloatScalars(numPts);
//
// Compute initial scalars
//
  for (min=LARGE_FLOAT,max=(-LARGE_FLOAT),ptId=0; ptId < numPts; ptId++)
    {
    n = inNormals->GetNormal(ptId);
    v = inVectors->GetVector(ptId);
    s = math.Dot(n,v);
    if ( s < min ) min = s;
    if ( s > max ) max = s;
    newScalars->InsertScalar(ptId,s);
    }
//
// Map scalars into scalar range
//
  if ( (dR=this->ScalarRange[1]-this->ScalarRange[0]) == 0.0 ) dR = 1.0;
  if ( (dS=max-min) == 0.0 ) dS = 1.0;

  for ( ptId=0; ptId < numPts; ptId++ )
    {
    s = newScalars->GetScalar(ptId);
    s = ((s - min)/dS) * dR + this->ScalarRange[0];
    newScalars->InsertScalar(ptId,s);
    }
//
// Update self
//
  this->PointData.CopyScalarsOff();
  this->PointData.PassData(this->Input->GetPointData());

  this->PointData.SetScalars(newScalars);
}

void vlVectorDot::PrintSelf(ostream& os, vlIndent indent)
{
  vlDataSetToDataSetFilter::PrintSelf(os,indent);

  os << indent << "Scalar Range: (" << this->ScalarRange[0] << ", "
                                << this->ScalarRange[1] << ")\n";
}
