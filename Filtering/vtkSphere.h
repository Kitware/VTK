/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSphere.h
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
// .NAME vtkSphere - implicit function for a sphere
// .SECTION Description
// vtkSphere computes the implicit function and/or gradient for a sphere.
// vtkSphere is a concrete implementation of vtkImplicitFunction.

#ifndef __vtkSphere_h
#define __vtkSphere_h

#include "vtkImplicitFunction.h"

class VTK_FILTERING_EXPORT vtkSphere : public vtkImplicitFunction
{
public:
  vtkTypeMacro(vtkSphere,vtkImplicitFunction);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description
  // Construct sphere with center at (0,0,0) and radius=0.5.
  static vtkSphere *New();

  // Description
  // Evaluate sphere equation ((x-x0)^2 + (y-y0)^2 + (z-z0)^2) - R^2.
  float EvaluateFunction(float x[3]);
  float EvaluateFunction(float x, float y, float z)
    {return this->vtkImplicitFunction::EvaluateFunction(x, y, z); } ;

  // Description
  // Evaluate sphere gradient.
  void EvaluateGradient(float x[3], float n[3]);

  // Description:
  // Set / get the radius of the sphere.
  vtkSetMacro(Radius,float);
  vtkGetMacro(Radius,float);

  // Description:
  // Set / get the center of the sphere.
  vtkSetVector3Macro(Center,float);
  vtkGetVectorMacro(Center,float,3);

protected:
  vtkSphere();
  ~vtkSphere() {};

  float Radius;
  float Center[3];

private:
  vtkSphere(const vtkSphere&);  // Not implemented.
  void operator=(const vtkSphere&);  // Not implemented.
};

#endif


