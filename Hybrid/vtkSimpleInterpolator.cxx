/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSimpleInterpolator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSimpleInterpolator.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkSimpleInterpolator, "1.2");
vtkStandardNewMacro(vtkSimpleInterpolator);

//----------------------------------------------------------------------------
vtkSimpleInterpolator::vtkSimpleInterpolator ()
{
  this->ComputeTime     = 0;
  this->ArraySize       = 0;
  this->Tvalues         = NULL;
  this->Dvalues         = NULL;
  this->Coefficients    = NULL;
  this->LeftConstraint  = 1;
  this->LeftValue       = 0.0;
  this->RightConstraint = 1;
  this->RightValue      = 0.0;
}
//----------------------------------------------------------------------------
void vtkSimpleInterpolator::SetArrays(int Size, double *Tarray, double *Varray, double *Warray, double *Coeffs)
{
  this->Tvalues      = Tarray;
  this->Dvalues      = Varray;
  this->ArraySize    = Size;
  this->Work         = Warray;
  this->Coefficients = Coeffs;
  this->Modified();
}
//----------------------------------------------------------------------------
double vtkSimpleInterpolator::EvaluateLinear(double t)
{
  double deltaT = (t-this->Tvalues[0])/(this->Tvalues[1]-this->Tvalues[0]);
  return this->Dvalues[0] + deltaT*(this->Dvalues[1]-this->Dvalues[0]);
}
//----------------------------------------------------------------------------
double vtkSimpleInterpolator::EvaluateSpline(double t)
{
  // make sure we have at least 2 points
  if (this->ArraySize < 2)
    {
    return 0.0;
    }

  // check to see if we need to recompute the spline
  if (this->ComputeTime < this->GetMTime ())
  {
    this->Fit1DSpline(this->ArraySize, this->Tvalues, this->Dvalues,
                 this->Work, (double (*)[4])this->Coefficients,
                 this->LeftConstraint, this->LeftValue,
                 this->RightConstraint, this->RightValue);

    // update compute time
    this->ComputeTime = this->GetMTime();
  }

  // clamp the function at both ends
  if (t < this->Tvalues[0])
    {
    t = this->Tvalues[0];
    }
  if (t > this->Tvalues[this->ArraySize - 1])
    {
    t = this->Tvalues[this->ArraySize - 1];
    }

  // find pointer to cubic spline coefficient using bisection method
  int index = this->FindIndex(this->ArraySize,t);

  // calculate offset within interval
  t = (t - this->Tvalues[index]);

  // evaluate intervals value y
  return (t * (t * (t * *(this->Coefficients + index * 4 + 3)
                      + *(this->Coefficients + index * 4 + 2))
                      + *(this->Coefficients + index * 4 + 1))
                      + *(this->Coefficients + index * 4));
}

