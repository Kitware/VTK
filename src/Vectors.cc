/*=========================================================================

  Program:   Visualization Library
  Module:    Vectors.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "Vectors.hh"
#include "FVectors.hh"
#include "IdList.hh"
#include "vlMath.hh"

vlVectors::vlVectors()
{
  this->MaxNorm = 0.0;
}

// Description:
// Given a list of pt ids, return an array of vectors.
void vlVectors::GetVectors(vlIdList& ptId, vlFloatVectors& fp)
{
  for (int i=0; i<ptId.GetNumberOfIds(); i++)
    {
    fp.InsertVector(i,this->GetVector(ptId.GetId(i)));
    }
}

// Description:
// Compute the largest norm for these vectors.
void vlVectors::ComputeMaxNorm()
{
  int i;
  float *v, norm;
  vlMath math;

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
float vlVectors::GetMaxNorm()
{
  this->ComputeMaxNorm();
  return this->MaxNorm;
}

void vlVectors::PrintSelf(ostream& os, vlIndent indent)
{
  if (this->ShouldIPrint(vlVectors::GetClassName()))
    {
    vlObject::PrintSelf(os,indent);

    os << indent << "Number Of Vectors: " << this->GetNumberOfVectors() << "\n";
    os << indent << "Maximum Euclidean Norm: " << this->GetMaxNorm() << "\n";
    }
}
