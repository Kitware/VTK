/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkSCurveSpline.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2009 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
  -------------------------------------------------------------------------*/
#include "vtkSCurveSpline.h"

#include "vtkObjectFactory.h"
#include "vtkPiecewiseFunction.h"
#include <cassert>
#include <algorithm> // for std::min()/std::max()

vtkStandardNewMacro(vtkSCurveSpline);

//----------------------------------------------------------------------------
// Construct a SCurve Spline.
vtkSCurveSpline::vtkSCurveSpline()
{
  this->NodeWeight = 0.0;
}

//----------------------------------------------------------------------------
// Evaluate a 1D Spline
double vtkSCurveSpline::Evaluate (double t)
{
  int index;
  double *intervals;
  double *coefficients;

  // check to see if we need to recompute the spline
  if (this->ComputeTime < this->GetMTime())
  {
    this->Compute ();
  }

  // make sure we have at least 2 points
  int size = this->PiecewiseFunction->GetSize();

  if (size < 2)
  {
    return 0.0;
  }

  intervals = this->Intervals;
  coefficients = this->Coefficients;

  if ( this->Closed )
  {
    size = size + 1;
  }

  // clamp the function at both ends
  if (t < intervals[0])
  {
    t = intervals[0];
  }
  if (t > intervals[size - 1])
  {
    t = intervals[size - 1];
  }

  // find pointer to cubic spline coefficient using bisection method
  index = this->FindIndex(size,t);

  // calculate offset within interval
  t = (t - intervals[index]);

  // normalize to unit width
  t /= intervals[index+1] - intervals[index];

  // apply weighting function
  if (this->NodeWeight > 0.0)
  {
    double shift = t * (t * (t * (-4*this->NodeWeight)
                             + (6*this->NodeWeight)))
      - this->NodeWeight;
    // clamp t
    t = std::max(std::min(t+shift,1.0),0.0);
  }

  // evaluate intervals value y
  return (t * (t * (t * *(coefficients + index * 3 + 2) // a
                    + *(coefficients + index * 3 + 1))) // b
          + *(coefficients + index * 3)); // d
}

//----------------------------------------------------------------------------
// Compute SCurve Splines for each dependent variable
void vtkSCurveSpline::Compute ()
{
  double *ts, *xs;
  //  double *work;
  double *coefficients;
  double *dependent;
  int size;
  int i;

  // Make sure the function is up to date.
  //this->PiecewiseFunction->Update();

  // get the size of the independent variables
  size = this->PiecewiseFunction->GetSize ();

  if(size < 2)
  {
    vtkErrorMacro("Cannot compute a spline with less than 2 points. # of points is: " << size);
    return;
  }

  // copy the independent variables. Note that if the spline
  // is closed the first and last point are assumed repeated -
  // so we add and extra point
  delete [] this->Intervals;

  if ( !this->Closed )
  {
    this->Intervals = new double[size];
    ts = this->PiecewiseFunction->GetDataPointer ();
    for (i = 0; i < size; i++)
    {
      this->Intervals[i] = *(ts + 2*i);
    }

    // allocate memory for work arrays
    //    work = new double[size];

    // allocate memory for coefficients
    delete [] this->Coefficients;
    this->Coefficients = new double [3*size];

    // allocate memory for dependent variables
    dependent = new double [size];

    // get start of coefficients for this dependent variable
    coefficients = this->Coefficients;

    // get the dependent variable values
    xs = this->PiecewiseFunction->GetDataPointer () + 1;
    for (int j = 0; j < size; j++)
    {
      *(dependent + j) = *(xs + 2*j);
    }

    for (int k = 0; k < size-1; k++)
    {
      *(coefficients + 3*k) = dependent[k]; // d
      *(coefficients + 3*k+1) = 3*(dependent[k+1]-dependent[k]); // b
      *(coefficients + 3*k+2) = -2*(dependent[k+1]-dependent[k]); // a
    }
    *(coefficients + 3*(size-1)) = dependent[size-1];
    *(coefficients + 3*(size-1)+1) = dependent[size-1];
    *(coefficients + 3*(size-1)+2) = dependent[size-1];
  }

  else //add extra "fictitious" point to close loop
  {
    size = size + 1;
    this->Intervals = new double[size];
    ts = this->PiecewiseFunction->GetDataPointer ();
    for (i = 0; i < size-1; i++)
    {
      this->Intervals[i] = *(ts + 2*i);
    }
    if ( this->ParametricRange[0] != this->ParametricRange[1] )
    {
      this->Intervals[size-1] = this->ParametricRange[1];
    }
    else
    {
      this->Intervals[size-1] = this->Intervals[size-2] + 1.0;
    }

    // allocate memory for work arrays
    //    work = new double[size];

    // allocate memory for coefficients
    delete [] this->Coefficients;
    //this->Coefficients = new double [4*size];
    this->Coefficients = new double [3*size];

    // allocate memory for dependent variables
    dependent = new double [size];

    // get start of coefficients for this dependent variable
    coefficients = this->Coefficients;

    // get the dependent variable values
    xs = this->PiecewiseFunction->GetDataPointer () + 1;
    for (int j = 0; j < size-1; j++)
    {
      *(dependent + j) = *(xs + 2*j);
    }
    dependent[size-1] = *xs;

    for (int k = 0; k < size-1; k++)
    {
      *(coefficients + 3*k) = dependent[k]; // d
      *(coefficients + 3*k+1) = 3*(dependent[k+1]-dependent[k]); // b
      *(coefficients + 3*k+2) = -2*(dependent[k+1]-dependent[k]); // a
    }
    *(coefficients + 3*(size-1)) = dependent[size-1];
    *(coefficients + 3*(size-1)+1) = dependent[size-1];
    *(coefficients + 3*(size-1)+2) = dependent[size-1];
  }

  // free the work array and dependent variable storage
  //delete [] work;
  delete [] dependent;

  // update compute time
  this->ComputeTime = this->GetMTime();
}

//----------------------------------------------------------------------------
void vtkSCurveSpline::DeepCopy(vtkSpline *s)
{
  vtkSCurveSpline *spline = vtkSCurveSpline::SafeDownCast(s);

  if ( spline != NULL )
  {
    //nothing to do
  }

  // Now do superclass
  this->vtkSpline::DeepCopy(s);
}

//----------------------------------------------------------------------------
void vtkSCurveSpline::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << "NodeWeight: " << this->NodeWeight << endl;
}
