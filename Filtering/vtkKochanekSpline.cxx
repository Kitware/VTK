/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKochanekSpline.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKochanekSpline.h"

#include "vtkObjectFactory.h"
#include "vtkPiecewiseFunction.h"

vtkStandardNewMacro(vtkKochanekSpline);

//----------------------------------------------------------------------------
// Construct a KochanekSpline wth the following defaults:
// DefaultBias = 0,
// DefaultTension = 0,
// DefaultContinuity = 0.
vtkKochanekSpline::vtkKochanekSpline ()
{
  this->DefaultBias = 0.0;
  this->DefaultTension = 0.0;
  this->DefaultContinuity = 0.0;
}

//----------------------------------------------------------------------------
// Evaluate a 1D Spline
double vtkKochanekSpline::Evaluate (double t)
{
  int index = 0;
  double *intervals;
  double *coefficients;

  // check to see if we need to recompute the spline
  if (this->ComputeTime < this->GetMTime ())
    {
    this->Compute ();
    }

  // make sure we have at least 2 points
  int size = this->PiecewiseFunction->GetSize ();
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

  // find pointer to cubic spline coefficient
  index = this->FindIndex(size,t);

  // calculate offset within interval
  t = (t - intervals[index]) / (intervals[index+1] - intervals[index]);

  // evaluate y
  return (t * (t * (t * *(coefficients + index * 4 + 3)
                      + *(coefficients + index * 4 + 2))
                      + *(coefficients + index * 4 + 1))
                      + *(coefficients + index * 4));
}

//----------------------------------------------------------------------------
// Compute Kochanek Spline coefficients.
void vtkKochanekSpline::Compute ()
{
  double *ts, *xs;
  double *coefficients;
  double *dependent;
  int size;
  int i;

  // Make sure the function is up to date.
  this->PiecewiseFunction->Update();

  // get the size of the independent variables
  size = this->PiecewiseFunction->GetSize ();

  if(size < 2)
    {
    vtkErrorMacro("Spline requires at least 2 points. # of points is: " <<size);
    return;
    }

  if ( !this->Closed )
    {
    // copy the independent variables
    if (this->Intervals)
      {
      delete [] this->Intervals;
      }
    this->Intervals = new double[size];
    ts = this->PiecewiseFunction->GetDataPointer ();  
    for (i = 0; i < size; i++)
      {
      this->Intervals[i] = *(ts + 2*i);    
      }

    // allocate memory for coefficients
    if (this->Coefficients)
      {
      delete [] this->Coefficients;
      }
    this->Coefficients = new double [4*size];

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
    }
  else //spline is closed, create extra "fictitious" point
    {
    size = size + 1;
    // copy the independent variables
    if (this->Intervals)
      {
      delete [] this->Intervals;
      }
    this->Intervals = new double[size];
    ts = this->PiecewiseFunction->GetDataPointer ();  
    for (i = 0; i < size-1; i++)
      {
      this->Intervals[i] = *(ts + 2 * i);    
      }
    if ( this->ParametricRange[0] != this->ParametricRange[1] )
      {
      this->Intervals[size-1] = this->ParametricRange[1];
      }
    else
      {
      this->Intervals[size-1] = this->Intervals[size-2] + 1.0;
      }

    // allocate memory for coefficients
    if (this->Coefficients)
      {
      delete [] this->Coefficients;
      }
    this->Coefficients = new double [4 * size];

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
    }

  this->Fit1D (size, this->Intervals, dependent,
                 this->DefaultTension,
                 this->DefaultBias,
                 this->DefaultContinuity,
                 (double (*)[4])coefficients,
                 this->LeftConstraint, this->LeftValue,
                 this->RightConstraint, this->RightValue);

  // free the dependent variable storage
  delete [] dependent;

  // update compute time
  this->ComputeTime = this->GetMTime();
}

#define VTK_EPSILON .0001

