/*=========================================================================

  Program:   Visualization Toolkit
  Module:    VectNorm.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "VectNorm.hh"
#include "vtkMath.hh"
#include "FScalars.hh"

// Description:
// Construct with normalize flag off.
vtkVectorNorm::vtkVectorNorm()
{
  this->Normalize = 0;
}

void vtkVectorNorm::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToDataSetFilter::PrintSelf(os,indent);

  os << indent << "Normalize: " << (this->Normalize ? "On\n" : "Off\n");
}

void vtkVectorNorm::Execute()
{
  int i, numVectors;
  vtkFloatScalars *newScalars;
  float *v, s, maxScalar;
  vtkPointData *pd;
  vtkVectors *inVectors;
//
// Initialize
//
  vtkDebugMacro(<<"Normalizing vectors!");
  this->Initialize();

  pd = this->Input->GetPointData();
  inVectors = pd->GetVectors();

  if ( (numVectors=inVectors->GetNumberOfVectors()) < 1 )
    {
    vtkErrorMacro(<< "No input vectors!\n");
    return;
    }

//
// Allocate
//
  newScalars = new vtkFloatScalars(numVectors);
  for (maxScalar=0.0, i=0; i < numVectors; i++)
    {
    v = inVectors->GetVector(i);
    s = sqrt((double)v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
    if ( s > maxScalar ) maxScalar = s;
    newScalars->SetScalar(i,s);
    }
//
// If necessary, normalize
//
  if ( this->Normalize && maxScalar > 0.0 )
    {
    for (i=0; i < numVectors; i++)
      {
      s = newScalars->GetScalar(i);
      s /= maxScalar;
      newScalars->SetScalar(i,s);
      }
    }
//
// Update self and release memory
//
  this->PointData.CopyScalarsOff();
  this->PointData.PassData(pd);

  this->PointData.SetScalars(newScalars);
  newScalars->Delete();
}

