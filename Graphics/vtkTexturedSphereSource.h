/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTexturedSphereSource.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTexturedSphereSource - create a sphere centered at the origin
// .SECTION Description
// vtkTexturedSphereSource creates a polygonal sphere of specified radius 
// centered at the origin. The resolution (polygonal discretization) in both 
// the latitude (phi) and longitude (theta) directions can be specified. 
// It also is possible to create partial sphere by specifying maximum phi and 
// theta angles.

#ifndef __vtkTexturedSphereSource_h
#define __vtkTexturedSphereSource_h

#include "vtkPolyDataSource.h"

#define VTK_MAX_SPHERE_RESOLUTION 1024

class VTK_GRAPHICS_EXPORT vtkTexturedSphereSource : public vtkPolyDataSource 
{
public:
  vtkTypeRevisionMacro(vtkTexturedSphereSource,vtkPolyDataSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct sphere with radius=0.5 and default resolution 8 in both Phi
  // and Theta directions.
  static vtkTexturedSphereSource *New();

  // Description:
  // Set radius of sphere.
  vtkSetClampMacro(Radius,float,0.0,VTK_LARGE_FLOAT);
  vtkGetMacro(Radius,float);

  // Description:
  // Set the number of points in the longitude direction.
  vtkSetClampMacro(ThetaResolution,int,4,VTK_MAX_SPHERE_RESOLUTION);
  vtkGetMacro(ThetaResolution,int);

  // Description:
  // Set the number of points in the latitude direction.
  vtkSetClampMacro(PhiResolution,int,4,VTK_MAX_SPHERE_RESOLUTION);
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
  vtkTexturedSphereSource(int res=8);
  ~vtkTexturedSphereSource() {};

  void Execute();
  float Radius;
  float Theta;
  float Phi;
  int ThetaResolution;
  int PhiResolution;

private:
  vtkTexturedSphereSource(const vtkTexturedSphereSource&);  // Not implemented.
  void operator=(const vtkTexturedSphereSource&);  // Not implemented.
};

#endif


