/*=========================================================================

  Program:   Visualization Library
  Module:    Planes.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlPlanes - implicit function for convex set of planes
// .SECTION Description
// vlPlanes computes the implicit function and function gradient for a set
// of planes. The planes must define a convex space.
//    The function value is the closest distance of a point to any of the 
// planes. The function gradient is the plane normal at the function value.
// Note that the normals must point outside of the convex region. Thus a 
// negative function value means that a point is inside the convex region.
//    To define the planes you must create two objects: a subclass of 
// vlPoints (e.g., vlFloatPoints) and a subclass of vlNormals (e.g., 
// vlFloatNormals). The points define a point on the plane, and the normals
// specify plane normals.

#ifndef __vlPlanes_h
#define __vlPlanes_h

#include <math.h>
#include "ImpFunc.hh"

class vlPlanes : public vlImplicitFunction
{
public:
  vlPlanes();
  ~vlPlanes();
  char *GetClassName() {return "vlPlanes";};
  void PrintSelf(ostream& os, vlIndent indent);

  float EvaluateFunction(float x[3]);
  void EvaluateGradient(float x[3], float n[3]);

  vlSetRefCountedObjectMacro(Points,vlPoints);
  vlGetObjectMacro(Points,vlPoints);

  vlSetRefCountedObjectMacro(Normals,vlNormals);
  vlGetObjectMacro(Normals,vlNormals);

protected:
  vlPoints *Points;
  vlNormals *Normals;

};

#endif


