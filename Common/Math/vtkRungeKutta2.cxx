/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRungeKutta2.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkRungeKutta2.h"

#include "vtkFunctionSet.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkRungeKutta2);

vtkRungeKutta2::vtkRungeKutta2()
{
}

vtkRungeKutta2::~vtkRungeKutta2()
{
}

// Calculate next time step
int vtkRungeKutta2::ComputeNextStep(double* xprev, double* dxprev, double* xnext,
                                    double t, double& delT, double& delTActual,
                                    double, double, double, double& error)
{
  int i, numDerivs, numVals;

  delTActual = delT;
  error = 0.0;

  if (!this->FunctionSet)
    {
    vtkErrorMacro("No derivative functions are provided!");
    return NOT_INITIALIZED;
    }

  if (!this->Initialized)
    {
    vtkErrorMacro("Integrator not initialized!");
    return NOT_INITIALIZED;
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
    memcpy(xnext, this->Vals, (numVals-1)*sizeof(double));
    return OUT_OF_DOMAIN;
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
    memcpy(xnext, this->Vals, (numVals-1)*sizeof(double));
    return OUT_OF_DOMAIN;
    }

  // Calculate x_i using improved values of derivatives
  for(i=0; i<numDerivs; i++)
    {
    xnext[i] = xprev[i] + delT*this->Derivs[i];
    }

  return 0;
}






