/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoMath.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

// .NAME vtkGeoMath - 
// .SECTION Description I wanted to hide the normal vtkCamera API

// .SECTION See Also
   
#ifndef __vtkGeoMath_h
#define __vtkGeoMath_h

#include "vtkObject.h"

class VTK_GEOVIS_EXPORT vtkGeoMath : public vtkObject
{
public:
  static vtkGeoMath *New();
  vtkTypeRevisionMacro(vtkGeoMath, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  static float EarthRadiusMeters() {return 6356750.0f;};

  static double DistanceSquared(double pt0[3], double pt1[3]);
  // Having altitude realtive to sea level causes issues....
  static void   LongLatAltToRect(double lla[3], double rect[3]);
  
protected:
  vtkGeoMath();
  ~vtkGeoMath();

private:
  vtkGeoMath(const vtkGeoMath&);  // Not implemented.
  void operator=(const vtkGeoMath&);  // Not implemented.
};

#endif
