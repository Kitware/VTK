// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkAnnulus.h"
#include "vtkCylinder.h"
#include "vtkImplicitBoolean.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkTransform.h"
#include "vtkVector.h"

#include <cmath>
#include <limits>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkAnnulus);

//------------------------------------------------------------------------------
vtkAnnulus::vtkAnnulus()
{
  this->InnerCylinder->SetRadius(0.25);
  this->OuterCylinder->SetRadius(0.5);

  this->BooleanOp->AddFunction(this->OuterCylinder);
  this->BooleanOp->AddFunction(this->InnerCylinder);
  this->BooleanOp->SetOperationTypeToDifference();
}

//------------------------------------------------------------------------------
vtkAnnulus::~vtkAnnulus() = default;

//------------------------------------------------------------------------------
double vtkAnnulus::EvaluateFunction(double x[3])
{
  return this->BooleanOp->EvaluateFunction(x);
}

//------------------------------------------------------------------------------
void vtkAnnulus::EvaluateGradient(double x[3], double g[3])
{
  this->BooleanOp->EvaluateGradient(x, g);
}

//------------------------------------------------------------------------------
void vtkAnnulus::SetCenter(double x, double y, double z)
{
  this->SetCenter(vtkVector3d(x, y, z));
}

//------------------------------------------------------------------------------
void vtkAnnulus::SetCenter(const double xyz[3])
{
  this->SetCenter(vtkVector3d(xyz));
}

//------------------------------------------------------------------------------
void vtkAnnulus::SetCenter(const vtkVector3d& xyz)
{
  if (this->Center != xyz)
  {
    this->Center = xyz;
    this->UpdateTransform();
  }
}

//------------------------------------------------------------------------------
void vtkAnnulus::GetCenter(double& x, double& y, double& z)
{
  x = this->Center[0];
  y = this->Center[1];
  z = this->Center[2];
}

//------------------------------------------------------------------------------
void vtkAnnulus::GetCenter(double xyz[3])
{
  xyz[0] = this->Center[0];
  xyz[1] = this->Center[1];
  xyz[2] = this->Center[2];
}

//------------------------------------------------------------------------------
double* vtkAnnulus::GetCenter()
{
  return this->Center.GetData();
}

//------------------------------------------------------------------------------
void vtkAnnulus::SetAxis(double x, double y, double z)
{
  this->SetAxis(vtkVector3d(x, y, z));
}

//------------------------------------------------------------------------------
void vtkAnnulus::SetAxis(double axis[3])
{
  this->SetAxis(vtkVector3d(axis));
}

//------------------------------------------------------------------------------
void vtkAnnulus::SetAxis(const vtkVector3d& axis)
{
  vtkVector3d newAxis = axis;

  // Normalize axis, reject if length == 0
  if (newAxis.Normalize() < std::numeric_limits<double>::epsilon())
  {
    return;
  }

  if (this->Axis != newAxis)
  {
    this->Axis = newAxis;
    this->UpdateTransform();
  }
}

//------------------------------------------------------------------------------
void vtkAnnulus::GetAxis(double& x, double& y, double& z)
{
  x = this->Axis[0];
  y = this->Axis[1];
  z = this->Axis[2];
}

//------------------------------------------------------------------------------
void vtkAnnulus::GetAxis(double xyz[3])
{
  xyz[0] = this->Axis[0];
  xyz[1] = this->Axis[1];
  xyz[2] = this->Axis[2];
}

//------------------------------------------------------------------------------
double* vtkAnnulus::GetAxis()
{
  return this->Axis.GetData();
}

//------------------------------------------------------------------------------
void vtkAnnulus::SetOuterRadius(double radius)
{
  if (this->OuterCylinder->GetRadius() != radius)
  {
    this->OuterCylinder->SetRadius(radius);
    this->Modified();
  }
}

//------------------------------------------------------------------------------
double vtkAnnulus::GetOuterRadius() const
{
  return this->OuterCylinder->GetRadius();
}

//------------------------------------------------------------------------------
void vtkAnnulus::SetInnerRadius(double radius)
{
  if (this->InnerCylinder->GetRadius() != radius)
  {
    this->InnerCylinder->SetRadius(radius);
    this->Modified();
  }
}

//------------------------------------------------------------------------------
double vtkAnnulus::GetInnerRadius() const
{
  return this->InnerCylinder->GetRadius();
}

//------------------------------------------------------------------------------
void vtkAnnulus::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Center: " << this->Center << std::endl;
  os << indent << "Axis: " << this->Axis << std::endl;
  os << indent << "Inner Radius: " << this->InnerCylinder->GetRadius() << std::endl;
  os << indent << "Outer Radius: " << this->OuterCylinder->GetRadius() << std::endl;
}

//------------------------------------------------------------------------------
void vtkAnnulus::UpdateTransform()
{
  const vtkVector3d yAxis(0., 1., 0.);

  vtkVector3d cross = yAxis.Cross(this->Axis);
  const double crossNorm = cross.Normalize();
  const double dot = yAxis.Dot(this->Axis);
  const double angle = vtkMath::DegreesFromRadians(std::atan2(crossNorm, dot));

  vtkNew<vtkTransform> transform;
  transform->Identity();
  transform->Translate(this->Center.GetData());
  transform->RotateWXYZ(angle, cross.GetData());
  transform->Inverse();

  this->SetTransform(transform.GetPointer());
  this->Modified();
}

VTK_ABI_NAMESPACE_END
