/*=========================================================================

  Program:   Visualization Toolkit
  Module:    AmoebaMinimizer.cxx
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

#include "vtkAmoebaMinimizer.h"
#include <math.h>

// the function to be minimized
static void vtkFunctionToMinimize(void *arg)
{
  vtkAmoebaMinimizer *minimizer = (vtkAmoebaMinimizer *)arg;

  double x = minimizer->GetParameterValue("x");
  double y = minimizer->GetParameterValue("y");
  double z = minimizer->GetParameterValue("z");

  double r = (x-5)*(x-5) + (y+2)*(y+2) + (z)*(z);

  minimizer->SetResult(r);
}

int main(int argc, char** vtkNotUsed(argv))
{
  vtkAmoebaMinimizer *minimizer = vtkAmoebaMinimizer::New();

  minimizer->SetFunction(&vtkFunctionToMinimize, minimizer);
  minimizer->SetParameterBracket("x",-2,2);
  minimizer->SetParameterBracket("y",-2,2);
  minimizer->SetParameterBracket("z",-2,2);
  
  minimizer->Minimize();

  double x = minimizer->GetParameterValue("x");
  double y = minimizer->GetParameterValue("y");
  double z = minimizer->GetParameterValue("z");

  double r = minimizer->GetResult();

  int iterations = minimizer->GetIterations();

  int maxiterations = minimizer->GetMaxIterations();

  int noconvergence = minimizer->Iterate();

  minimizer->Delete();

  // check parameters to make sure that they converged to the
  // correct values
  if (argc > 1 ||
      fabs(x - 5.0) > 1e-4 ||
      fabs(y + 2.0) > 1e-4 ||
      fabs(z - 0.0) > 1e-4 ||
      r > 1e-4 ||
      iterations >= maxiterations ||
      noconvergence)
    {
    return 1;
    }

  return 0;
}
