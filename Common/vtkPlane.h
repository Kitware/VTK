/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPlane.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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

class VTK_EXPORT vtkPlane : public vtkImplicitFunction
{
public:
  // Description
  // Construct plane passing through origin and normal to z-axis.
  static vtkPlane *New();

  vtkTypeMacro(vtkPlane,vtkImplicitFunction);
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
  vtkPlane(const vtkPlane&);
  void operator=(const vtkPlane&);

  float Normal[3];
  float Origin[3];

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


