// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#include "vtkCirclePackLayoutStrategy.h"

#include "vtkTree.h"

VTK_ABI_NAMESPACE_BEGIN
vtkCirclePackLayoutStrategy::vtkCirclePackLayoutStrategy() = default;

vtkCirclePackLayoutStrategy::~vtkCirclePackLayoutStrategy() = default;

void vtkCirclePackLayoutStrategy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
