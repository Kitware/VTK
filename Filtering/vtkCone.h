/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCone.h
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
// .NAME vtkCone - implicit function for a cone
// .SECTION Description
// vtkCone computes the implicit function and function gradient for a cone.
// vtkCone is a concrete implementation of vtkImplicitFunction. The cone vertex
// is located at the origin with axis of rotation coincident with x-axis. (Use
// the superclass' vtkImplicitFunction transformation matrix if necessary to 
// reposition.) The angle specifies the angle between the axis of rotation 
// and the side of the cone.

// .SECTION Caveats
// The cone is infinite in extent. To truncate the cone use the 
// vtkImplicitBoolean in combination with clipping planes.

#ifndef __vtkCone_h
#define __vtkCone_h

#include "vtkImplicitFunction.h"

class VTK_FILTERING_EXPORT vtkCone : public vtkImplicitFunction
{
public:
  // Description
  // Construct cone with angle of 45 degrees.
  static vtkCone *New();

  vtkTypeRevisionMacro(vtkCone,vtkImplicitFunction);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description
  // Evaluate cone equation.
  float EvaluateFunction(float x[3]);
  float EvaluateFunction(float x, float y, float z)
    {return this->vtkImplicitFunction::EvaluateFunction(x, y, z); } ;

  // Description
  // Evaluate cone normal.
  void EvaluateGradient(float x[3], float g[3]);

  // Description:
  // Set/Get the cone angle (expressed in degrees).
  vtkSetClampMacro(Angle,float,0,89.0);
  vtkGetMacro(Angle,float);

protected:
  vtkCone();
  ~vtkCone() {};

  float Angle;

private:
  vtkCone(const vtkCone&);  // Not implemented.
  void operator=(const vtkCone&);  // Not implemented.
};

#endif


