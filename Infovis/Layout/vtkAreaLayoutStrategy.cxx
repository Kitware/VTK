// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#include "vtkAreaLayoutStrategy.h"

#include "vtkTree.h"

VTK_ABI_NAMESPACE_BEGIN
vtkAreaLayoutStrategy::vtkAreaLayoutStrategy()
{
  this->ShrinkPercentage = 0.0;
}

vtkAreaLayoutStrategy::~vtkAreaLayoutStrategy() = default;

void vtkAreaLayoutStrategy::LayoutEdgePoints(vtkTree* inputTree,
  vtkDataArray* vtkNotUsed(coordsArray), vtkDataArray* vtkNotUsed(sizeArray),
  vtkTree* edgeRoutingTree)
{
  edgeRoutingTree->ShallowCopy(inputTree);
}

void vtkAreaLayoutStrategy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ShrinkPercentage: " << this->ShrinkPercentage << endl;
}
VTK_ABI_NAMESPACE_END
