/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCardinalSpline.cxx
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

#include "vtkCardinalSpline.h"

// Construct a Cardinal Spline.
vtkCardinalSpline::vtkCardinalSpline ()
{
}

// Evaluate a 1D Spline
float vtkCardinalSpline::Evaluate (float t)
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

  // find pointer to cubic spline coefficient
  for (i = 1; i < size; i++)
    {
    index = i - 1;
    if (t < intervals[i])
      {
      break;
      }
    }

  // calculate offset within interval
  t = (t - intervals[index]);

  // evaluate y
  return (t * (t * (t * *(coefficients + index * 4 + 3)
                      + *(coefficients + index * 4 + 2))
                      + *(coefficients + index * 4 + 1))
                      + *(coefficients + index * 4));
}

// Compute Cardinal Splines for each dependent variable
void vtkCardinalSpline::Compute ()
{
  float *ts, *xs;
  float *work;
  float *coefficients;
  float *dependent;
  int size;
  int i;

  // get the size of the independent variables
  size = this->PiecewiseFunction->GetSize ();

  // copy the independent variables. Note that if the spline
  // is closed the first and last point are assumed repeated -
  // so we add and extra point
  if (this->Intervals)
    {
    delete [] this->Intervals;
    }

  if ( !this->Closed )
    {
    this->Intervals = new float[size];
    ts = this->PiecewiseFunction->GetDataPointer ();  
    for (i = 0; i < size; i++)
      {
      this->Intervals[i] = *(ts + 2 * i);    
      }

    // allocate memory for work arrays
    work = new float[size];

    // allocate memory for coefficients
    if (this->Coefficients)
      {
      delete [] this->Coefficients;
      }
    this->Coefficients = new float [4 * size];

    // allocate memory for dependent variables
    dependent = new float [size];

    // get start of coefficients for this dependent variable
    coefficients = this->Coefficients;

    // get the dependent variable values
    xs = this->PiecewiseFunction->GetDataPointer () + 1;
    for (int j = 0; j < size; j++)
      {
      *(dependent + j) = *(xs + 2*j);
      }

    this->Fit1D (size, this->Intervals, dependent,
		 work, (float (*)[4])coefficients,
		 this->LeftConstraint, this->LeftValue,
		 this->RightConstraint, this->RightValue);
    }

  else //add extra "fictitious" point to close loop
    {
    size = size + 1;
    this->Intervals = new float[size];
    ts = this->PiecewiseFunction->GetDataPointer ();  
    for (i = 0; i < size-1; i++)
      {
      this->Intervals[i] = *(ts + 2 * i);    
      }
    this->Intervals[size-1] = this->Intervals[size-2] + 1.0;

    // allocate memory for work arrays
    work = new float[size];

    // allocate memory for coefficients
    if (this->Coefficients)
      {
      delete [] this->Coefficients;
      }
    this->Coefficients = new float [4 * size];

    // allocate memory for dependent variables
    dependent = new float [size];

    // get start of coefficients for this dependent variable
    coefficients = this->Coefficients;

    // get the dependent variable values
    xs = this->PiecewiseFunction->GetDataPointer () + 1;
    for (int j = 0; j < size-1; j++)
      {
      *(dependent + j) = *(xs + 2*j);
      }
    dependent[size-1] = *xs;

    this->FitClosed1D (size, this->Intervals, dependent,
		 work, (float (*)[4])coefficients);
    }

  // free the work array and dependent variable storage
  delete [] work;
  delete [] dependent;
}


