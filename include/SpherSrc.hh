/*=========================================================================

  Program:   Visualization Toolkit
  Module:    SpherSrc.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkSphereSource - create a sphere centered at the origin
// .SECTION Description
// vtkSphereSource creates a polygonal sphere of specified radius centered 
// at the origin. The resolution (polygonal discretization) in both the
// latitude (phi) and longitude (theta) diections can be specified. It is
// also possible to create partial spheres by specifying maximum phi
// and theta angles.

#ifndef __vtkSphereSource_h
#define __vtkSphereSource_h

#include "PolySrc.hh"

#define MAX_SPHERE_RESOLUTION 1024

class vtkSphereSource : public vtkPolySource 
{
public:
  vtkSphereSource(int res=8);
  char *GetClassName() {return "vtkSphereSource";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set radius of sphere.
  vtkSetClampMacro(Radius,float,0.0,LARGE_FLOAT);
  vtkGetMacro(Radius,float);

  // Description:
  // Set the number of points in the longitude direction.
  vtkSetClampMacro(ThetaResolution,int,4,MAX_SPHERE_RESOLUTION);
  vtkGetMacro(ThetaResolution,int);

  // Description:
  // Set the number of points in the latitude direction.
  vtkSetClampMacro(PhiResolution,int,4,MAX_SPHERE_RESOLUTION);
  vtkGetMacro(PhiResolution,int);

  // Description:
  // Set the maximum longitude angle.
  vtkSetClampMacro(Theta,float,0.0,360.0);
  vtkGetMacro(Theta,float);

  // Description:
  // Set the maximum latitude angle (0 is at north pole).
  vtkSetClampMacro(Phi,float,0.0,180.0);
  vtkGetMacro(Phi,float);

protected:
  void Execute();
  float Radius;
  float Theta;
  float Phi;
  int ThetaResolution;
  int PhiResolution;

};

#endif


