/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQuadric.cxx
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
#include <math.h>
#include "vtkQuadric.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkQuadric* vtkQuadric::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkQuadric");
  if(ret)
    {
    return (vtkQuadric*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkQuadric;
}




// Construct quadric with all coefficients = 1.
vtkQuadric::vtkQuadric()
{
  this->Coefficients[0] = 1.0;
  this->Coefficients[1] = 1.0;
  this->Coefficients[2] = 1.0;
  this->Coefficients[3] = 1.0;
  this->Coefficients[4] = 1.0;
  this->Coefficients[5] = 1.0;
  this->Coefficients[6] = 1.0;
  this->Coefficients[7] = 1.0;
  this->Coefficients[8] = 1.0;
  this->Coefficients[9] = 1.0;
}

// Set the 10 coefficients of the quadric equation.
void vtkQuadric::SetCoefficients(float a[10])
{
  int i;
  float *c=this->Coefficients;

  for (i=0; i < 10; i++ )
    {
    if ( a[i] != c[i] )
      {
      break;
      }
    }

  if ( i < 10 )
    {
    this->Modified();
    for (i=0; i < 10; i++ )
      {
      c[i] = a[i];
      }
    }
}

// Evaluate quadric equation.
float vtkQuadric::EvaluateFunction(float x[3])
{
  float *a = this->Coefficients;
  return ( a[0]*x[0]*x[0] + a[1]*x[1]*x[1] + a[2]*x[2]*x[2] +
           a[3]*x[0]*x[1] + a[4]*x[1]*x[2] + a[5]*x[0]*x[2] +
           a[6]*x[0] + a[7]*x[1] + a[8]*x[2] + a[9] );
}

// Evaluate the gradient to the quadric equation.
void vtkQuadric::EvaluateGradient(float x[3], float n[3])
{
  float *a=this->Coefficients;

  n[0] = 2.0*a[0]*x[0] + a[3]*x[1] + a[5]*x[2] + a[6];
  n[1] = 2.0*a[1]*x[1] + a[3]*x[0] + a[4]*x[2] + a[7];
  n[2] = 2.0*a[2]*x[2] + a[4]*x[1] + a[5]*x[0] + a[8];
}


// Set the 10 coefficients of the quadric equation.
void vtkQuadric::SetCoefficients(float a0,float a1,float a2,float a3, float a4, 
                                float a5,float a6,float a7,float a8, float a9)
{
  float a[10];

  a[0] = a0; a[1] = a1; a[2] = a2; a[3] = a3; a[4] = a4; 
  a[5] = a5; a[6] = a6; a[7] = a7; a[8] = a8; a[9] = a9; 

  vtkQuadric::SetCoefficients(a);
}
void vtkQuadric::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImplicitFunction::PrintSelf(os,indent);

  os << indent << "Coefficients: " 
     << "\n\ta0: " << this->Coefficients[0]
     << "\n\ta1: " << this->Coefficients[1]
     << "\n\ta2: " << this->Coefficients[2]
     << "\n\ta3: " << this->Coefficients[3]
     << "\n\ta4: " << this->Coefficients[4]
     << "\n\ta5: " << this->Coefficients[5]
     << "\n\ta6: " << this->Coefficients[6]
     << "\n\ta7: " << this->Coefficients[7]
     << "\n\ta8: " << this->Coefficients[8]
     << "\n\ta9: " << this->Coefficients[9] << "\n";
}
