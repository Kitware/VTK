/*=========================================================================

  Program:   Visualization Library
  Module:    Plane.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlPlane - perform various plane computations
// .SECTION Description
// vlPlane provides methods for various plane computations. These include
// projecting points onto a plane, evaluating the plane equation, and 
// returning plane normal. vlPlane is a concrete implementation of the 
// abstract class vlImplicitFunction.

#ifndef __vlPlane_h
#define __vlPlane_h

#include <math.h>
#include "ImpFunc.hh"

class vlPlane : public vlImplicitFunction
{
public:
  vlPlane();
  char *GetClassName() {return "vlPlane";};

  // project point onto plane, returning coordinates
  void ProjectPoint(float x[3], float origin[3], float normal[3], float xproj[3]);

  // ImplicitFunction interface
  float Evaluate(float x, float y, float z);
  float Evaluate(float normal[3], float origin[3], float x[3]);
  void EvaluateNormal(float x, float y, float z, float n[3]);

  vlSetVector3Macro(Normal,float);
  vlGetVectorMacro(Normal,float,3);

  vlSetVector3Macro(Origin,float);
  vlGetVectorMacro(Origin,float,3);

  float DistanceToPlane(float x[3], float n[3], float p0[3]);

  int IntersectWithLine(float p1[3], float p2[3], float n[3], float p0[3],
                        float& t, float x[3]);

protected:
  float Normal[3];
  float Origin[3];

};

// Description:
// Quick evaluation of plane equation n(x-origin)=0.
inline float vlPlane::Evaluate(float normal[3], float origin[3], float x[3])
{
  return normal[0]*(x[0]-origin[0]) + normal[1]*(x[1]-origin[1]) + 
         normal[2]*(x[2]-origin[2]);
}

// Description:
// Return the distance of a point x to a plane defined by n(x-p0) = 0. The
// normal n[3] must be magnitude=1.
inline float vlPlane::DistanceToPlane(float x[3], float n[3], float p0[3])
{
  return ((float) fabs(n[0]*(x[0]-p0[0]) + n[1]*(x[1]-p0[1]) + 
                       n[2]*(x[2]-p0[2])));
}

#endif


