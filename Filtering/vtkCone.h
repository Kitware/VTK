/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCone.h
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

class VTK_EXPORT vtkCone : public vtkImplicitFunction
{
public:
  // Description
  // Construct cone with angle of 45 degrees.
  static vtkCone *New();

  vtkTypeMacro(vtkCone,vtkImplicitFunction);
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
  vtkCone(const vtkCone&);
  void operator=(const vtkCone&);

  float Angle;

};

#endif


