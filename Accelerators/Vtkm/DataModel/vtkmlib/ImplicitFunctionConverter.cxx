//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2012 Sandia Corporation.
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//=============================================================================
#include "ImplicitFunctionConverter.h"

#include "vtkmFilterPolicy.h"

#include "vtkBox.h"
#include "vtkCylinder.h"
#include "vtkPlane.h"
#include "vtkSphere.h"

#include <vtkm/ImplicitFunction.h>

namespace tovtkm
{

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
    vtkGenericWarningMacro(<< "The implicit functions " << function->GetClassName()
                           << " is not supported by vtk-m.");
    return;
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

} // tovtkm
