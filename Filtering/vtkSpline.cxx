/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSpline.cxx
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
#include "vtkSpline.h"

vtkCxxRevisionMacro(vtkSpline, "1.16");

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

// Add a point to the Piecewise Functions containing the data
void vtkSpline::AddPoint (float t, float x)
{
  this->PiecewiseFunction->AddPoint (t, x);
}

// Remove a point from the Piecewise Functions.
void vtkSpline::RemovePoint (float t)
{
  this->PiecewiseFunction->RemovePoint (t);
}

// Remove all points from the Piecewise Functions.
void vtkSpline::RemoveAllPoints ()
{
  this->PiecewiseFunction->RemoveAllPoints ();
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
