/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TransF.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "TransF.hh"
#include "FNormals.hh"
#include "FVectors.hh"

void vtkTransformFilter::Execute()
{
  vtkPoints *inPts;
  vtkFloatPoints *newPts;
  vtkPointData *pd;
  vtkVectors *inVectors;
  vtkFloatVectors *newVectors=NULL;
  vtkNormals *inNormals;
  vtkFloatNormals *newNormals=NULL;
  int numPts;
  vtkPointSet *input=(vtkPointSet *)this->Input;

  vtkDebugMacro(<<"Executing transformation");
  this->Initialize();
//
// Check input
//
  if ( this->Transform == NULL )
    {
    vtkErrorMacro(<<"No transform defined!");
    return;
    }

  inPts = input->GetPoints();
  pd = input->GetPointData();
  inVectors = pd->GetVectors();
  inNormals = pd->GetNormals();

  if ( !inPts )
    {
    vtkErrorMacro(<<"No input data");
    return;
    }

  numPts = inPts->GetNumberOfPoints();
  newPts = new vtkFloatPoints(numPts);
  if ( inVectors ) newVectors = new vtkFloatVectors(numPts);
  if ( inNormals ) newNormals = new vtkFloatNormals(numPts);
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

unsigned long vtkTransformFilter::GetMTime()
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

void vtkTransformFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPointSetToPointSetFilter::PrintSelf(os,indent);

  os << indent << "Transform: " << this->Transform << "\n";
}
