/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImplicitFunction.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
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
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkImplicitFunction.h"

vtkImplicitFunction::vtkImplicitFunction()
{
  this->Transform = NULL;
}

vtkImplicitFunction::~vtkImplicitFunction()
{
  this->SetTransform(NULL);
}

float vtkImplicitFunction::EvaluateFunction(float x, float y, float z)
{
  float xyz[3];
  xyz[0] = x; xyz[1] = y; xyz[2] = z;
  return this->EvaluateFunction (xyz);
}

// Evaluate function at position x-y-z and return value. Point x[3] is
// transformed through transform (if provided).
float vtkImplicitFunction::FunctionValue(float x[3])
{
  if ( ! this->Transform )
    {
    return this->EvaluateFunction(x);
    }

  else //pass point through transform
    {
    float pt[4];
    int i;

    pt[0] = x[0];
    pt[1] = x[1];
    pt[2] = x[2];
    pt[3] = 1.0;
    this->Transform->MultiplyPoint(pt,pt);
    if (pt[3] != 1.0 )
      {
      for (i=0; i<3; i++)
	{
	pt[i] /= pt[3];
	}
      }

    return this->EvaluateFunction((float *)pt);
    }
}

// Evaluate function gradient at position x-y-z and pass back vector. Point
// x[3] is transformed through transform (if provided).
void vtkImplicitFunction::FunctionGradient(float x[3], float g[3])
{
  if ( ! this->Transform )
    {
    this->EvaluateGradient(x,g);
    }

  else //pass point through transform
    {
    float pt[4];
    int i;

    pt[0] = x[0];
    pt[1] = x[1];
    pt[2] = x[2];
    pt[3] = 1.0;
    this->Transform->MultiplyPoint(pt,pt);
    if (pt[3] != 1.0 )
      {
      for (i=0; i<3; i++)
	{
	pt[i] /= pt[3];
	}
      }
    this->EvaluateGradient((float *)pt,g);
    }
}

// Overload standard modified time function. If Transform is modified,
// then this object is modified as well.
unsigned long vtkImplicitFunction::GetMTime()
{
  unsigned long mTime=this->vtkObject::GetMTime();
  unsigned long TransformMTime;

  if ( this->Transform != NULL )
    {
    TransformMTime = this->Transform->GetMTime();
    mTime = ( TransformMTime > mTime ? TransformMTime : mTime );
    }

  return mTime;
}

void vtkImplicitFunction::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkObject::PrintSelf(os,indent);

  if ( this->Transform )
    {
    os << indent << "Transform:\n";
    this->Transform->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "Transform: (None)\n";
    }
}

