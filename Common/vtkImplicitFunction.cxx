/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImplicitFunction.cxx
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
#include "vtkImplicitFunction.h"
#include "vtkMath.h"

vtkImplicitFunction::vtkImplicitFunction()
{
  this->Transform = NULL;
}

vtkImplicitFunction::~vtkImplicitFunction()
{
  this->SetTransform(NULL);
}

// Evaluate function at position x-y-z and return value. Point x[3] is
// transformed through transform (if provided).
float vtkImplicitFunction::FunctionValue(const float x[3])
{
  if ( ! this->Transform )
    {
    return this->EvaluateFunction((float *)x);
    }
  else //pass point through transform
    {
    float pt[3];
    this->Transform->TransformPoint(x,pt);
    return this->EvaluateFunction(pt);
    }

  /* Return negative if determinant of Jacobian matrix is negative,
     i.e. if the transformation has a flip.  This is more 'correct'
     than the above behaviour, because it turns the implicit surface
     inside-out in the same way that polygonal surfaces are turned
     inside-out by a flip.  It takes up too many valuable CPU cycles
     to check the determinant on every function evaluation, though.
  {
    float pt[3];
    float A[3][3];
    this->Transform->Update();
    this->Transform->InternalTransformDerivative(x,pt,A);
    float val = this->EvaluateFunction((float *)pt);

    if (vtkMath::Determinant3x3(A) < 0)
      {
      return -val;
      }
    else
      {
      return +val;
      }
    }
  */
}

// Evaluate function gradient at position x-y-z and pass back vector. Point
// x[3] is transformed through transform (if provided).
void vtkImplicitFunction::FunctionGradient(const float x[3], float g[3])
{
  if ( ! this->Transform )
    {
    this->EvaluateGradient((float *)x,g);
    }
  else //pass point through transform
    {
    float pt[3];
    float A[3][3];
    this->Transform->Update();
    this->Transform->InternalTransformDerivative(x,pt,A);
    this->EvaluateGradient((float *)pt,g);

    // The gradient must be transformed using the same math as is
    // use for a normal to a surface: it must be multiplied by the
    // inverse of the transposed inverse of the Jacobian matrix of 
    // the transform, which is just the transpose of the Jacobian.
    vtkMath::Transpose3x3(A,A);
    vtkMath::Multiply3x3(A,g,g);

    /* If the determinant of the Jacobian matrix is negative,
       then the gradient points in the opposite direction.  This
       behaviour is actually incorrect, but is necessary to
       balance the incorrect behaviour of FunctionValue.  Otherwise,
       if you feed certain VTK filters a transform with a flip
       the gradient will point in the wrong direction and they
       will never converge to a result */
       
    if (vtkMath::Determinant3x3(A) < 0)
      {
      g[0] = -g[0];
      g[1] = -g[1];
      g[2] = -g[2];
      }
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


