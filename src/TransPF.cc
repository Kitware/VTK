/*=========================================================================

  Program:   Visualization Library
  Module:    TransPF.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "TransPF.hh"
#include "FNormals.hh"
#include "FVectors.hh"

void vlTransformPolyFilter::Execute()
{
  vlPoints *inPts;
  vlFloatPoints *newPts;
  vlPointData *pd;
  vlVectors *inVectors;
  vlFloatVectors *newVectors=NULL;
  vlNormals *inNormals;
  vlFloatNormals *newNormals=NULL;
  int numPts;
  vlPolyData *input=(vlPolyData *)this->Input;

  vlDebugMacro(<<"Executing polygonal transformation");
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

  this->SetVerts(input->GetVerts());
  this->SetLines(input->GetLines());
  this->SetPolys(input->GetPolys());
  this->SetStrips(input->GetStrips());
}

unsigned long vlTransformPolyFilter::GetMTime()
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

void vlTransformPolyFilter::PrintSelf(ostream& os, vlIndent indent)
{
  vlPolyToPolyFilter::PrintSelf(os,indent);

  os << indent << "Transform: " << this->Transform << "\n";
}
