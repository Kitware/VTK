/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPlane.h
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
// .NAME vtkPlane - perform various plane computations
// .SECTION Description
// vtkPlane provides methods for various plane computations. These include
// projecting points onto a plane, evaluating the plane equation, and 
// returning plane normal. vtkPlane is a concrete implementation of the 
// abstract class vtkImplicitFunction.

#ifndef __vtkPlane_h
#define __vtkPlane_h

#include <math.h>
#include "vtkImplicitFunction.h"

class VTK_COMMON_EXPORT vtkPlane : public vtkImplicitFunction
{
public:
  // Description
  // Construct plane passing through origin and normal to z-axis.
  static vtkPlane *New();

  vtkTypeRevisionMacro(vtkPlane,vtkImplicitFunction);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description
  // Evaluate plane equation for point x[3].
  float EvaluateFunction(float x[3]);
  float EvaluateFunction(float x, float y, float z)
    {return this->vtkImplicitFunction::EvaluateFunction(x, y, z); } ;

  // Description
  // Evaluate function gradient at point x[3].
  void EvaluateGradient(float x[3], float g[3]);

  // Description:
  // Set/get plane normal. Plane is defined by point and normal.
  vtkSetVector3Macro(Normal,float);
  vtkGetVectorMacro(Normal,float,3);

  // Description:
  // Set/get point through which plane passes. Plane is defined by point 
  // and normal.
  vtkSetVector3Macro(Origin,float);
  vtkGetVectorMacro(Origin,float,3);

  // Description
  // Project a point x onto plane defined by origin and normal. The 
  // projected point is returned in xproj. NOTE : normal assumed to
  // have magnitude 1.
  static void ProjectPoint(float x[3], float origin[3], float normal[3], 
                           float xproj[3]);
  static void ProjectPoint(double x[3], double origin[3], double normal[3], 
                           double xproj[3]);

  // Description
  // Project a point x onto plane defined by origin and normal. The 
  // projected point is returned in xproj. NOTE : normal does NOT have to 
  // have magnitude 1.
  static void GeneralizedProjectPoint(float x[3], float origin[3],
                                      float normal[3], float xproj[3]);
  
  // Description:
  // Quick evaluation of plane equation n(x-origin)=0.
  static float Evaluate(float normal[3], float origin[3], float x[3]);
  static float Evaluate(double normal[3], double origin[3], double x[3]);

  // Description:
  // Return the distance of a point x to a plane defined by n(x-p0) = 0. The
  // normal n[3] must be magnitude=1.
  static float DistanceToPlane(float x[3], float n[3], float p0[3]);
  
  // Description:
  // Given a line defined by the two points p1,p2; and a plane defined by the
  // normal n and point p0, compute an intersection. The parametric
  // coordinate along the line is returned in t, and the coordinates of 
  // intersection are returned in x. A zero is returned if the plane and line
  // are parallel.
  static int IntersectWithLine(float p1[3], float p2[3], float n[3], 
                               float p0[3], float& t, float x[3]);


protected:
  vtkPlane();
  ~vtkPlane() {};

  float Normal[3];
  float Origin[3];

private:
  vtkPlane(const vtkPlane&);  // Not implemented.
  void operator=(const vtkPlane&);  // Not implemented.
};

inline float vtkPlane::Evaluate(float normal[3], float origin[3], float x[3])
{
  return normal[0]*(x[0]-origin[0]) + normal[1]*(x[1]-origin[1]) + 
         normal[2]*(x[2]-origin[2]);
}
inline float vtkPlane::Evaluate(double normal[3], double origin[3],double x[3])
{
  return normal[0]*(x[0]-origin[0]) + normal[1]*(x[1]-origin[1]) + 
         normal[2]*(x[2]-origin[2]);
}

inline float vtkPlane::DistanceToPlane(float x[3], float n[3], float p0[3])
{
  return ((float) fabs(n[0]*(x[0]-p0[0]) + n[1]*(x[1]-p0[1]) + 
                       n[2]*(x[2]-p0[2])));
}

#endif


