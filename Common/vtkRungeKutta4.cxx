/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRungeKutta4.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkRungeKutta4.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkRungeKutta4, "1.8");
vtkStandardNewMacro(vtkRungeKutta4);

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
int vtkRungeKutta4::ComputeNextStep(float* xprev, float* dxprev, float* xnext, 
				    float t, float& delT, float& delTActual,
				    float, float, float, float& error)
{

  int i, numDerivs, numVals;

  delTActual = delT;
  error = 0;

  if (!this->FunctionSet)
    {
    vtkErrorMacro("No derivative functions are provided!");
    return NotInitialized;
    }

  if (!this->Initialized)
    {
    vtkErrorMacro("Integrator not initialized!");
    return NotInitialized;
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
    return OutOfDomain;
    }

  for(i=0; i<numVals-1; i++)
    {
    this->Vals[i] = xprev[i] + delT/2.0*this->Derivs[i];
    }
  this->Vals[numVals-1] = t + delT/2.0;

  // 2
  if (!this->FunctionSet->FunctionValues(this->Vals, this->NextDerivs[0]))
    {
    return OutOfDomain;
    }
    
  for(i=0; i<numVals-1; i++)
    {
    this->Vals[i] = xprev[i] + delT/2.0*this->NextDerivs[0][i];
    }
  this->Vals[numVals-1] = t + delT/2.0;

  // 3
  if (!this->FunctionSet->FunctionValues(this->Vals, this->NextDerivs[1]))
    {
    return OutOfDomain;
    }

  for(i=0; i<numVals-1; i++)
    {
    this->Vals[i] = xprev[i] + delT*this->NextDerivs[1][i];
    }
  this->Vals[numVals-1] = t + delT;

  // 4
  if (!this->FunctionSet->FunctionValues(this->Vals, this->NextDerivs[2]))
    {
    return OutOfDomain;
    }

  for(i=0; i<numDerivs; i++)
    {
    xnext[i] = xprev[i] + delT*(this->Derivs[i]/6.0 +
                                this->NextDerivs[0][i]/3.0 +
                                this->NextDerivs[1][i]/3.0 +
                                this->NextDerivs[2][i]/6.0);
    }

  return 0;
}

void vtkRungeKutta4::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Runge-Kutta 4 function derivatives: " 
     << this->NextDerivs[0] << " " << this->NextDerivs[1]  << " "
     << this->NextDerivs[2] << endl;
}






