// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-FileCopyrightText: Copyright 2012 Sandia Corporation.
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
#include "ImplicitFunctionConverter.h"

#include "vtkBox.h"
#include "vtkCylinder.h"
#include "vtkPlane.h"
#include "vtkSphere.h"

#include <vtkm/cont/ErrorBadType.h>

#include <vtkm/ImplicitFunction.h>

namespace tovtkm
{
VTK_ABI_NAMESPACE_BEGIN

inline vtkm::Vec<vtkm::FloatDefault, 3> MakeFVec3(const double x[3])
{
  return vtkm::Vec<vtkm::FloatDefault, 3>(static_cast<vtkm::FloatDefault>(x[0]),
    static_cast<vtkm::FloatDefault>(x[1]), static_cast<vtkm::FloatDefault>(x[2]));
}

ImplicitFunctionConverter::ImplicitFunctionConverter()
  : InFunction(nullptr)
  , MTime(0)
{
}

void ImplicitFunctionConverter::Set(vtkImplicitFunction* function)
{
  vtkBox* box = nullptr;
  vtkCylinder* cylinder = nullptr;
  vtkPlane* plane = nullptr;
  vtkSphere* sphere = nullptr;

  if (function->GetTransform())
  {
    throw vtkm::cont::ErrorBadType(
      "Vtk-m's implicit functions currently do not support transformations.");
  }

  if ((box = vtkBox::SafeDownCast(function)))
  {
    double xmin[3], xmax[3];
    box->GetXMin(xmin);
    box->GetXMax(xmax);

    this->OutFunction = vtkm::Box(MakeFVec3(xmin), MakeFVec3(xmax));
  }
  else if ((cylinder = vtkCylinder::SafeDownCast(function)))
  {
    double center[3], axis[3], radius;
    cylinder->GetCenter(center);
    cylinder->GetAxis(axis);
    radius = cylinder->GetRadius();

    this->OutFunction =
      vtkm::Cylinder(MakeFVec3(center), MakeFVec3(axis), static_cast<vtkm::FloatDefault>(radius));
  }
  else if ((plane = vtkPlane::SafeDownCast(function)))
  {
    double origin[3], normal[3];
    plane->GetOrigin(origin);
    plane->GetNormal(normal);

    this->OutFunction = vtkm::Plane(MakeFVec3(origin), MakeFVec3(normal));
  }
  else if ((sphere = vtkSphere::SafeDownCast(function)))
  {
    double center[3], radius;
    sphere->GetCenter(center);
    radius = sphere->GetRadius();

    this->OutFunction = vtkm::Sphere(MakeFVec3(center), static_cast<vtkm::FloatDefault>(radius));
  }
  else
  {
    std::string message = std::string("The implicit functions ") + function->GetClassName() +
      std::string(" is not supported by vtk-m.");
    throw vtkm::cont::ErrorBadType(message);
  }

  this->MTime = function->GetMTime();
  this->InFunction = function;
}

const vtkm::ImplicitFunctionGeneral& ImplicitFunctionConverter::Get()
{
  if (this->InFunction && (this->MTime < this->InFunction->GetMTime()))
  {
    vtkBox* box = nullptr;
    vtkCylinder* cylinder = nullptr;
    vtkPlane* plane = nullptr;
    vtkSphere* sphere = nullptr;

    if ((box = vtkBox::SafeDownCast(this->InFunction)))
    {
      double xmin[3], xmax[3];
      box->GetXMin(xmin);
      box->GetXMax(xmax);

      this->OutFunction = vtkm::Box(MakeFVec3(xmin), MakeFVec3(xmax));
    }
    else if ((cylinder = vtkCylinder::SafeDownCast(this->InFunction)))
    {
      double center[3], axis[3], radius;
      cylinder->GetCenter(center);
      cylinder->GetAxis(axis);
      radius = cylinder->GetRadius();

      this->OutFunction =
        vtkm::Cylinder(MakeFVec3(center), MakeFVec3(axis), static_cast<vtkm::FloatDefault>(radius));
    }
    else if ((plane = vtkPlane::SafeDownCast(this->InFunction)))
    {
      double origin[3], normal[3];
      plane->GetOrigin(origin);
      plane->GetNormal(normal);

      this->OutFunction = vtkm::Plane(MakeFVec3(origin), MakeFVec3(normal));
    }
    else if ((sphere = vtkSphere::SafeDownCast(this->InFunction)))
    {
      double center[3], radius;
      sphere->GetCenter(center);
      radius = sphere->GetRadius();

      this->OutFunction = vtkm::Sphere(MakeFVec3(center), static_cast<vtkm::FloatDefault>(radius));
    }

    this->MTime = this->InFunction->GetMTime();
  }

  return this->OutFunction;
}

VTK_ABI_NAMESPACE_END
} // tovtkm
