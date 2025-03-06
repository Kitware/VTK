// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCellAttributeInformation.h"

#include "vtkBoundingBox.h"
#include "vtkCellAttribute.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkVector.h"

VTK_ABI_NAMESPACE_BEGIN

using namespace vtk::literals;

void vtkCellAttributeInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

VTK_ABI_NAMESPACE_END
