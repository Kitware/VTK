/*=========================================================================

  Program:   Visualization Library
  Module:    Planes.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "Planes.hh"
#include "Plane.hh"

vlPlanes::vlPlanes()
{
  this->Points = NULL;
  this->Normals = NULL;
}

vlPlanes::~vlPlanes()
{
  if ( this->Points )
    this->Points->UnRegister(this);

  if ( this->Normals )
    this->Normals->UnRegister(this);
}

// Description
// Evaluate plane equations. Return smallest absolute value.
float vlPlanes::EvaluateFunction(float x[3])
{
  static vlPlane plane;
  int numPlanes, i;
  float val, minVal;

  if ( !this->Points || ! this->Normals )
    {
    vlErrorMacro(<<"Please define points and/or normals!");
    return LARGE_FLOAT;
    }

  if ( (numPlanes=this->Points->GetNumberOfPoints()) != this->Normals->GetNumberOfNormals() )
    {
    vlErrorMacro(<<"Number of normals/points inconsistent!");
    return LARGE_FLOAT;
    }

  for (minVal=LARGE_FLOAT, i=0; i < numPlanes; i++)
    {
    val = plane.Evaluate(this->Normals->GetNormal(i),this->Points->GetPoint(i), x);
    if ( val < minVal ) minVal = val;
    }

  return minVal;
}

// Description
// Evaluate planes gradient.
void vlPlanes::EvaluateGradient(float x[3], float n[3])
{
  static vlPlane plane;
  int numPlanes, i;
  float val, minVal, *nTemp;

  if ( !this->Points || ! this->Normals )
    {
    vlErrorMacro(<<"Please define points and/or normals!");
    return;
    }

  if ( (numPlanes=this->Points->GetNumberOfPoints()) != this->Normals->GetNumberOfNormals() )
    {
    vlErrorMacro(<<"Number of normals/points inconsistent!");
    return;
    }

  for (minVal=LARGE_FLOAT, i=0; i < numPlanes; i++)
    {
    nTemp = this->Normals->GetNormal(i);
    val = plane.Evaluate(nTemp,this->Points->GetPoint(i), x);
    if ( val < minVal )
      {
      minVal = val;
      n[0] = nTemp[0];
      n[1] = nTemp[1];
      n[2] = nTemp[2];
      }
    }
}

void vlPlanes::PrintSelf(ostream& os, vlIndent indent)
{
  int numPlanes;

  vlImplicitFunction::PrintSelf(os,indent);

  if ( this->Points && (numPlanes=this->Points->GetNumberOfPoints()) > 0 )
    {
    os << indent << "Number of Planes: " << numPlanes << "\n";
    }
  else
    {
    os << indent << "No Planes Defined.\n";
    }
}
