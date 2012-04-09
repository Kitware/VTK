/*=============================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoMath.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=============================================================================*/

/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "vtkObjectFactory.h"
#include "vtkGeoMath.h"
#include "vtkMath.h"

vtkStandardNewMacro(vtkGeoMath);


//----------------------------------------------------------------------------
vtkGeoMath::vtkGeoMath()
{
}

//-----------------------------------------------------------------------------
vtkGeoMath::~vtkGeoMath()
{
}

//-----------------------------------------------------------------------------
void vtkGeoMath::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}


//-----------------------------------------------------------------------------
double vtkGeoMath::DistanceSquared(double pt0[3], double pt1[3])
{
  double tmp;
  double d2;

  tmp = pt1[0] - pt0[0];
  d2 = tmp * tmp;
  tmp = pt1[1] - pt0[1];
  d2 += tmp * tmp;
  tmp = pt1[2] - pt0[2];
  d2 += tmp * tmp;

  return d2;
}

//-----------------------------------------------------------------------------
void vtkGeoMath::LongLatAltToRect(double longLatAlt[3], double rect[3])
{
  double theta = vtkMath::RadiansFromDegrees( longLatAlt[0] );
  double phi   = vtkMath::RadiansFromDegrees( longLatAlt[1] );
  double cosPhi = cos(phi);
  double radius = vtkGeoMath::EarthRadiusMeters()+ longLatAlt[2];

  rect[2] = sin(phi) * radius;
  rect[1] = cos(theta) * cosPhi * radius;
  rect[0] = -sin(theta) * cosPhi * radius;
}

