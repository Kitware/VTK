/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRungeKutta2.cxx
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

#include "vtkRungeKutta2.h"
#include "vtkObjectFactory.h"

vtkRungeKutta2::vtkRungeKutta2() 
{
}

vtkRungeKutta2::~vtkRungeKutta2() 
{
}


vtkRungeKutta2* vtkRungeKutta2::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkRungeKutta2");
  if(ret)
    {
    return (vtkRungeKutta2*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkRungeKutta2;
}


// Calculate next time step
float vtkRungeKutta2::ComputeNextStep(float* xprev, float* dxprev, 
				      float* xnext, float t, float delT)
{
  int i, numDerivs, numVals;

  if (!this->FunctionSet)
    {
    vtkErrorMacro("No derivative functions are provided!");
    return -1;
    }

  if (!this->Initialized)
    {
    vtkErrorMacro("Integrator not initialized!");
    return -1;
    }
  
  numDerivs = this->FunctionSet->GetNumberOfFunctions();
  numVals = numDerivs + 1;
  for(i=0; i<numVals-1; i++)
    {
    this->Vals[i] = xprev[i];
    }
  this->Vals[numVals-1] = t;

  // Obtain the derivatives dx_i at x_i
  if (dxprev)
    {
    for(i=0; i<numDerivs; i++)
      {
      this->Derivs[i] = dxprev[i];
      }
    }
  else if ( !this->FunctionSet->FunctionValues(this->Vals, this->Derivs) )
    {
    return -1;
    }

  // Half-step
  for(i=0; i<numVals-1; i++)
    {
    this->Vals[i] = xprev[i] + delT/2.0*this->Derivs[i];
    }
  this->Vals[numVals-1] = t + delT/2.0;

  // Obtain the derivatives at x_i + dt/2 * dx_i
  if (!this->FunctionSet->FunctionValues(this->Vals, this->Derivs))
    {
    return -1;
    }
    
  // Calculate x_i using improved values of derivatives
  for(i=0; i<numDerivs; i++)
    {
    xnext[i] = xprev[i] + delT*this->Derivs[i];
    }

  // TO DO: Should return estimated error
  return 0;
}






