/*=========================================================================

  Program:   Visualization Library
  Module:    TransF.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

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
  vlPointData *pd;
  vlVectors *inVectors;
  vlFloatVectors *newVectors=NULL;
  vlNormals *inNormals;
  vlFloatNormals *newNormals=NULL;
  int numPts;
  vlPointSet *input=(vlPointSet *)this->Input;

  vlDebugMacro(<<"Executing transformation");
  this->Initialize();
//
// Check input
//
  if ( this->Transform == NULL )
    {
    vlErrorMacro(<<"No transform defined!");
    return;
    }

  inPts = input->GetPoints();
  pd = input->GetPointData();
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
  this->Transform->MultiplyPoints(inPts,newPts);
//
// Ditto for vectors and normals
//
  if ( inVectors )
    {
    this->Transform->MultiplyVectors(inVectors,newVectors);
    }

  if ( inNormals )
    {
    this->Transform->MultiplyNormals(inNormals,newNormals);
    }
//
// Update ourselves
//
  this->PointData.CopyVectorsOff();
  this->PointData.CopyNormalsOff();
  this->PointData.PassData(input->GetPointData());

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
