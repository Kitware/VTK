/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRungeKutta45.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkRungeKutta45.h"

#include "vtkObjectFactory.h"
#include "vtkFunctionSet.h"

vtkStandardNewMacro(vtkRungeKutta45);

//----------------------------------------------------------------------------
// Cash-Karp parameters
double vtkRungeKutta45::A[5] = { 1.0/5.0, 3.0/10.0, 3.0/5.0, 1.0,
                 7.0/8.0 };
double vtkRungeKutta45::B[5][5] = { { 1.0/5.0, 0, 0, 0, 0 },
                    { 3.0/40.0, 9.0/40.0,  0, 0, 0 },
                    { 3.0/10.0, -9.0/10.0, 6.0/5.0, 0, 0 },
                    { -11.0/54.0,
                      5.0/2.0,
                      -70.0/27.0,
                      35.0/27.0,
                      0 },
                    { 1631.0/55296.0,
                      175.0/512.0,
                      575.0/13824.0,
                      44275.0/110592.0,
                      253.0/4096.0 } };
double vtkRungeKutta45::C[6] = {37.0/378.0,
                0,
                250.0/621.0,
                125.0/594.0,
                0,
                512.0/1771.0 };
double vtkRungeKutta45::DC[6] = { 37.0/378.0 - 2825.0/27648.0,
                  0,
                  250.0/621.0 - 18575.0/48384.0,
                  125.0/594.0 - 13525.0/55296.0,
                  -277.0/14336.0,
                  512.0/1771.0 - 1.0/4.0};

//----------------------------------------------------------------------------
vtkRungeKutta45::vtkRungeKutta45()
{
  for(int i=0; i<6; i++)
  {
    this->NextDerivs[i] = 0;
  }
  this->Adaptive = 1;
}

//----------------------------------------------------------------------------
vtkRungeKutta45::~vtkRungeKutta45()
{
  for(int i=0; i<6; i++)
  {
    delete[] this->NextDerivs[i];
    this->NextDerivs[i] = 0;
  }
}

//----------------------------------------------------------------------------
void vtkRungeKutta45::Initialize()
{
  this->vtkInitialValueProblemSolver::Initialize();
  if (!this->FunctionSet || !this->Initialized)
  {
    return;
  }
  // Allocate memory for temporary derivatives array
  for(int i=0; i<6; i++)
  {
    delete[] this->NextDerivs[i];
    this->NextDerivs[i] =
      new double[this->FunctionSet->GetNumberOfFunctions()];
  }
}

//----------------------------------------------------------------------------
int vtkRungeKutta45::ComputeNextStep(double* xprev, double* dxprev,
                                     double* xnext, double t, double& delT,
                                     double& delTActual,
                                     double minStep, double maxStep,
                                     double maxError, double& estErr )
{
  estErr = VTK_DOUBLE_MAX;

  // Step size should always be positive. We'll check anyway.
  if (minStep < 0)
  {
    minStep = -minStep;
  }
  if (maxStep < 0)
  {
    maxStep = -maxStep;
  }

  delTActual = 0;

  // No step size control if minStep == maxStep == delT
  double absDT = fabs(delT);
  if ( ((minStep == absDT) && (maxStep == absDT)) ||
       (maxError <= 0.0) )
  {
    int retVal = this->ComputeAStep(xprev, dxprev, xnext, t, delT, delTActual, estErr);
    return retVal;
  }
  else if ( minStep > maxStep )
  {
    return UNEXPECTED_VALUE;
  }

  double errRatio, tmp, tmp2;
  int retVal, shouldBreak = 0;

  // Reduce the step size until estimated error <= maximum allowed error
  while ( estErr > maxError )
  {
    if ((retVal =
         this->ComputeAStep(xprev, dxprev, xnext, t, delT, delTActual, estErr)))
    {
      return retVal;
    }
    // If the step just taken was either min, we are done,
    // break
    absDT = fabs(delT);
    if ( absDT == minStep )
    {
      break;
    }

    errRatio = static_cast<double>(estErr) / static_cast<double>(maxError);
    // Empirical formulae for calculating next step size
    // 0.9 is a safety factor to prevent infinite loops (see reference)
    if ( errRatio == 0.0 ) // avoid pow errors
    {
      tmp = delT < 0 ? -minStep : minStep;  // arbitrarily set to minStep
    }
    else if ( errRatio > 1 )
    {
      tmp = 0.9*delT*pow(errRatio, -0.25);
    }
    else
    {
      tmp = 0.9*delT*pow(errRatio, -0.2);
    }
    tmp2 = fabs(tmp);

    // Re-adjust step size if it exceeds the bounds
    // If this happens, calculate once with the extrama step
    // size and break (flagged by setting shouldBreak, see below)
    if (tmp2 > maxStep )
    {
      delT = maxStep * delT/fabs(delT);
      shouldBreak = 1;
    }
    else if (tmp2 < minStep)
    {
      delT = minStep * delT/fabs(delT);
      shouldBreak = 1;
    }
    else
    {
      delT = tmp;
    }

    tmp2 = t + delT;
    if ( tmp2 == t )
    {
      vtkWarningMacro("Step size underflow. You must choose a larger "
              "tolerance or set the minimum step size to a larger "
              "value.");
      return UNEXPECTED_VALUE;
    }

    // If the new step size is equal to min or max,
    // calculate once with the extrama step size and break
    // (flagged by setting shouldBreak, see above)
    if (shouldBreak)
    {
      if ( (retVal =
            this->ComputeAStep(xprev, dxprev, xnext, t, delT, delTActual, estErr)) )
      {
        return retVal;
      }
      break;
    }
  }

  return 0;
}

