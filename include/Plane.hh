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

#include "ImpFunc.hh"

class vlPlane : public vlImplicitFunction
{
public:
  vlPlane();
  char *GetClassName() {return "vlPlane";};

  // project point onto plane, returning coordinates
  void ProjectPoint(float x[3], float origin[3], float normal[3], float xproj[3]);

  float Evaluate(float normal[3], float origin[3], float x[3])
    {return normal[0]*(x[0]-origin[0]) + normal[1]*(x[1]-origin[1]) + 
            normal[2]*(x[2]-origin[2]);};

  // ImplicitFunction interface
  float Evaluate(float x, float y, float z);
  void EvaluateNormal(float x, float y, float z, float n[3]);

  vlSetVector3Macro(Normal,float);
  vlGetVectorMacro(Normal,float,3);

  vlSetVector3Macro(Origin,float);
  vlGetVectorMacro(Origin,float,3);

protected:
  float Normal[3];
  float Origin[3];

};

#endif


