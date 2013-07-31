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


//----------------------------------------------------------------------------
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

  this->ParametricRange[0] = -1;
  this->ParametricRange[1] = -1;
}

//----------------------------------------------------------------------------
vtkSpline::~vtkSpline ()
{
  this->PiecewiseFunction->Delete();
  delete [] this->Coefficients;
  delete [] this->Intervals;
}

//----------------------------------------------------------------------------
void vtkSpline::SetParametricRange(double tMin, double tMax)
{
  if ( tMin != this->ParametricRange[0] || tMax != this->ParametricRange[1] )
    {
    if ( tMin >= tMax )
      {
      tMax = tMin + 1;
      }

    this->ParametricRange[0] = tMin;
    this->ParametricRange[1] = tMax;

    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkSpline::GetParametricRange(double tRange[2]) const
{
  if ( this->ParametricRange[0] != this->ParametricRange[1] )
    {
    tRange[0] = this->ParametricRange[0];
    tRange[1] = this->ParametricRange[1];
    }
  else
    {
    tRange[0] = this->PiecewiseFunction->GetRange()[0];
    tRange[1] = this->PiecewiseFunction->GetRange()[1];
    }
}

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
int vtkSpline::GetNumberOfPoints()
{
  return this->PiecewiseFunction->GetSize();
}


//----------------------------------------------------------------------------
// Add a point to the Piecewise Functions containing the data
void vtkSpline::AddPoint (double t, double x)
{
  if ( this->ParametricRange[0] != this->ParametricRange[1] )
    {
    t = (t < this->ParametricRange[0] ? this->ParametricRange[0] :
         (t > this->ParametricRange[1] ? this->ParametricRange[1] : t));
    }
  this->PiecewiseFunction->AddPoint (t, x);
}

//----------------------------------------------------------------------------
// Remove a point from the Piecewise Functions.
void vtkSpline::RemovePoint (double t)
{
  if ( this->ParametricRange[0] != this->ParametricRange[1] )
    {
    t = (t < this->ParametricRange[0] ? this->ParametricRange[0] :
         (t > this->ParametricRange[1] ? this->ParametricRange[1] : t));
    }
  this->PiecewiseFunction->RemovePoint (t);
}

//----------------------------------------------------------------------------
// Remove all points from the Piecewise Functions.
void vtkSpline::RemoveAllPoints ()
{
  this->PiecewiseFunction->RemoveAllPoints ();
}

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
int vtkSpline::FindIndex(int size, double t)
{
  int index=0;
  if ( size > 2 ) //bisection method for speed
    {
    int rightIdx = size - 1;
    int centerIdx = rightIdx - size/2;
    for (int converged=0; !converged; )
      {
      if ( this->Intervals[index] <= t && t <= this->Intervals[centerIdx] )
        {
        rightIdx = centerIdx;
        }
      else //if ( this->Intervals[centerIdx] < t && t <= this->Intervals[rightIdx] )
        {
        index = centerIdx;
        }
      if ( (index + 1) == rightIdx )
        {
        converged = 1;
        }
      else
        {
        centerIdx = index + (rightIdx-index)/2;
        }
      }//while not converged
    }
  return index;
}


//----------------------------------------------------------------------------
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
