/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRungeKutta4.cxx
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

#include "vtkRungeKutta4.h"
#include "vtkObjectFactory.h"

vtkRungeKutta4::vtkRungeKutta4() 
{
  for(int i=0; i<3; i++)
    {
    this->NextDerivs[i] = 0;
    }
}

vtkRungeKutta4::~vtkRungeKutta4() 
{
  for(int i=0; i<3; i++)
    {
    delete[] this->NextDerivs[i];
    this->NextDerivs[i] = 0;
    }
}


vtkRungeKutta4* vtkRungeKutta4::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkRungeKutta4");
  if(ret)
    {
    return (vtkRungeKutta4*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkRungeKutta4;
}

void vtkRungeKutta4::Initialize()
{
  this->vtkInitialValueProblemSolver::Initialize();
  if (!this->Initialized)
    {
    return;
    }
  // Allocate memory for temporary derivatives array
  for(int i=0; i<3; i++)
    {
    this->NextDerivs[i] = 
      new float[this->FunctionSet->GetNumberOfFunctions()];
    }
}
  
// For a detailed description of Runge-Kutta methods,
// see, for example, Numerical Recipes in (C/Fortran/Pascal) by
// Press et al. (Cambridge University Press) or
// Applied Numerical Analysis by C. F. Gerald and P. O. Wheatley
// (Addison Wesley)
float vtkRungeKutta4::ComputeNextStep(float* xprev, float* dxprev,
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

  //  4th order
  //  1
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

  for(i=0; i<numVals-1; i++)
    {
    this->Vals[i] = xprev[i] + delT/2.0*this->Derivs[i];
    }
  this->Vals[numVals-1] = t + delT/2.0;

  // 2
  if (!this->FunctionSet->FunctionValues(this->Vals, this->NextDerivs[0]))
    {
    return -1;
    }
    
  for(i=0; i<numVals-1; i++)
    {
    this->Vals[i] = xprev[i] + delT/2.0*this->NextDerivs[0][i];
    }
  this->Vals[numVals-1] = t + delT/2.0;

  // 3
  if (!this->FunctionSet->FunctionValues(this->Vals, this->NextDerivs[1]))
    {
    return -1;
    }

  for(i=0; i<numVals-1; i++)
    {
    this->Vals[i] = xprev[i] + delT*this->NextDerivs[1][i];
    }
  this->Vals[numVals-1] = t + delT;

  // 4
  if (!this->FunctionSet->FunctionValues(this->Vals, this->NextDerivs[2]))
    {
    return -1;
    }

  for(i=0; i<numDerivs; i++)
    {
    xnext[i] = xprev[i] + delT*(this->Derivs[i]/6.0 +
				this->NextDerivs[0][i]/3.0 +
				this->NextDerivs[1][i]/3.0 +
				this->NextDerivs[2][i]/6.0);
    }

  // TO DO: Should return estimated error
  return 0;
}

void vtkRungeKutta4::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkInitialValueProblemSolver::PrintSelf(os,indent);
  os << indent << "Runge-Kutta 4 function derivatives: " 
     << this->NextDerivs[0] << " " << this->NextDerivs[1]  << " "
     << this->NextDerivs[2] << endl;
}






