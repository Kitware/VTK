/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCone.cxx
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
#include "vtkCone.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkCone* vtkCone::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkCone");
  if(ret)
    {
    return (vtkCone*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkCone;
}




// Construct cone with angle of 45 degrees.
vtkCone::vtkCone()
{
  this->Angle = 45.0;
}

// Evaluate cone equation.
float vtkCone::EvaluateFunction(float x[3])
{
  float tanTheta = (float) 
    tan((double)this->Angle*vtkMath::DegreesToRadians());
  return x[1]*x[1] + x[2]*x[2] - x[0]*x[0]*tanTheta*tanTheta;
}

// Evaluate cone normal.
void vtkCone::EvaluateGradient(float x[3], float g[3])
{
  float tanTheta = (float) 
    tan((double)this->Angle*vtkMath::DegreesToRadians());
  g[0] = -2.0*x[0]*tanTheta*tanTheta;
  g[1] = 2.0*x[1];
  g[2] = 2.0*x[2];
}

void vtkCone::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImplicitFunction::PrintSelf(os,indent);

  os << indent << "Angle: " << this->Angle << "\n";
}
