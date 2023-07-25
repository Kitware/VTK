// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkParametricRoman.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkParametricRoman);

//------------------------------------------------------------------------------
vtkParametricRoman::vtkParametricRoman()
  : Radius(1)
{
  this->MinimumU = 0;
  this->MaximumU = vtkMath::Pi();
  this->MinimumV = 0;
  this->MaximumV = vtkMath::Pi();

  this->JoinU = 1;
  this->JoinV = 1;
  this->TwistU = 1;
  this->TwistV = 0;
  this->ClockwiseOrdering = 0;
  this->DerivativesAvailable = 1;
}

//------------------------------------------------------------------------------
vtkParametricRoman::~vtkParametricRoman() = default;

//------------------------------------------------------------------------------
void vtkParametricRoman::Evaluate(double uvw[3], double Pt[3], double Duvw[9])
{
  double u = uvw[0];
  double v = uvw[1];
  double* Du = Duvw;
  double* Dv = Duvw + 3;

  double cu = cos(u);
  double c2u = cos(2.0 * u);
  double su = sin(u);
  double s2u = sin(2.0 * u);
  double cv = cos(v);
  double cv2 = cv * cv;
  double c2v = cos(2.0 * v);
  double s2v = sin(2.0 * v);
  double sv = sin(v);
  double a2 = this->Radius * this->Radius;

  // The point
  Pt[0] = a2 * cv2 * s2u / 2.0;
  Pt[1] = a2 * su * s2v / 2.0;
  Pt[2] = a2 * cu * s2v / 2.0;

  // The derivatives are:
  Du[0] = a2 * cv2 * c2u;
  Du[1] = a2 * cu * s2v / 2.0;
  Du[2] = -a2 * su * s2v / 2.0;
  Dv[0] = -a2 * cv * s2u * sv;
  Dv[1] = a2 * su * c2v;
  Dv[2] = a2 * cu * c2v;
}

//------------------------------------------------------------------------------
double vtkParametricRoman::EvaluateScalar(
  double* vtkNotUsed(uv[3]), double* vtkNotUsed(Pt[3]), double* vtkNotUsed(Duv[9]))
{
  return 0;
}

//------------------------------------------------------------------------------
void vtkParametricRoman::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Radius: " << this->Radius << "\n";
}
VTK_ABI_NAMESPACE_END
