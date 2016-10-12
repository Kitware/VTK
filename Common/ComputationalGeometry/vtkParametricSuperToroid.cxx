/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParametricSuperToroid.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkParametricSuperToroid.h"
#include "vtkObjectFactory.h"
#include "vtkMath.h"
#include <cmath>

vtkStandardNewMacro(vtkParametricSuperToroid);

namespace
{
  /**
  * Calculate sign(x)*(abs(x)^n).
  */
  double SgnPower(double x, double n)
  {
    if (x == 0)
    {
      return 0;
    }
    if (n == 0)
    {
      return 1;
    }
    double sgn = (x < 0) ? -1 : 1;
    return sgn * std::pow(std::abs(x), n);
  }

} // anonymous namespace

//----------------------------------------------------------------------------
vtkParametricSuperToroid::vtkParametricSuperToroid() :
  RingRadius(1)
  , CrossSectionRadius(0.5)
  , XRadius(1)
  , YRadius(1)
  , ZRadius(1)
  , N1(1)
  , N2(1)
{
  // Preset triangulation parameters
  this->MinimumU = 0;
  this->MaximumU = 2.0 * vtkMath::Pi();
  this->MinimumV = 0;
  this->MaximumV = 2.0 * vtkMath::Pi();

  this->JoinU = 0;
  this->JoinV = 0;
  this->TwistU = 0;
  this->TwistV = 0;
  this->ClockwiseOrdering = 0;
  this->DerivativesAvailable = 0;
}

//----------------------------------------------------------------------------
vtkParametricSuperToroid::~vtkParametricSuperToroid()
{
}


//----------------------------------------------------------------------------
void vtkParametricSuperToroid::Evaluate(double uvw[3], double Pt[3],
                                        double Duvw[9])
{
  double u = uvw[0];
  double v = uvw[1];
  double *Du = Duvw;
  double *Dv = Duvw + 3;

  for (int i = 0; i < 3; ++i)
  {
    Pt[i] = Du[i] = Dv[i] = 0;
  }

  double cu = cos(u);
  double su = sin(u);
  double cv = cos(v);
  double sv = sin(v);

  double tmp  = this->RingRadius + this->CrossSectionRadius * SgnPower(cv,
                this->N2);

  // The point
  Pt[0] = this->XRadius * tmp * SgnPower(su, this->N1);
  Pt[1] = this->YRadius * tmp * SgnPower(cu, this->N1);
  Pt[2] = this->ZRadius * this->CrossSectionRadius * SgnPower(sv,
          this->N2);
}

//----------------------------------------------------------------------------
double vtkParametricSuperToroid::EvaluateScalar(double*, double*,
    double*)
{
  return 0;
}

//----------------------------------------------------------------------------
void vtkParametricSuperToroid::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Ring radius: " << this->RingRadius << "\n";
  os << indent << "Cross-sectional radius: " << this->CrossSectionRadius
     << "\n";
  os << indent << "Squareness in the z-axis: " << this->N1 << "\n";
  os << indent << "Squareness in the x-y plane: " << this->N2 << "\n";
  os << indent << "X scale factor: " << this->XRadius << "\n";
  os << indent << "Y scale factor: " << this->YRadius << "\n";
  os << indent << "Z scale factor: " << this->ZRadius << "\n";

}