// Compute the coefficients for a 1D spline. The spline is open.
void vtkCardinalSpline::Fit1D (int size, float *x, float *y,
			float *work, float coefficients[][4],
			int leftConstraint, float leftValue,
			int rightConstraint, float rightValue)
{
  float   b;
  float   xlk;
  float   xlkp;
  int     k;

  // develop constraint at leftmost point.
  switch (leftConstraint) 
    {
    case 1:
      // desired slope at leftmost point is leftValue.
      coefficients[0][1] = 1.0;
      coefficients[0][2] = 0.0;
      work[0] = leftValue;
      break;
    case 2:
      // desired second derivative at leftmost point is leftValue.
      coefficients[0][1] = 2.0;
      coefficients[0][2] = 1.0;
      work[0]= 3.0 * ((y[1] - y[0]) / (x[1] - x[0])) -
	 0.5 * (x[1] - x[0]) * leftValue;
      break;
    case 3:
      // desired second derivative at leftmost point is
      // leftValue times second derivative at first interior point.
      coefficients[0][1] = 2.0;
      coefficients[0][2] = 4.0 * ((0.5 + leftValue) / 
				(2.0 + leftValue));
      work[0]= 6.0 * ((1.0 + leftValue) / (2.0 + leftValue)) *
		 ((y[1] - y[0]) / (x[1]-x[0]));

      break;
    }

    // develop body of band matrix.
    for (k = 1; k < size - 1; k++)
      {
      xlk = x[k] - x[k-1];
      xlkp = x[k+1] - x[k];
      coefficients[k][0] = xlkp;
      coefficients[k][1] = 2.0 * (xlkp + xlk);
      coefficients[k][2] = xlk;
      work[k] = 3.0 * (((xlkp * (y[k] - y[k-1])) / xlk) +
		((xlk * (y[k+1] - y[k])) / xlkp));
      }

    // develop constraint at rightmost point.
    switch (rightConstraint) 
      {
      case 1:
	// desired slope at rightmost point is rightValue
	coefficients[size - 1][0] = 0.0;
	coefficients[size - 1][1] = 1.0;
	work[size - 1] = rightValue;
	break;
      case 2:
	// desired second derivative at rightmost point is rightValue.
	coefficients[size-1][0] = 1.0;
	coefficients[size-1][1] = 2.0;
	work[size-1] = 3.0 * ((y[size-1] - y[size-2]) / 
	  (x[size-1] - x[size-2])) +
	  0.5 * (x[size-1]-x[size-2]) * rightValue;
	break;
      case 3:
	// desired second derivative at rightmost point is
	// rightValue times second derivative at last interior point.
	coefficients[size-1][0] = 4.0 * ((0.5 + rightValue) /
				   (2.0 + rightValue));
	coefficients[size-1][1] = 2.0;
	work[size-1] = 6.0 * ((1.0 + rightValue) / (2.0 + rightValue)) *
		  ((y[size-1] - y[size-2]) /
		   (x[size-1] - x[size-2]));
	break;
      }

    // solve resulting set of equations.
    coefficients[0][2] = coefficients[0][2] / coefficients[0][1];
    work[0] = work[0] / coefficients[0][1];
    coefficients[size-1][2] = 0.0;

    for (k = 1; k < size; k++)
      {
      coefficients[k][1] = coefficients[k][1] - (coefficients[k][0] *
			 coefficients[k-1][2]);
      coefficients[k][2] = coefficients[k][2] / coefficients[k][1];
      work[k]  = (work[k] - (coefficients[k][0] * work[k-1]))
			   / coefficients[k][1];
      }

    for (k = size - 2; k >= 0; k--)
      {
      work[k] = work[k] - (coefficients[k][2] * work[k+1]);
      }

    // the column vector work now contains the first
    // derivative of the spline function at each joint.
    // compute the coefficients of the cubic between
    // each pair of joints.
    for (k = 0; k < size - 1; k++)
      {
      b = x[k+1] - x[k];
      coefficients[k][0] = y[k];
      coefficients[k][1] = work[k];
      coefficients[k][2] = (3.0 * (y[k+1] - y[k])) / (b * b) - 
			(work[k+1] + 2.0 * work[k]) / b;
      coefficients[k][3] = (2.0 * (y[k] - y[k+1])) / (b * b * b) + 
			(work[k+1] + work[k]) / (b * b);
      }

    // the coefficients of a fictitious nth cubic
    // are evaluated.  This may simplify
    // algorithms which include both end points.

    coefficients[size-1][0] = y[size-1];
    coefficients[size-1][1] = work[size-1];
    coefficients[size-1][2] = coefficients[size-2][2] + 
		3.0 * coefficients[size-2][3] * b;
    coefficients[size-1][3] = coefficients[size-2][3];

}

