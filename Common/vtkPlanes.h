/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPlanes.h
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
// .NAME vtkPlanes - implicit function for convex set of planes
// .SECTION Description
// vtkPlanes computes the implicit function and function gradient for a set
// of planes. The planes must define a convex space.
//
// The function value is the closest first order distance of a point to the
// convex region defined by the planes. The function gradient is the plane
// normal at the function value.  Note that the normals must point outside of
// the convex region. Thus, a negative function value means that a point is
// inside the convex region.
//
// To define the planes you must create two objects: a subclass of 
// vtkPoints (e.g., vtkPoints) and a subclass of vtkNormals (e.g., 
// vtkNormals). The points define a point on the plane, and the normals
// specify plane normals.

#ifndef __vtkPlanes_h
#define __vtkPlanes_h

#include "vtkImplicitFunction.h"

class vtkPlane;

class VTK_EXPORT vtkPlanes : public vtkImplicitFunction
{
public:
  static vtkPlanes *New();
  vtkTypeMacro(vtkPlanes,vtkImplicitFunction);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description
  // Evaluate plane equations. Return smallest absolute value.
  float EvaluateFunction(float x[3]);
  float EvaluateFunction(float x, float y, float z)
    {return this->vtkImplicitFunction::EvaluateFunction(x, y, z); } ;

  // Description
  // Evaluate planes gradient.
  void EvaluateGradient(float x[3], float n[3]);

  // Description:
  // Specify a list of points defining points through which the planes pass.
  vtkSetObjectMacro(Points,vtkPoints);
  vtkGetObjectMacro(Points,vtkPoints);
  
  // Description:
  // Specify a list of normal vectors for the planes. There is a one-to-one
  // correspondence between plane points and plane normals.
  void SetNormals(vtkDataArray* normals);
  vtkGetObjectMacro(Normals,vtkDataArray);
  void SetNormals(vtkNormals* normals)
    { this->SetNormals(normals->GetData()); }

  // Description:
  // Specify the planes - see camera Get frustum planes definition.
  void SetFrustumPlanes(float planes[24]);

  // Description:
  // Return the number of planes in the set of planes.
  int GetNumberOfPlanes();
  
  // Description:
  // Create and return a pointer to a vtkPlane object at the ith
  // position. It is your responsibility to delete the vtkPlane
  // when done with it. Asking for a plane outside the allowable
  // range returns NULL.
  vtkPlane *GetPlane(int i);

protected:
  vtkPlanes();
  ~vtkPlanes();
  vtkPlanes(const vtkPlanes&) {};
  void operator=(const vtkPlanes&) {};

  vtkPoints *Points;
  vtkDataArray *Normals;
  vtkPlane *Plane;

private:
  float Planes[24];

};

#endif


