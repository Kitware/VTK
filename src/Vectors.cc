/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Vectors.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "Vectors.hh"
#include "FVectors.hh"
#include "IdList.hh"
#include "vtkMath.hh"

vtkVectors::vtkVectors()
{
  this->MaxNorm = 0.0;
}

void vtkVectors::GetVector(int id, float v[3])
{
  float *vp = this->GetVector(id);
  for (int i=0; i<3; i++) v[i] = vp[i];
}

// Description:
// Insert vector into position indicated.
void vtkVectors::InsertVector(int id, float vx, float vy, float vz)
{
  float v[3];

  v[0] = vx;
  v[1] = vy;
  v[2] = vz;
  this->InsertVector(id,v);
}

// Description:
// Insert vector into position indicated.
int vtkVectors::InsertNextVector(float vx, float vy, float vz)
{
  float v[3];

  v[0] = vx;
  v[1] = vy;
  v[2] = vz;
  return this->InsertNextVector(v);
}

// Description:
// Given a list of pt ids, return an array of vectors.
void vtkVectors::GetVectors(vtkIdList& ptId, vtkFloatVectors& fp)
{
  for (int i=0; i<ptId.GetNumberOfIds(); i++)
    {
    fp.InsertVector(i,this->GetVector(ptId.GetId(i)));
    }
}

// Description:
// Compute the largest norm for these vectors.
void vtkVectors::ComputeMaxNorm()
{
  int i;
  float *v, norm;
  vtkMath math;

  if ( this->GetMTime() > this->ComputeTime )
    {
    this->MaxNorm = 0.0;
    for (i=0; i<this->GetNumberOfVectors(); i++)
      {
      v = this->GetVector(i);
      norm = math.Norm(v);
      if ( norm > this->MaxNorm ) this->MaxNorm = norm;
      }

    this->ComputeTime.Modified();
    }
}

// Description:
// Return the maximum norm for these vectors.
float vtkVectors::GetMaxNorm()
{
  this->ComputeMaxNorm();
  return this->MaxNorm;
}

void vtkVectors::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkRefCount::PrintSelf(os,indent);

  os << indent << "Number Of Vectors: " << this->GetNumberOfVectors() << "\n";
  os << indent << "Maximum Euclidean Norm: " << this->GetMaxNorm() << "\n";
}
