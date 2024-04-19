// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkGDAL.h"
#include "vtkInformationIntegerVectorKey.h"
#include "vtkInformationKey.h"
#include "vtkInformationStringKey.h"

VTK_ABI_NAMESPACE_BEGIN
vtkGDAL::vtkGDAL() = default;

vtkGDAL::~vtkGDAL() = default;

void vtkGDAL::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

vtkInformationKeyMacro(vtkGDAL, MAP_PROJECTION, String);
vtkInformationKeyRestrictedMacro(vtkGDAL, FLIP_AXIS, IntegerVector, 3);
VTK_ABI_NAMESPACE_END
