/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBox.h
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
// .NAME vtkBox - implicit function for a bounding box
// .SECTION Description
// vtkBox computes the implicit function and/or gradient for a axis-aligned
// bounding box. (The superclasses transform can be used to modify this
// orientation.) Each side of the box is orthogonal to all other sides
// meeting along shared edges and all faces are orthogonal to the x-y-z
// coordinate axes.  (If you wish to orient this box differently, recall that
// the superclass vtkImplicitFunction supports a transformation matrix.)
// vtkCube is a concrete implementation of vtkImplicitFunction.
// 
// .SECTION See Also
// vtkCubeSource vtkImplicitFunction

#ifndef __vtkBox_h
#define __vtkBox_h

#include "vtkImplicitFunction.h"

class VTK_COMMON_EXPORT vtkBox : public vtkImplicitFunction
{
public:
  vtkTypeRevisionMacro(vtkBox,vtkImplicitFunction);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description
  // Construct box with center at (0,0,0) and each side of length 1.0.
  static vtkBox *New();

  // Description
  // Evaluate box defined by the two points (pMin,pMax).
  float EvaluateFunction(float x[3]);
  float EvaluateFunction(float x, float y, float z)
    {return this->vtkImplicitFunction::EvaluateFunction(x, y, z); }

  // Description
  // Evaluate the gradient of the box.
  void EvaluateGradient(float x[3], float n[3]);

  // Description:
  // Set / get the bounding box using various methods.
  vtkSetVector3Macro(XMin,float);
  vtkGetVector3Macro(XMin,float);
  vtkSetVector3Macro(XMax,float);
  vtkGetVector3Macro(XMax,float);
  void SetBounds(float xMin, float xMax,
                 float yMin, float yMax,
                 float zMin, float zMax);
  void SetBounds(float bounds[6]);
  void GetBounds(float &xMin, float &xMax,
                 float &yMin, float &yMax,
                 float &zMin, float &zMax);
  void GetBounds(float bounds[6]);

  // Description:
  // Bounding box intersection modified from Graphics Gems Vol I. The method
  // returns a non-zero value if the bounding box is hit. Origin[3] starts
  // the ray, dir[3] is the vector components of the ray in the x-y-z
  // directions, coord[3] is the location of hit, and t is the parametric
  // coordinate along line. (Notes: the intersection ray dir[3] is NOT
  // normalized.  Valid intersections will only occur between 0<=t<=1.)
  static char IntersectBox(float bounds[6], float origin[3], float dir[3], 
                           float coord[3], float& t);

protected:
  vtkBox();
  ~vtkBox() {}

  float XMin[3];
  float XMax[3];

private:
  vtkBox(const vtkBox&);  // Not implemented.
  void operator=(const vtkBox&);  // Not implemented.
};

#endif


