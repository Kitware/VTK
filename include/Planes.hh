/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Planes.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkPlanes - implicit function for convex set of planes
// .SECTION Description
// vtkPlanes computes the implicit function and function gradient for a set
// of planes. The planes must define a convex space.
//    The function value is the closest distance of a point to any of the 
// planes. The function gradient is the plane normal at the function value.
// Note that the normals must point outside of the convex region. Thus a 
// negative function value means that a point is inside the convex region.
//    To define the planes you must create two objects: a subclass of 
// vtkPoints (e.g., vtkFloatPoints) and a subclass of vtkNormals (e.g., 
// vtkFloatNormals). The points define a point on the plane, and the normals
// specify plane normals.

#ifndef __vtkPlanes_h
#define __vtkPlanes_h

#include <math.h>
#include "ImpFunc.hh"

class vtkPlanes : public vtkImplicitFunction
{
public:
  vtkPlanes();
  ~vtkPlanes();
  char *GetClassName() {return "vtkPlanes";};
  void PrintSelf(ostream& os, vtkIndent indent);

  float EvaluateFunction(float x[3]);
  void EvaluateGradient(float x[3], float n[3]);

  vtkSetRefCountedObjectMacro(Points,vtkPoints);
  vtkGetObjectMacro(Points,vtkPoints);

  vtkSetRefCountedObjectMacro(Normals,vtkNormals);
  vtkGetObjectMacro(Normals,vtkNormals);

protected:
  vtkPoints *Points;
  vtkNormals *Normals;

};

#endif