//----------------------------------------------------------------------------
// Compute the coefficients for a 1D spline
void vtkKochanekSpline::Fit1D (int size, double *x, double *y,
                               double tension, double bias, double continuity,
                               double coefficients[][4],
                               int leftConstraint, double leftValue,
                               int rightConstraint, double rightValue)
{
  double        cs;             /* source chord                 */
  double        cd;             /* destination chord            */
  double        ds;             /* source deriviative           */
  double        dd;             /* destination deriviative      */
  double        n0, n1;         /* number of frames btwn nodes  */
  int           N;              /* top point number             */
  int           i;

  N = size - 1;

  for (i=1; i < N; i++)
    {
    cs = y[i] - y[i-1];
    cd = y[i+1] - y[i];

    ds = cs*((1 - tension)*(1 - continuity)*(1 + bias)) / 2.0 
       + cd*((1 - tension)*(1 + continuity)*(1 - bias)) / 2.0;

    dd = cs*((1 - tension)*(1 + continuity)*(1 + bias)) / 2.0
       + cd*((1 - tension)*(1 - continuity)*(1 - bias)) / 2.0;

    // adjust deriviatives for non uniform spacing between nodes
    n1  = x[i+1] - x[i];
    n0  = x[i] - x[i-1];

    ds *= (2 * n0 / (n0 + n1));
    dd *= (2 * n1 / (n0 + n1));

    coefficients[i][0] = y[i];
    coefficients[i][1] = dd;
    coefficients[i][2] = ds;
    }

  // Calculate the deriviatives at the end points
  coefficients[0][0] = y[0];
  coefficients[N][0] = y[N];
  coefficients[N][1] = 0.0;
  coefficients[N][2] = 0.0;
  coefficients[N][3] = 0.0;

  if ( this->Closed ) //the curve is continuous and closed at P0=Pn
    {
    cs = y[N] - y[N-1];
    cd = y[1] - y[0];

    ds = cs*((1 - tension)*(1 - continuity)*(1 + bias)) / 2.0 
       + cd*((1 - tension)*(1 + continuity)*(1 - bias)) / 2.0;

    dd = cs*((1 - tension)*(1 + continuity)*(1 + bias)) / 2.0
       + cd*((1 - tension)*(1 - continuity)*(1 - bias)) / 2.0;

    // adjust deriviatives for non uniform spacing between nodes
    n1  = x[1] - x[0];
    n0  = x[N] - x[N-1];
    ds *= (2 * n0 / (n0 + n1));
    dd *= (2 * n1 / (n0 + n1));

    coefficients[0][1] = dd;
    coefficients[0][2] = ds;
    coefficients[N][1] = dd;
    coefficients[N][2] = ds;
    }
  else //curve is open
    {
    switch (leftConstraint) 
      {
      case 0:
        // desired slope at leftmost point is leftValue
        coefficients[0][1] = this->ComputeLeftDerivative();
        break;

      case 1:
        // desired slope at leftmost point is leftValue
        coefficients[0][1] = leftValue;
        break;

      case 2:
        // desired second derivative at leftmost point is leftValue
        coefficients[0][1] = (6*(y[1] - y[0]) - 2*coefficients[1][2] - leftValue)
              / 4.0;
        break;

      case 3:
        // desired secord derivative at leftmost point is leftValue
        // times secod derivative at first interior point
        if ((leftValue > (-2.0 + VTK_EPSILON)) || 
            (leftValue < (-2.0 - VTK_EPSILON)))
          {
          coefficients[0][1] = (3*(1 + leftValue)*(y[1] - y[0]) -
                                    (1 + 2*leftValue)*coefficients[1][2])
                                  / (2 + leftValue);
          }
        else
          {
          coefficients[0][1] = 0.0;
          }
        break;
      }

    switch (rightConstraint)
      {
      case 0:
        // desired slope at rightmost point is rightValue
        coefficients[N][2] = this->ComputeRightDerivative();
        break;

      case 1:
        // desired slope at rightmost point is rightValue
        coefficients[N][2] = rightValue;
        break;

       case 2:
         // desired second derivative at rightmost point is rightValue
         coefficients[N][2] = (6*(y[N] - y[N-1]) - 2*coefficients[N-1][1] + 
                          rightValue) / 4.0;
         break;

      case 3:
        // desired secord derivative at rightmost point is rightValue
        // times secord derivative at last interior point
        if ((rightValue > (-2.0 + VTK_EPSILON)) ||
            (rightValue < (-2.0 - VTK_EPSILON)))
          {
          coefficients[N][2] = (3*(1 + rightValue)*(y[N] - y[N-1]) -
                                  (1 + 2*rightValue)*coefficients[N-1][1])
                         / (2 + rightValue);
          }
        else
          {
          coefficients[N][2] = 0.0;
          }
          break;
      }
    }//curve is open

  // Compute the Coefficients
  for (i=0; i < N; i++) 
    {
    //
    // c0    = P ;    c1    = DD ;
    //   i      i       i       i
    //
    // c1    = P   ;  c2    = DS   ;
    //   i+1    i+1     i+1     i+1
    //
    // c2  = -3P  + 3P    - 2DD  - DS   ;
    //   i      i     i+1      i     i+1
    //
    // c3  =  2P  - 2P    +  DD  + DS   ;
    //   i      i     i+1      i     i+1
    //
    coefficients[i][2] = (-3 * y[i])        + ( 3 * y[i+1])
                       + (-2 * coefficients[i][1]) + (-1 * coefficients[i+1][2]);
    coefficients[i][3] = ( 2 * y[i])        + (-2 * y[i+1])
                       + ( 1 * coefficients[i][1]) + ( 1 * coefficients[i+1][2]);
    }
}

//----------------------------------------------------------------------------
void vtkKochanekSpline::DeepCopy(vtkSpline *s)
{
  vtkKochanekSpline *spline = vtkKochanekSpline::SafeDownCast(s);

  if ( spline != NULL )
    {
    this->DefaultBias = spline->DefaultBias;
    this->DefaultTension = spline->DefaultTension;
    this->DefaultContinuity = spline->DefaultContinuity;
    }

  // Now do superclass
  this->vtkSpline::DeepCopy(s);
}

//----------------------------------------------------------------------------
void vtkKochanekSpline::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "DefaultBias: " << this->DefaultBias << "\n";
  os << indent << "DefaultTension: " << this->DefaultTension << "\n";
  os << indent << "DefaultContinuity: " << this->DefaultContinuity << "\n";
}

