/*=========================================================================

  Program:   Visualization Library
  Module:    SpherSrc.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
//
// Methods for Sphere generator
//
#include <math.h>
#include "SpherSrc.hh"

vlSphereSource::vlSphereSource(int res)
{
  res = res < 4 ? 4 : res;
  this->Radius = 0.5;
  this->ThetaResolution = 4;
  this->PhiResolution = 4;
}

void vlSphereSource::Execute()
{

}

void vlSphereSource::PrintSelf(ostream& os, vlIndent indent)
{
  if (this->ShouldIPrint(vlSphereSource::GetClassName()))
    {
    vlPolySource::PrintSelf(os,indent);

    os << indent << "Theta Resolution: " << this->ThetaResolution << "\n";
    os << indent << "Phi Resolution: " << this->PhiResolution << "\n";
    os << indent << "Radius: " << this->Radius << "\n";
    }
}
