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
  , OutFunction()
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

    auto b = new vtkm::Box(MakeFVec3(xmin), MakeFVec3(xmax));
    this->OutFunction.Reset(b, true);
  }
  else if ((cylinder = vtkCylinder::SafeDownCast(function)))
  {
    double center[3], axis[3], radius;
    cylinder->GetCenter(center);
    cylinder->GetAxis(axis);
    radius = cylinder->GetRadius();

    auto c = new vtkm::Cylinder(
      MakeFVec3(center), MakeFVec3(axis), static_cast<vtkm::FloatDefault>(radius));
    this->OutFunction.Reset(c, true);
  }
  else if ((plane = vtkPlane::SafeDownCast(function)))
  {
    double origin[3], normal[3];
    plane->GetOrigin(origin);
    plane->GetNormal(normal);

    auto p = new vtkm::Plane(MakeFVec3(origin), MakeFVec3(normal));
    this->OutFunction.Reset(p, true);
  }
  else if ((sphere = vtkSphere::SafeDownCast(function)))
  {
    double center[3], radius;
    sphere->GetCenter(center);
    radius = sphere->GetRadius();

    auto s = new vtkm::Sphere(MakeFVec3(center), static_cast<vtkm::FloatDefault>(radius));
    this->OutFunction.Reset(s, true);
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

const vtkm::cont::ImplicitFunctionHandle& ImplicitFunctionConverter::Get() const
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

      auto b = static_cast<vtkm::Box*>(this->OutFunction.Get());
      b->SetMinPoint(MakeFVec3(xmin));
      b->SetMaxPoint(MakeFVec3(xmax));
    }
    else if ((cylinder = vtkCylinder::SafeDownCast(this->InFunction)))
    {
      double center[3], axis[3], radius;
      cylinder->GetCenter(center);
      cylinder->GetAxis(axis);
      radius = cylinder->GetRadius();

      auto c = static_cast<vtkm::Cylinder*>(this->OutFunction.Get());
      c->SetCenter(MakeFVec3(center));
      c->SetAxis(MakeFVec3(axis));
      c->SetRadius(static_cast<vtkm::FloatDefault>(radius));
    }
    else if ((plane = vtkPlane::SafeDownCast(this->InFunction)))
    {
      double origin[3], normal[3];
      plane->GetOrigin(origin);
      plane->GetNormal(normal);

      auto p = static_cast<vtkm::Plane*>(this->OutFunction.Get());
      p->SetOrigin(MakeFVec3(origin));
      p->SetNormal(MakeFVec3(normal));
    }
    else if ((sphere = vtkSphere::SafeDownCast(this->InFunction)))
    {
      double center[3], radius;
      sphere->GetCenter(center);
      radius = sphere->GetRadius();

      auto s = static_cast<vtkm::Sphere*>(this->OutFunction.Get());
      s->SetCenter(MakeFVec3(center));
      s->SetRadius(static_cast<vtkm::FloatDefault>(radius));
    }

    this->MTime = this->InFunction->GetMTime();
  }

  return this->OutFunction;
}

} // tovtkm