//----------------------------------------------------------------------------
// Compute the coeffs for a 1D spline. The spline is open.
void vtkSimpleInterpolator::Fit1DSpline (int size, double *x, double *y,
                        double *work, double coeffs[][4],
                        int leftConstraint, double leftValue,
                        int rightConstraint, double rightValue)
{
  double   b = 0.0;
  double   xlk;
  double   xlkp;
  int      k;

  // develop constraint at leftmost point.
  switch (leftConstraint) 
    {
    case 0:
      // desired slope at leftmost point is derivative from two points
      coeffs[0][1] = 1.0;
      coeffs[0][2] = 0.0;
      work[0] = this->ComputeLeftDerivative();
      break;
    case 1:
      // desired slope at leftmost point is leftValue.
      coeffs[0][1] = 1.0;
      coeffs[0][2] = 0.0;
      work[0] = leftValue;
      break;
    case 2:
      // desired second derivative at leftmost point is leftValue.
      coeffs[0][1] = 2.0;
      coeffs[0][2] = 1.0;
      work[0]= 3.0 * ((y[1] - y[0]) / (x[1] - x[0])) -
         0.5 * (x[1] - x[0]) * leftValue;
      break;
    case 3:
      // desired second derivative at leftmost point is
      // leftValue times second derivative at first interior point.
      coeffs[0][1] = 2.0;
      coeffs[0][2] = 4.0 * ((0.5 + leftValue) / 
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
    coeffs[k][0] = xlkp;
    coeffs[k][1] = 2.0 * (xlkp + xlk);
    coeffs[k][2] = xlk;
    work[k] = 3.0 * (((xlkp * (y[k] - y[k-1])) / xlk) +
                     ((xlk * (y[k+1] - y[k])) / xlkp));
    }


  // develop constraint at rightmost point.
  switch (rightConstraint) 
    {
    case 0:
      // desired slope at leftmost point is derivative from two points
      coeffs[size - 1][0] = 0.0;
      coeffs[size - 1][1] = 1.0;
      work[size - 1] = this->ComputeRightDerivative();
      break;
    case 1:
      // desired slope at rightmost point is rightValue
      coeffs[size - 1][0] = 0.0;
      coeffs[size - 1][1] = 1.0;
      work[size - 1] = rightValue;
      break;
    case 2:
      // desired second derivative at rightmost point is rightValue.
      coeffs[size-1][0] = 1.0;
      coeffs[size-1][1] = 2.0;
      work[size-1] = 3.0 * ((y[size-1] - y[size-2]) / 
                            (x[size-1] - x[size-2])) +
        0.5 * (x[size-1]-x[size-2]) * rightValue;
      break;
    case 3:
      // desired second derivative at rightmost point is
      // rightValue times second derivative at last interior point.
      coeffs[size-1][0] = 4.0 * ((0.5 + rightValue) /
                                       (2.0 + rightValue));
      coeffs[size-1][1] = 2.0;
      work[size-1] = 6.0 * ((1.0 + rightValue) / (2.0 + rightValue)) *
        ((y[size-1] - y[size-2]) /
         (x[size-1] - x[size-2]));
      break;
    }

  // solve resulting set of equations.
  coeffs[0][2] = coeffs[0][2] / coeffs[0][1];
  work[0] = work[0] / coeffs[0][1];
  coeffs[size-1][2] = 0.0;

  for (k = 1; k < size; k++)
    {
    coeffs[k][1] = coeffs[k][1] - (coeffs[k][0] *
                                               coeffs[k-1][2]);
    coeffs[k][2] = coeffs[k][2] / coeffs[k][1];
    work[k]  = (work[k] - (coeffs[k][0] * work[k-1]))
      / coeffs[k][1];
    }

  for (k = size - 2; k >= 0; k--)
    {
    work[k] = work[k] - (coeffs[k][2] * work[k+1]);
    }

  // the column vector work now contains the first
  // derivative of the spline function at each joint.
  // compute the coeffs of the cubic between
  // each pair of joints.
  for (k = 0; k < size - 1; k++)
    {
    b = x[k+1] - x[k];
    coeffs[k][0] = y[k];
    coeffs[k][1] = work[k];
    coeffs[k][2] = (3.0 * (y[k+1] - y[k])) / (b * b) - 
      (work[k+1] + 2.0 * work[k]) / b;
    coeffs[k][3] = (2.0 * (y[k] - y[k+1])) / (b * b * b) + 
      (work[k+1] + work[k]) / (b * b);
    }

  // the coeffs of a fictitious nth cubic
  // are evaluated.  This may simplify
  // algorithms which include both end points.

  coeffs[size-1][0] = y[size-1];
  coeffs[size-1][1] = work[size-1];
  coeffs[size-1][2] = coeffs[size-2][2] + 
    3.0 * coeffs[size-2][3] * b;
  coeffs[size-1][3] = coeffs[size-2][3];

}
//----------------------------------------------------------------------------
int vtkSimpleInterpolator::FindIndex(int size, double t)
{
  int index=0;
  if ( size > 2 ) //bisection method for speed
    {
    int rightIdx = size - 1;
    int centerIdx = rightIdx - size/2;
    for (int converged=0; !converged; )
      {
      if ( this->Tvalues[index] <= t && t <= this->Tvalues[centerIdx] )
        {
        rightIdx = centerIdx;
        }
      else // if ( this->Tvalues[centerIdx] < t && t <= this->Tvalues[rightIdx] )
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
      } // while not converged
    }
  return index;
}
//----------------------------------------------------------------------------
double vtkSimpleInterpolator::ComputeLeftDerivative()
{
  if (this->ArraySize < 2)
    {
    return 0.0;
    }
  else
    {
    return (this->Tvalues[1]-this->Tvalues[0]);
    }
}
//----------------------------------------------------------------------------
double vtkSimpleInterpolator::ComputeRightDerivative()
{
  if (this->ArraySize < 2)
    {
    return 0.0;
    }
  else
    {
    return (this->Tvalues[this->ArraySize-1]-this->Tvalues[this->ArraySize-2]);
    }
}
//----------------------------------------------------------------------------
void vtkSimpleInterpolator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "LeftConstraint: "
     << LeftConstraint;
  os << indent << "RightConstraint: "
     << RightConstraint;
  os << indent << "LeftValue: "
     << LeftValue;
  os << indent << "RightValue: "
     << RightValue;
}
