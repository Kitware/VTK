// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkParametricBour.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkParametricBour);
//----------------------------------------------------------------------------//
vtkParametricBour::vtkParametricBour()
{
  // Preset triangulation parameters
  this->MinimumU = 0.;
  this->MaximumU = 1.0;
  this->MinimumV = 0.;
  this->MaximumV = 4. * vtkMath::Pi();

  this->JoinU = 0;
  this->JoinV = 0;
  this->TwistU = 0;
  this->TwistV = 0;
  this->ClockwiseOrdering = 0;
  this->DerivativesAvailable = 1;
}

//----------------------------------------------------------------------------//
vtkParametricBour::~vtkParametricBour() = default;

//----------------------------------------------------------------------------//
void vtkParametricBour::Evaluate(double uvw[3], double Pt[3], double Duvw[9])
{
  // Copy the parameters out of the vector, for the sake of convenience.
  double u = uvw[0];
  double v = uvw[1];

  // We're only going to need the u and v partial derivatives.
  // The w partial derivatives are not needed.
  double* Du = Duvw;
  double* Dv = Duvw + 3;

  // Location of the point. This parametrization was taken from:
  // https://en.wikipedia.org/wiki/Bour%27s_minimal_surface
  const double u1_2 = std::sqrt(u);
  const double u3_2 = u * u1_2;
  const double cos_v = std::cos(v);
  const double cos_1_5v = std::cos(1.5 * v);
  const double cos_2v = std::cos(2. * v);
  const double sin_v = std::sin(v);
  const double sin_1_5v = std::sin(1.5 * v);
  Pt[0] = u * cos_v - u * u * cos_2v / 2.;
  Pt[1] = -u * sin_v * (u * cos_v + 1.);
  Pt[2] = 4. / 3. * u3_2 * cos_1_5v;

  // The derivative with respect to u:
  Du[0] = cos_v - u * cos_2v;
  Du[1] = -sin_v * (1. + 2. * u * cos_v);
  Du[2] = 2 * u1_2 * cos_1_5v;

  // The derivative with respect to v:
  Dv[0] = u * (2. * u * cos_v - 1.) * sin_v;
  Dv[1] = -u * (cos_v + u * cos_2v);
  Dv[2] = -2. * u3_2 * sin_1_5v;
}

//----------------------------------------------------------------------------//
double vtkParametricBour::EvaluateScalar(double*, double*, double*)
{
  return 0;
}

//----------------------------------------------------------------------------//
void vtkParametricBour::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
