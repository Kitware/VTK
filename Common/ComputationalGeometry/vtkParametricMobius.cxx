/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParametricMobius.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkParametricMobius.h"
#include "vtkObjectFactory.h"
#include "vtkMath.h"

vtkStandardNewMacro(vtkParametricMobius);

//----------------------------------------------------------------------------
vtkParametricMobius::vtkParametricMobius()
{
  this->MinimumU = 0;
  this->MaximumU = 2 * vtkMath::Pi();
  this->MinimumV = -1;
  this->MaximumV = 1;

  this->JoinU = 1;
  this->JoinV = 0;
  this->TwistU = 1;
  this->TwistV = 0;
  this->ClockwiseOrdering = 0;
  this->DerivativesAvailable = 1;

  this->Radius = 1;
}

//----------------------------------------------------------------------------
vtkParametricMobius::~vtkParametricMobius()
{
}

//----------------------------------------------------------------------------
void vtkParametricMobius::Evaluate(double uvw[3], double Pt[3],
                                   double Duvw[9])
{
  double u = uvw[0];
  double v = uvw[1];
  double *Du = Duvw;
  double *Dv = Duvw + 3;

  double cu = cos(u);
  double cu2 = cos(u / 2);
  double su = sin(u);
  double su2 = sin(u  / 2);
  double t = this->Radius - v * su2;

  // The point
  Pt[0] = t * su;
  Pt[1] = t * cu;
  Pt[2] = v * cu2;

  //The derivatives are:
  Du[0] = -v * cu2 * su / 2 + Pt[1];
  Du[1] = -v * cu2 * cu / 2 - Pt[0];
  Du[2] = -v * su2 / 2;
  Dv[0] = -su2 * su;
  Dv[1] = -su2 * cu;
  Dv[2] = cu2;
}

//----------------------------------------------------------------------------
double vtkParametricMobius::EvaluateScalar(double *, double*, double *)
{
  return 0;
}

//----------------------------------------------------------------------------
void vtkParametricMobius::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Radius: " << this->Radius << "\n";
}
