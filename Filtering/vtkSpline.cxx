/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSpline.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSpline.h"

#include "vtkPiecewiseFunction.h"

vtkCxxRevisionMacro(vtkSpline, "1.23");

// Construct a spline wth the folloing defaults:
// ClampValueOff
vtkSpline::vtkSpline ()
{
  this->ComputeTime = 0;
  this->ClampValue = 0;
  this->PiecewiseFunction = vtkPiecewiseFunction::New();
  this->Intervals = NULL;
  this->Coefficients = NULL;
  this->LeftConstraint = 1;
  this->LeftValue = 0.0;
  this->RightConstraint = 1;
  this->RightValue = 0.0;
  this->Closed = 0;
}

vtkSpline::~vtkSpline ()
{
  if (this->PiecewiseFunction)
    {
    this->PiecewiseFunction->Delete();
    }
  if (this->Coefficients)
    {
    delete [] this->Coefficients;
    }
  if (this->Intervals)
    {
    delete [] this->Intervals;
    }
}

double vtkSpline::ComputeLeftDerivative()
{
  double *dptr = this->PiecewiseFunction->GetDataPointer();
  int size = this->PiecewiseFunction->GetSize();
  if ( dptr == NULL || size < 2 )
    {
    return 0.0;
    }
  else
    {
    return (dptr[2]-dptr[0]);
    }
}

double vtkSpline::ComputeRightDerivative()
{
  double *dptr = this->PiecewiseFunction->GetDataPointer();
  int size = this->PiecewiseFunction->GetSize();
  if ( dptr == NULL || size < 2 )
    {
    return 0.0;
    }
  else
    {
    return (dptr[(size-1)*2]-dptr[(size-2)*2]);
    }
}

// Add a point to the Piecewise Functions containing the data
void vtkSpline::AddPoint (double t, double x)
{
  this->PiecewiseFunction->AddPoint (t, x);
}

// Remove a point from the Piecewise Functions.
void vtkSpline::RemovePoint (double t)
{
  this->PiecewiseFunction->RemovePoint (t);
}

// Remove all points from the Piecewise Functions.
void vtkSpline::RemoveAllPoints ()
{
  this->PiecewiseFunction->RemoveAllPoints ();
}

void vtkSpline::DeepCopy(vtkSpline *s)
{
  vtkSpline *spline = vtkSpline::SafeDownCast(s);

  if ( spline != NULL )
    {
    this->ClampValue = s->ClampValue;
    this->LeftConstraint = s->LeftConstraint;
    this->LeftValue = s->LeftValue;
    this->RightConstraint = s->RightConstraint;
    this->RightValue = s->RightValue;
    this->Closed = s->Closed;
    this->PiecewiseFunction->DeepCopy(s->PiecewiseFunction);
    }
}

// Overload standard modified time function. If data is modified,
// then this object is modified as well.
unsigned long vtkSpline::GetMTime()
{
  unsigned long mTime=this->vtkObject::GetMTime();
  unsigned long DataMTime;

  if ( this->PiecewiseFunction != NULL )
    {
    DataMTime = this->PiecewiseFunction->GetMTime();
    mTime = ( DataMTime > mTime ? DataMTime : mTime );
    }

  return mTime;
}

void vtkSpline::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Clamp Value: " << (this->ClampValue ? "On\n" : "Off\n");
  os << indent << "Left Constraint: " << this->LeftConstraint << "\n";
  os << indent << "Right Constraint: " << this->RightConstraint << "\n";
  os << indent << "Left Value: " << this->LeftValue << "\n";
  os << indent << "Right Value: " << this->RightValue << "\n";
  os << indent << "Closed: " << (this->Closed ? "On\n" : "Off\n");

  os << indent << "Piecewise Function:\n";
  this->PiecewiseFunction->PrintSelf(os,indent.GetNextIndent());

  os << indent << "Closed: " << (this->Closed ? "On\n" : "Off\n");
}
