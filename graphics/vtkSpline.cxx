/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSpline.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/

#include "vtkSpline.h"

// Description
// Construct a spline wth the folloing defaults:
// ClampValueOff
vtkSpline::vtkSpline ()
{
  this->ComputeTime = 0;
  this->ClampValue = 0;
  this->PiecewiseFunction = new vtkPiecewiseFunction;
  this->Intervals = NULL;
  this->Coefficients = NULL;
  this->LeftConstraint = 1;
  this->LeftValue = 0.0;
  this->RightConstraint = 1;
  this->RightValue = 0.0;
}

// Description
// Add a point to the Piecewise Functions containing the data
void vtkSpline::AddPoint (float t, float x)
{
  this->PiecewiseFunction->AddPoint (t, x);
}

// Description
// Remove a point from the Piecewise Functions.
void vtkSpline::RemovePoint (float t)
{
  this->PiecewiseFunction->RemovePoint (t);
}

// Description
// Remove all points from the Piecewise Functions.
void vtkSpline::RemoveAllPoints ()
{
  this->PiecewiseFunction->RemoveAllPoints ();
}

// Description
// Evaluate a 1D Spline
float vtkSpline::Evaluate (float t)
{
  int i, index;
  int size = this->PiecewiseFunction->GetSize ();
  float *intervals;
  float *coefficients;

  // check to see if we need to recompute the spline
  if (this->ComputeTime < this->GetMTime ())
    {
    this->Compute ();
    }	

  intervals = this->Intervals;
  coefficients = this->Coefficients;

  // clamp the function at both ends
  if (t < intervals[0]) t = intervals[0];
  if (t > intervals[size - 1]) t = intervals[size - 1];

  // find pointer to cubic spline coefficient
  for (i = 1; i < size; i++)
    {
    index = i - 1;
    if (t < intervals[i]) break;
    }

  // calculate offset within interval
  t = (t - intervals[index]) / (intervals[index+1] - intervals[index]);

  // evaluate y
  return (t * (t * (t * *(coefficients + index * 4 + 3)
                      + *(coefficients + index * 4 + 2))
                      + *(coefficients + index * 4 + 1))
                      + *(coefficients + index * 4));
}

// Description:
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
  vtkObject::PrintSelf(os,indent);
  os << indent << "ClampValue: " << (this->ClampValue ? "On\n" : "Off\n");
  os << indent << "Piecewise Function:\n";
  this->PiecewiseFunction->PrintSelf(os,indent.GetNextIndent());
}






















