/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWarpTransform.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to David G. Gobbi who developed this class.

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

#include "vtkWarpTransform.h"
#include "vtkMath.h"

//----------------------------------------------------------------------------
void vtkWarpTransform::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkAbstractTransform::PrintSelf(os, indent);

  os << indent << "InverseFlag: " << this->InverseFlag << "\n";
  os << indent << "InverseTolerance: " << this->InverseTolerance << "\n";
  os << indent << "InverseIterations: " << this->InverseIterations << "\n";
}

//----------------------------------------------------------------------------
vtkWarpTransform::vtkWarpTransform()
{
  this->InverseFlag = 0;
  this->InverseTolerance = 0.001;
  this->InverseIterations = 500;
}

//----------------------------------------------------------------------------
vtkWarpTransform::~vtkWarpTransform()
{
} 

//------------------------------------------------------------------------
// Check the InverseFlag, and perform a forward or reverse transform
// as appropriate.
template<class T>
static inline void vtkWarpTransformPoint(vtkWarpTransform *self, int inverse,
					 const T input[3], T output[3])
{
  if (inverse)
    {
    self->TemplateTransformInverse(input,output);
    }
  else
    {
    self->TemplateTransformPoint(input,output);
    }
}

void vtkWarpTransform::InternalTransformPoint(const float input[3],
					      float output[3])
{
  vtkWarpTransformPoint(this,this->InverseFlag,input,output);
}

void vtkWarpTransform::InternalTransformPoint(const double input[3],
					      double output[3])
{
  vtkWarpTransformPoint(this,this->InverseFlag,input,output);
}

//------------------------------------------------------------------------
// Check the InverseFlag, and set the output point and derivative as
// appropriate.
template<class T>
inline static void vtkWarpTransformDerivative(vtkWarpTransform *self,
					      int inverse,
					      const T input[3], T output[3],
					      T derivative[3][3])
{
  if (inverse)
    {
    self->TemplateTransformInverse(input,output,derivative);
    vtkMath::Invert3x3(derivative,derivative);
    }
  else
    {
    self->TemplateTransformPoint(input,output,derivative);
    }
}

void vtkWarpTransform::InternalTransformDerivative(const float input[3],
						   float output[3],
						   float derivative[3][3])
{
  vtkWarpTransformDerivative(this,this->InverseFlag,input,output,derivative);
}

void vtkWarpTransform::InternalTransformDerivative(const double input[3],
						   double output[3],
						   double derivative[3][3])
{
  vtkWarpTransformDerivative(this,this->InverseFlag,input,output,derivative);
}

