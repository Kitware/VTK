/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKochanekSpline.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#include "vtkKochanekSpline.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkKochanekSpline* vtkKochanekSpline::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkKochanekSpline");
  if(ret)
    {
    return (vtkKochanekSpline*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkKochanekSpline;
}




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

// Evaluate a 1D Spline
float vtkKochanekSpline::Evaluate (float t)
{
  int i, index = 0;
  int size = this->PiecewiseFunction->GetSize ();
  float *intervals;
  float *coefficients;

  // make sure we have at least 2 points
  if (size < 2)
    {
    vtkErrorMacro("Cannot evaluate a spline with less than 2 points. # of points is: " << size);
    return 0.0;
    }

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
  t = (t - intervals[index]) / (intervals[index+1] - intervals[index]);

  // evaluate y
  return (t * (t * (t * *(coefficients + index * 4 + 3)
                      + *(coefficients + index * 4 + 2))
                      + *(coefficients + index * 4 + 1))
                      + *(coefficients + index * 4));
}

// Compute Kochanek Spline coefficients.
void vtkKochanekSpline::Compute ()
{
  float *ts, *xs;
  float *coefficients;
  float *dependent;
  int size;
  int i;

  // get the size of the independent variables
  size = this->PiecewiseFunction->GetSize ();

  if ( !this->Closed )
    {
    // copy the independent variables
    if (this->Intervals)
      {
      delete [] this->Intervals;
      }
    this->Intervals = new float[size];
    ts = this->PiecewiseFunction->GetDataPointer ();  
    for (i = 0; i < size; i++)
      {
      this->Intervals[i] = *(ts + 2 * i);    
      }

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
    }
  else //spline is closed, create extra "fictitious" point
    {
    size = size + 1;
    // copy the independent variables
    if (this->Intervals)
      {
      delete [] this->Intervals;
      }
    this->Intervals = new float[size];
    ts = this->PiecewiseFunction->GetDataPointer ();  
    for (i = 0; i < size-1; i++)
      {
      this->Intervals[i] = *(ts + 2 * i);    
      }
    this->Intervals[size-1] = this->Intervals[size-2] + 1.0;

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
    }

  this->Fit1D (size, this->Intervals, dependent,
		 this->DefaultTension,
		 this->DefaultBias,
		 this->DefaultContinuity,
		 (float (*)[4])coefficients,
		 this->LeftConstraint, this->LeftValue,
		 this->RightConstraint, this->RightValue);

  // free the dependent variable storage
  delete [] dependent;

  // update compute time
  this->ComputeTime = this->GetMTime();
}

#define VTK_EPSILON .0001

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
    // two points, set coefficients for a straight line
    coefficients[0][3] = 0.0;
    coefficients[1][3] = 0.0;
    coefficients[0][2] = 0.0;
    coefficients[1][2] = 0.0;
    coefficients[0][1] = (y[1] - y[0]) / (x[1] - x[0]);
    coefficients[1][1] = coefficients[0][1];
    coefficients[0][0] = y[0];
    coefficients[1][0] = y[1];
    return;
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

    ds *= (2 * n0 / (n0 + n1));
    dd *= (2 * n1 / (n0 + n1));

    coefficients[i][0] = y[i];
    coefficients[i][1] = dd;
    coefficients[i][2] = ds;
    }

  // Calculate the deriviatives at the end points
  coefficients[0][0] = y[0];
  coefficients[N][0] = y[N];

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
    ds *= (2 * n1 / (n0 + n1));
    dd *= (2 * n0 / (n0 + n1));

    coefficients[0][1] = dd;
    coefficients[0][2] = ds;
    coefficients[N][1] = dd;
    coefficients[N][2] = ds;
    }
  else //curve is open
    {
    switch (leftConstraint) 
      {
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