//----------------------------------------------------------------------------
// Calculate next time step
int vtkRungeKutta45::ComputeAStep(
  double* xprev, double* dxprev, double* xnext, double t, double& delT,
  double& actualDelT, double& error)
{
  int i, j, k, numDerivs, numVals;

  actualDelT = 0;

  if (!this->FunctionSet)
  {
    vtkErrorMacro("No derivative functions are provided!");
    return NOT_INITIALIZED ;
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
      this->NextDerivs[0][i] = dxprev[i];
    }
  }
  else if ( !this->FunctionSet->FunctionValues(this->Vals,
                           this->NextDerivs[0]) )
  {
    for(i=0; i<numVals-1; i++)
    {
      xnext[i] = this->Vals[i];
    }
    return OUT_OF_DOMAIN;
  }

  double sum;
  for (i=1; i<6; i++)
  {
    // Step i
    // Calculate k_i (NextDerivs) for each step
    for(j=0; j<numVals-1; j++)
    {
      sum = 0;
      for (k=0; k<i; k++)
      {
        sum += B[i-1][k]*this->NextDerivs[k][j];
      }
      this->Vals[j] = xprev[j] + delT*sum;
    }
    this->Vals[numVals-1] = t + delT*A[i-1];

    if ( !this->FunctionSet->FunctionValues(this->Vals,
                                            this->NextDerivs[i]) )
    {
      for(i=0; i<numVals-1; i++)
      {
        xnext[i] = this->Vals[i];
      }
      actualDelT = delT;
      return OUT_OF_DOMAIN;
    }
  }


  // Calculate xnext
  for(i=0; i<numDerivs; i++)
  {
    sum = 0;
    for (j=0; j<6; j++)
    {
      sum += C[j]*this->NextDerivs[j][i];
    }
    xnext[i] = xprev[i] + delT*sum;
  }
  actualDelT = delT;

  // Calculate norm of error vector
  double err=0;
  for(i=0; i<numDerivs; i++)
  {
    sum = 0;
    for (j=0; j<6; j++)
    {
      sum += DC[j]*this->NextDerivs[j][i];
    }
    err += delT*sum*delT*sum;
  }
  error = sqrt(err);

  int numZero = 0;
  for(i=0; i<numDerivs; i++)
  {
    if ( xnext[i] == xprev[i] )
    {
      numZero++;
    }
  }
  if (numZero == numDerivs)
  {
    return UNEXPECTED_VALUE;
  }

  return 0;
}

//----------------------------------------------------------------------------
void vtkRungeKutta45::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