//----------------------------------------------------------------------------
// We use Newton's method to iteratively invert the transformation.  
// This is actally quite robust as long as the Jacobian matrix is never
// singular.
template<class T>
static inline void vtkWarpInverseTransformPoint(vtkWarpTransform *self,
						const T point[3], 
						T output[3],
						T derivative[3][3])
{
  T inverse[3], lastInverse[3];
  T deltaP[3], deltaI[3];

  double functionValue = 0;
  double functionDerivative = 0;
  double lastFunctionValue = VTK_DOUBLE_MAX;

  double errorSquared = 0;
  double toleranceSquared = self->GetInverseTolerance();
  toleranceSquared *= toleranceSquared;

  T f = 1.0;
  T a;

  // first guess at inverse point: invert the displacement
  self->TemplateTransformPoint(point,inverse);
  
  inverse[0] -= 2*(inverse[0]-point[0]);
  inverse[1] -= 2*(inverse[1]-point[1]);
  inverse[2] -= 2*(inverse[2]-point[2]);

  lastInverse[0] = inverse[0];
  lastInverse[1] = inverse[1];
  lastInverse[2] = inverse[2];

  // do a maximum 500 iterations, usually less than 10 are required
  int n = self->GetInverseIterations();
  int i;

  for (i = 0; i < n; i++)
    {
    // put the inverse point back through the transform
    self->TemplateTransformPoint(inverse,deltaP,derivative);

    // how far off are we?
    deltaP[0] -= point[0];
    deltaP[1] -= point[1];
    deltaP[2] -= point[2];

    // get the current function value
    functionValue = (deltaP[0]*deltaP[0] +
		     deltaP[1]*deltaP[1] +
		     deltaP[2]*deltaP[2]);

    // if the function value is decreasing, do next Newton step
    // (the check on f is to ensure that we don't do too many
    // reduction steps between the Newton steps)
    if (functionValue < lastFunctionValue || f < 0.05)
      {
      // here is the critical step in Newton's method
      vtkMath::LinearSolve3x3(derivative,deltaP,deltaI);

      // get the error value in the output coord space
      errorSquared = (deltaI[0]*deltaI[0] +
		      deltaI[1]*deltaI[1] +
		      deltaI[2]*deltaI[2]);

      // break if less than tolerance in both coordinate systems
      if (errorSquared < toleranceSquared && 
	  functionValue < toleranceSquared)
	{
	break;
	}

      // save the last inverse point
      lastInverse[0] = inverse[0];
      lastInverse[1] = inverse[1];
      lastInverse[2] = inverse[2];

      // save the function value at that point
      lastFunctionValue = functionValue;

      // derivative of functionValue at last inverse point
      functionDerivative = (deltaP[0]*derivative[0][0]*deltaI[0] +
			    deltaP[1]*derivative[1][1]*deltaI[1] +
			    deltaP[2]*derivative[2][2]*deltaI[2])*2;

      // calculate new inverse point
      inverse[0] -= deltaI[0];
      inverse[1] -= deltaI[1];
      inverse[2] -= deltaI[2];

      // reset f to 1.0 
      f = 1.0;

      continue;
      }

    // the error is increasing, so take a partial step 
    // (see Numerical Recipes 9.7 for rationale, this code
    //  is a simplification of the algorithm provided there)

    // quadratic approximation to find best fractional distance
    a = -functionDerivative/(2*(functionValue - 
				lastFunctionValue -
				functionDerivative));

    // clamp to range [0.1,0.5]
    f *= (a < 0.1 ? 0.1 : (a > 0.5 ? 0.5 : a));

    // re-calculate inverse using fractional distance
    inverse[0] = lastInverse[0] - f*deltaI[0];
    inverse[1] = lastInverse[1] - f*deltaI[1];
    inverse[2] = lastInverse[2] - f*deltaI[2];
    }

  if (self->GetDebug())
    {
    vtkGenericWarningMacro(<<"Debug: In " __FILE__ ", line "<< __LINE__ <<"\n" 
               << self->GetClassName() << " (" << self 
               <<") Inverse Iterations: " << (i+1));
    }

  if (i >= n)
    {
    // didn't converge: back up to last good result
    inverse[0] = lastInverse[0];
    inverse[1] = lastInverse[1];
    inverse[2] = lastInverse[2];

    // print warning: Newton's method didn't converge
    vtkGenericWarningMacro(<<
         "Warning: In " __FILE__ ", line " << __LINE__ << "\n" << 
	 self->GetClassName() << " (" << self << ") " << 
	 "InverseTransformPoint: no convergence (" <<
	 point[0] << ", " << point[1] << ", " << point[2] << 
	 ") error = " << sqrt(errorSquared) << " after " <<
	 i << " iterations.");
    }

  output[0] = inverse[0];
  output[1] = inverse[1];
  output[2] = inverse[2];
}

void vtkWarpTransform::InverseTransformPoint(const float point[3], 
					     float output[3])
{
  float derivative[3][3];
  vtkWarpInverseTransformPoint(this, point, output, derivative);
}

void vtkWarpTransform::InverseTransformPoint(const double point[3], 
					     double output[3])
{
  double derivative[3][3];
  vtkWarpInverseTransformPoint(this, point, output, derivative);
}

//----------------------------------------------------------------------------
void vtkWarpTransform::InverseTransformDerivative(const float point[3], 
						  float output[3],
						  float derivative[3][3])
{
  vtkWarpInverseTransformPoint(this, point, output, derivative);
}

void vtkWarpTransform::InverseTransformDerivative(const double point[3], 
						  double output[3],
						  double derivative[3][3])
{
  vtkWarpInverseTransformPoint(this, point, output, derivative);
}

//----------------------------------------------------------------------------
// To invert the transformation, just set the InverseFlag.
void vtkWarpTransform::Inverse()
{
  this->InverseFlag = !this->InverseFlag;
  this->Modified();
}







