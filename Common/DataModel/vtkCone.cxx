// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCone.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkTransform.h"
#include "vtkVector.h"

#include <limits>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkCone);

//------------------------------------------------------------------------------
// Evaluate cone equation. The function's transform should have been applied to x already, so the
// computation assumes the cone to be aligned to the X-axis, and it's origin to be (0, 0, 0).
double vtkCone::EvaluateFunction(double x[3])
{
  if (!this->IsDoubleCone && x[0] < 0.)
  {
    // Any point before the origin on X
    // is out of a one-sided cone.
    return -x[0];
  }

  double tanTheta = std::tan(vtkMath::RadiansFromDegrees(this->Angle));
  return x[1] * x[1] + x[2] * x[2] - x[0] * x[0] * tanTheta * tanTheta;
}

//------------------------------------------------------------------------------
// Evaluate cone normal.
void vtkCone::EvaluateGradient(double x[3], double g[3])
{
  if (!this->IsDoubleCone && x[0] < 0.)
  {
    g[0] = 0.;
    g[1] = 0.;
    g[2] = 0.;
    return;
  }

  double tanTheta = std::tan(vtkMath::RadiansFromDegrees(this->Angle));
  g[0] = -2.0 * x[0] * tanTheta * tanTheta;
  g[1] = 2.0 * x[1];
  g[2] = 2.0 * x[2];
}

//------------------------------------------------------------------------------
void vtkCone::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "IsDoubleCone: " << this->IsDoubleCone << std::endl;

  os << indent << "Angle: " << this->Angle << std::endl;

  os << indent << "Axis: ";
  os << this->Axis[0] << " ";
  os << this->Axis[1] << " ";
  os << this->Axis[2] << std::endl;

  os << indent << "Origin: ";
  os << this->Origin[0] << " ";
  os << this->Origin[1] << " ";
  os << this->Origin[2] << std::endl;
}

//------------------------------------------------------------------------------
void vtkCone::SetOrigin(double x, double y, double z)
{
  if (x != this->Origin[0] || y != this->Origin[1] || z != this->Origin[2])
  {
    this->Origin[0] = x;
    this->Origin[1] = y;
    this->Origin[2] = z;

    this->UpdateTransform();
  }
}

//------------------------------------------------------------------------------
void vtkCone::SetOrigin(const double xyz[3])
{
  this->SetOrigin(xyz[0], xyz[1], xyz[2]);
}

//------------------------------------------------------------------------------
// Specify the cone axis. Normalize if necessary.
void vtkCone::SetAxis(double ax, double ay, double az)
{
  double axis[3] = { ax, ay, az };
  this->SetAxis(axis);
}

//------------------------------------------------------------------------------
// Specify the cone axis. Reject non-zero axis vectors. Normalize the
// axis vector.
void vtkCone::SetAxis(double a[3])
{
  // If axis length is zero, then don't change it
  if (vtkMath::Normalize(a) < std::numeric_limits<double>::epsilon())
  {
    return;
  }

  if (a[0] != this->Axis[0] || a[1] != this->Axis[1] || a[2] != this->Axis[2])
  {
    this->Axis[0] = a[0];
    this->Axis[1] = a[1];
    this->Axis[2] = a[2];

    UpdateTransform();
  }
}

//----------------------------------------------------------------------------
void vtkCone::UpdateTransform()
{
  // vtkCone is aligned to the x-axis. Setup a transform that rotates
  // <1, 0, 0> to the vector in Axis and translates according to origin
  const vtkVector3d xAxis(1., 0., 0.);
  vtkVector3d axis(this->Axis);

  vtkVector3d cross = xAxis.Cross(axis);
  double crossNorm = cross.Normalize();
  double dot = xAxis.Dot(axis);
  double angle = vtkMath::DegreesFromRadians(std::atan2(crossNorm, dot));

  vtkNew<vtkTransform> xform;
  xform->Identity();
  xform->Translate(this->Origin);
  xform->RotateWXYZ(angle, cross.GetData());
  xform->Inverse();

  this->SetTransform(xform.GetPointer());
  this->Modified();
}
VTK_ABI_NAMESPACE_END
