/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImplicitFunction.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImplicitFunction.h"

#include "vtkMath.h"
#include "vtkAbstractTransform.h"
#include "vtkTransform.h"

vtkCxxSetObjectMacro(vtkImplicitFunction,Transform,vtkAbstractTransform);

vtkImplicitFunction::vtkImplicitFunction()
{
  this->Transform = NULL;
}

vtkImplicitFunction::~vtkImplicitFunction()
{
  //static_cast needed since otherwise the
  //call to SetTransform becomes ambiguous
  this->SetTransform(static_cast<vtkAbstractTransform*>(NULL));
}

// Evaluate function at position x-y-z and return value. Point x[3] is
// transformed through transform (if provided).
double vtkImplicitFunction::FunctionValue(const double x[3])
{
  if ( ! this->Transform )
  {
    return this->EvaluateFunction(const_cast<double *>(x));
  }
  else //pass point through transform
  {
    double pt[3];
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
    double pt[3];
    double A[3][3];
    this->Transform->Update();
    this->Transform->InternalTransformDerivative(x,pt,A);
    double val = this->EvaluateFunction((double *)pt);

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
void vtkImplicitFunction::FunctionGradient(const double x[3], double g[3])
{
  if ( ! this->Transform )
  {
    this->EvaluateGradient(const_cast<double *>(x),g);
  }
  else //pass point through transform
  {
    double pt[3];
    double A[3][3];
    this->Transform->Update();
    this->Transform->InternalTransformDerivative(x,pt,A);
    this->EvaluateGradient(static_cast<double *>(pt),g);

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
vtkMTimeType vtkImplicitFunction::GetMTime()
{
  vtkMTimeType mTime=this->vtkObject::GetMTime();
  vtkMTimeType TransformMTime;

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

void vtkImplicitFunction::SetTransform(const double elements[16])
{
  vtkTransform* transform = vtkTransform::New();
  transform->SetMatrix(elements);
  this->SetTransform(transform);
  transform->Delete();
}

