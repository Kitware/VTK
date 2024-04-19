// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "uses_vtk.h"

#include "vtkNew.h"
#include "vtkObject.h"

std::string vtkObject_class_name()
{
  vtkNew<vtkObject> obj;
  return obj->GetClassName();
}
