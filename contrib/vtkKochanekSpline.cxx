/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKochanekSpline.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

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

#include "vtkKochanekSpline.h"

// Description
// Construct a KochanekSpline wth the following defaults:
// DefaultBias = 0
// DefaultTension = 0
// DefaultContinuity = 0
vtkKochanekSpline::vtkKochanekSpline ()
{
  this->DefaultBias = 0.0;
  this->DefaultTension = 0.0;
  this->DefaultContinuity = 0.0;
}

// Description
// Compute Kochanek Splines for each dependent variable
void vtkKochanekSpline::Compute ()
{
  float *ts, *xs;
  float *coefficients;
  float *dependent;
  int size;
  int i;

  // get the size of the independent variables
  size = this->PiecewiseFunction->GetSize ();

  // copy the independent variables
  if (this->Intervals) delete this->Intervals;
  this->Intervals = new float[size];
  ts = this->PiecewiseFunction->GetDataPointer ();  
  for (i = 0; i < size; i++)
    {
    this->Intervals[i] = *(ts + 2 * i);    
    }

  // allocate memory for coefficients
  if (this->Coefficients) delete this->Coefficients;
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
		 this->DefaultTension,
		 this->DefaultBias,
		 this->DefaultContinuity,
		 (float (*)[4])coefficients,
		 this->LeftConstraint, this->LeftValue,
		 this->RightConstraint, this->RightValue);

  // free the dependent variable storage
  delete dependent;

  // update compute time
  this->ComputeTime = this->GetMTime();
}

#define EPSILON .0001

// Description
// Compute the coefficients for a 1D spline
void vtkKochanekSpline::Fit1D (int size, float *x, float *y,
                        float tension, float bias, float continuity,
			float coefficients[][4],
			int leftConstraint, float leftValue,
			int rightConstraint, float rightValue)
{
  float		cs;		/* source chord			*/
  float		cd;		/* destination chord		*/
  float		ds;		/* source deriviative		*/
  float		dd;		/* destination deriviative	*/
  float		n0, n1;		/* number of frames btwn nodes	*/
  int		N;		/* top point number		*/
  int		i;

  if (size == 2)
    {
    // two points, set coeffieints for a straight line
  	coefficients[0][3] = 0.0;
  	coefficients[1][3] = 0.0;
  	coefficients[0][2] = 0.0;
  	coefficients[1][2] = 0.0;
  	coefficients[0][1] = (y[1] - y[0]) / (x[1] - x[0]);
  	coefficients[1][1] = coefficients[0][1];
  	coefficients[0][0] = y[0];
  	coefficients[1][0] = y[1];
    }

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
    ds *= (2 * n1 / (n0 + n1));
    dd *= (2 * n0 / (n0 + n1));

    coefficients[i][0] = y[i];
    coefficients[i][1] = dd;
    coefficients[i][2] = ds;
    }

  // Calculate the deriviatives at the end points
  coefficients[0][0] = y[0];
  coefficients[N][0] = y[N];

  switch (leftConstraint) {
    case 1:
      // desired slope at leftmost point is leftValue
      coefficients[0][1] = leftValue;
      break;

    case 2:
      // desired secord derivative at leftmost point is leftValue
      coefficients[0][1] = (6*(y[1] - y[0]) - 2*coefficients[1][2] - leftValue)
  	    / 4.0;
      break;

    case 3:
      // desired secord derivative at leftmost point is leftValue
      // times secod derivative at first interior point
      if ((leftValue > (-2.0 + EPSILON)) || 
  	  (leftValue < (-2.0 - EPSILON)))
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
      if ((rightValue > (-2.0 + EPSILON)) ||
          (rightValue < (-2.0 - EPSILON)))
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

  // Compute the Coefficients
  for (i=0; i < N; i++) {

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

void vtkKochanekSpline::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkSpline::PrintSelf(os,indent);
  os << indent << "DefaultBias: " << this->DefaultBias << "\n";
  os << indent << "DefaultTension: " << this->DefaultTension << "\n";
  os << indent << "DefaultContinuity: " << this->DefaultContinuity << "\n";
}

