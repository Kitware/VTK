/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImplicitFunction.cxx
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
#include "vtkImplicitFunction.h"
#include "vtkMath.h"

vtkCxxRevisionMacro(vtkImplicitFunction, "1.30");

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
  this->Superclass::PrintSelf(os,indent);

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


