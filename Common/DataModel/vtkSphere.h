/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSphere.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSphere - implicit function for a sphere
// .SECTION Description
// vtkSphere computes the implicit function and/or gradient for a sphere.
// vtkSphere is a concrete implementation of vtkImplicitFunction. Additional
// methods are available for sphere-related computations, such as computing
// bounding spheres for a set of points, or set of spheres.

#ifndef __vtkSphere_h
#define __vtkSphere_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkImplicitFunction.h"

class VTKCOMMONDATAMODEL_EXPORT vtkSphere : public vtkImplicitFunction
{
public:
  vtkTypeMacro(vtkSphere,vtkImplicitFunction);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description
  // Construct sphere with center at (0,0,0) and radius=0.5.
  static vtkSphere *New();

  // Description
  // Evaluate sphere equation ((x-x0)^2 + (y-y0)^2 + (z-z0)^2) - R^2.
  double EvaluateFunction(double x[3]);
  double EvaluateFunction(double x, double y, double z)
    {return this->vtkImplicitFunction::EvaluateFunction(x, y, z); } ;

  // Description
  // Evaluate sphere gradient.
  void EvaluateGradient(double x[3], double n[3]);

  // Description:
  // Set / get the radius of the sphere. The default is 0.5.
  vtkSetMacro(Radius,double);
  vtkGetMacro(Radius,double);

  // Description:
  // Set / get the center of the sphere. The default is (0,0,0).
  vtkSetVector3Macro(Center,double);
  vtkGetVectorMacro(Center,double,3);

  // Description:
  // Create a bounding sphere from a set of points. The set of points is
  // defined by an array of doubles, in the order of x-y-z (which repeats for
  // each point).  An optional hints array provides a guess for the initial
  // bounding sphere; the two values in the hints array are the two points
  // expected to be the furthest apart. The output sphere consists of a
  // center (x-y-z) and a radius.
  static void ComputeBoundingSphere(float *pts, vtkIdType numPts, float sphere[4],
                                    vtkIdType hints[2]);
  static void ComputeBoundingSphere(double *pts, vtkIdType numPts, double sphere[4],
                                    vtkIdType hints[2]);

  // Description:
  // Create a bounding sphere from a set of spheres. The set of input spheres
  // is defined by an array of pointers to spheres. Each sphere is defined by
  // the 4-tuple: center(x-y-z)+radius. An optional hints array provides a
  // guess for the initial bounding sphere; the two values in the hints array
  // are the two spheres expected to be the furthest apart. The output sphere
  // consists of a center (x-y-z) and a radius.
  static void ComputeBoundingSphere(float **spheres, vtkIdType numSpheres, float sphere[4],
                                    vtkIdType hints[2]);
  static void ComputeBoundingSphere(double **spheres, vtkIdType numSpheres, double sphere[4],
                                    vtkIdType hints[2]);

protected:
  vtkSphere();
  ~vtkSphere() {};

  double Radius;
  double Center[3];

private:
  vtkSphere(const vtkSphere&);  // Not implemented.
  void operator=(const vtkSphere&);  // Not implemented.
};

#endif


