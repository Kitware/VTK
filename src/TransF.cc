/*=========================================================================

  Program:   Visualization Library
  Module:    TransF.cc
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
#include "TransF.hh"
#include "FNormals.hh"
#include "FVectors.hh"

void vlTransformFilter::Execute()
{
  vlPoints *inPts;
  vlFloatPoints *newPts;
  int i, ptId;
  float *x, *v, *n, newX[3];
  vlPointData *pd;
  vlVectors *inVectors;
  vlFloatVectors *newVectors=0;
  vlNormals *inNormals;
  vlFloatNormals *newNormals=0;
  int numPts;
  vlTransform trans;
//
// Initialize
//
  this->Initialize();

  inPts = this->Input->GetPoints();
  pd = this->Input->GetPointData();
  inVectors = pd->GetVectors();
  inNormals = pd->GetNormals();

  if ( !inPts )
    {
    vlErrorMacro(<<"No input data");
    return;
    }

  numPts = inPts->GetNumberOfPoints();
  newPts = new vlFloatPoints(numPts);
  if ( inVectors ) newVectors = new vlFloatVectors(numPts);
  if ( inNormals ) newNormals = new vlFloatNormals(numPts);
//
// Loop over all points, updating position
//
  trans.MultiplyPoints(inPts,newPts);
//
// Ditto for vectors and normals
//
  if ( inVectors )
    {
    trans.MultiplyVectors(inVectors,newVectors);
    }

  if ( inNormals )
    {
    trans.MultiplyNormals(inNormals,newNormals);
    }
//
// Update ourselves
//
  this->SetPoints(newPts);
  this->PointData.SetNormals(newNormals);
  this->PointData.SetVectors(newVectors);
}

unsigned long vlTransformFilter::GetMTime()
{
  unsigned long mTime=this->MTime.GetMTime();
  unsigned long transMTime;

  if ( this->Transform )
    {
    transMTime = this->Transform->GetMTime();
    mTime = ( transMTime > mTime ? transMTime : mTime );
    }

  return mTime;
}

void vlTransformFilter::PrintSelf(ostream& os, vlIndent indent)
{
  if (this->ShouldIPrint(vlTransformFilter::GetClassName()))
    {
    vlPointSetToPointSetFilter::PrintSelf(os,indent);

    os << indent << "Transform: " << this->Transform << "\n";
    }
}
