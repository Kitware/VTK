/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Cone.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkCone - implicit function for a cone
// .SECTION Description
// vtkCone computes the implicit function and function gradient for a cone.
// vtkCone is a concrete implementation of vtkImplicitFunction. The cone vertex
// is located at the origin with axis of rotation coincident with z-axis. (Use
// a transform filter if necessary to reposition). The angle specifies the
// angle between the axis of rotation and the side of the cone.
// .SECTION Caveats
// The cone is infinite in extent. To truncate the cone use the 
// vtkImplicitBoolean in combination with clipping planes.

#ifndef __vtkCone_h
#define __vtkCone_h

#include "ImpFunc.hh"

class vtkCone : public vtkImplicitFunction
{
public:
  vtkCone();
  char *GetClassName() {return "vtkCone";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // ImplicitFunction interface
  float EvaluateFunction(float x[3]);
  void EvaluateGradient(float x[3], float g[3]);

  // Description:
  // Set/Get the cone angle (expressed in degrees).
  vtkSetClampMacro(Angle,float,0,89.0);
  vtkGetMacro(Angle,float);

protected:
  float Angle;

};

#endif