// Compute the coefficients for a 1D spline. The spline is closed
// (i.e., the first and last point are assumed the same) and the
// spline is continous in value and derivatives.
void vtkCardinalSpline::FitClosed1D (int size, float *x, float *y,
			float *work, float coefficients[][4])
{
  float   b;
  float   xlk;
  float   xlkp;
  int     k;
  float	aN, bN, cN, dN;
  int	N;

  N = size - 1;

  // develop body of band matrix.
  //
  for (k = 1; k < N; k++) 
    {
    xlk = x[k] - x[k-1];
    xlkp = x[k+1] - x[k];
    coefficients[k][0] = xlkp;
    coefficients[k][1] = 2.0 * (xlkp + xlk);
    coefficients[k][2] = xlk;
    work[k] = 3.0 * (((xlkp * (y[k] - y[k-1])) / xlk) +
		     ((xlk * (y[k+1] - y[k])) / xlkp));
    }

  xlk = x[N] - x[N-1];
  xlkp = x[1] - x[0];
  aN = coefficients[N][0] = xlkp;
  bN = coefficients[N][1] = 2.0 * (xlkp + xlk);
  cN = coefficients[N][2] = xlk;
  dN = work[N] = 3.0 * (((xlkp * (y[N] - y[N-1])) / xlk) +
			((xlk * (y[1] - y[0])) / xlkp));


  // solve resulting set of equations.
  //
  coefficients[0][2] = 0.0;
  work[0]            = 0.0;
  coefficients[0][3] = 1.0;

  for (k = 1; k <= N; k++) 
    {
    coefficients[k][1] = coefficients[k][1] - 
      (coefficients[k][0] * coefficients[k-1][2]);
    coefficients[k][2] = coefficients[k][2] / coefficients[k][1];
    work[k]  = (work[k] - (coefficients[k][0] * work[k-1]))
      / coefficients[k][1];
    coefficients[k][3] = (-1.0 * coefficients[k][0] * 
			  coefficients[k-1][3]) / coefficients[k][1];
    }

  coefficients[N][0] = 1.0;
  coefficients[N][1] = 0.0;

  for (k = N - 1; k > 0; k--) 
    {
    coefficients[k][0] = coefficients[k][3] - 
      coefficients[k][2] * coefficients[k+1][0];
    coefficients[k][1] = work[k] - coefficients[k][2] * coefficients[k+1][1];
    }

  work[0] = work[N] = (dN - cN * coefficients[1][1] - 
		       aN * coefficients[N-1][1]) / ( bN +
						      cN * coefficients[1][0] + aN * coefficients[N-1][0]);

  for (k=1; k < N; k++) 
    {
    work[k] = coefficients[k][0] * work[N] + coefficients[k][1];
    }

  // the column vector work now contains the first
  // derivative of the spline function at each joint.
  // compute the coefficients of the cubic between
  // each pair of joints.
  for (k = 0; k < N; k++) 
    {
    b = x[k+1] - x[k];
    coefficients[k][0] = y[k];
    coefficients[k][1] = work[k];
    coefficients[k][2] = (3.0 * (y[k+1] - y[k])) / (b * b) - 
      (work[k+1] + 2.0 * work[k]) / b;
    coefficients[k][3] = (2.0 * (y[k] - y[k+1])) / (b * b * b) + 
      (work[k+1] + work[k]) / (b * b);
    }


  // the coefficients of a fictitious nth cubic
  // are the same as the coefficients in the first interval
  //
  coefficients[N][0] = y[N];
  coefficients[N][1] = work[N];
  coefficients[N][2] = coefficients[0][2];
  coefficients[N][3] = coefficients[0][3];
}

void vtkCardinalSpline::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkSpline::PrintSelf(os,indent);
}

