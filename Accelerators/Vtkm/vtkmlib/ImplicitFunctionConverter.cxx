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
#include "vtkPlane.h"
#include "vtkSphere.h"

#include <vtkm/cont/ImplicitFunction.h>


namespace tovtkm {

inline vtkm::Vec<vtkm::FloatDefault, 3> MakeFVec3(const double x[3])
{
  return vtkm::Vec<vtkm::FloatDefault, 3>(static_cast<vtkm::FloatDefault>(x[0]),
                                          static_cast<vtkm::FloatDefault>(x[1]),
                                          static_cast<vtkm::FloatDefault>(x[2]));
}

ImplicitFunctionConverter::ImplicitFunctionConverter()
  : InFunction(nullptr), OutFunction(nullptr), MTime(0)
{
}

void ImplicitFunctionConverter::Set(vtkImplicitFunction *function)
{
  this->InFunction = function;

  vtkBox *box = nullptr;
  vtkPlane *plane = nullptr;
  vtkSphere *sphere = nullptr;

  if ((box = vtkBox::SafeDownCast(function)))
  {
    double xmin[3], xmax[3];
    box->GetXMin(xmin);
    box->GetXMax(xmax);

    auto b = new vtkm::cont::Box(MakeFVec3(xmin), MakeFVec3(xmax));
    b->ResetDevices(vtkmInputFilterPolicy::DeviceAdapterList());
    this->OutFunction.reset(b);
  }
  else if ((plane = vtkPlane::SafeDownCast(function)))
  {
    double origin[3], normal[3];
    plane->GetOrigin(origin);
    plane->GetNormal(normal);

    auto p = new vtkm::cont::Plane(MakeFVec3(origin), MakeFVec3(normal));
    p->ResetDevices(vtkmInputFilterPolicy::DeviceAdapterList());
    this->OutFunction.reset(p);
  }
  else if ((sphere = vtkSphere::SafeDownCast(function)))
  {
    double center[3], radius;
    sphere->GetCenter(center);
    radius = sphere->GetRadius();

    auto s = new vtkm::cont::Sphere(MakeFVec3(center),
                                    static_cast<vtkm::FloatDefault>(radius));
    s->ResetDevices(vtkmInputFilterPolicy::DeviceAdapterList());
    this->OutFunction.reset(s);
  }

  this->MTime = function->GetMTime();
}

const std::shared_ptr<vtkm::cont::ImplicitFunction>& ImplicitFunctionConverter::Get() const
{
  if (this->MTime < this->InFunction->GetMTime())
  {
    vtkBox *box = nullptr;
    vtkPlane *plane = nullptr;
    vtkSphere *sphere = nullptr;

    if ((box = vtkBox::SafeDownCast(this->InFunction)))
    {
      double xmin[3], xmax[3];
      box->GetXMin(xmin);
      box->GetXMax(xmax);

      auto b = static_cast<vtkm::cont::Box*>(this->OutFunction.get());
      b->SetMinPoint(MakeFVec3(xmin));
      b->SetMaxPoint(MakeFVec3(xmax));
    }
    else if ((plane = vtkPlane::SafeDownCast(this->InFunction)))
    {
      double origin[3], normal[3];
      plane->GetOrigin(origin);
      plane->GetNormal(normal);

      auto p = static_cast<vtkm::cont::Plane*>(this->OutFunction.get());
      p->SetOrigin(MakeFVec3(origin));
      p->SetNormal(MakeFVec3(normal));
    }
    else if ((sphere = vtkSphere::SafeDownCast(this->InFunction)))
    {
      double center[3], radius;
      sphere->GetCenter(center);
      radius = sphere->GetRadius();

      auto s = static_cast<vtkm::cont::Sphere*>(this->OutFunction.get());
      s->SetCenter(MakeFVec3(center));
      s->SetRadius(static_cast<vtkm::FloatDefault>(radius));
    }

    this->MTime = this->InFunction->GetMTime();
  }

  return this->OutFunction;
}

} // tovtkm
