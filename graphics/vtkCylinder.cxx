/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCylinder.cxx
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
#include "vtkCylinder.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkCylinder* vtkCylinder::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkCylinder");
  if(ret)
    {
    return (vtkCylinder*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkCylinder;
}




// Construct cylinder radius of 0.5.
vtkCylinder::vtkCylinder()
{
  this->Center[0] = this->Center[1] = this->Center[2] = 0.0;
  this->Radius = 0.5;
}

// Evaluate cylinder equation F(x,y,z) = (x-x0)^2 + (z-z0)^2 - R^2.
float vtkCylinder::EvaluateFunction(float xyz[3])
{
  float x = xyz[0] - this->Center[0];
  float z = xyz[2] - this->Center[2];

  return ( x * x + z * z - this->Radius*this->Radius );
}

// Evaluate cylinder function gradient.
void vtkCylinder::EvaluateGradient(float xyz[3], float g[3])
{
  float x = xyz[0] - this->Center[0];
  float z = xyz[2] - this->Center[2];

  g[0] = 2.0 * (x - this->Center[0]);
  g[1] = 0.0;
  g[2] = 2.0 * (z - this->Center[2]);
}

void vtkCylinder::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImplicitFunction::PrintSelf(os,indent);

  os << indent << "Center: " << "( " << this->Center[0] << ", " <<
     this->Center[1] << ", " << this->Center[2] << " )";

  os << indent << "Radius: " << this->Radius << "\n";
}
